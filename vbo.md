
本文主要研究vbo如何下发到gpu。


## Vertex Buffer Object


### vbo的创建

```
glGenBuffers  
	create_buffers_err  
		create_buffers  
```

在mesa中create_buffers内部没有实际调用驱动函数进行对象的产生，而是直接将bufferid绑定到了一个全局变量DummyBufferObject

从注释来看，这段代码定义了一个名为`DummyBufferObject`的静态结构体变量，用作在调用`glGenBuffers()`和`glBindBuffer()`之间的占位符缓冲对象。这种方法允许`glIsBuffer()`函数正常工作，即使在生成和绑定实际缓冲对象之前。这样的设计可能有助于代码的逻辑和可读性，同时确保OpenGL函数的正确使用。

```
glBindBuffer
	_mesa_BindBuffer  
		get_buffer_target  
		bind_buffer_object  
			_mesa_lookup_bufferobj
			_mesa_reference_buffer_object_

```
如果绑定目标为GL_ARRAY_BUFFER,bind_buffer_object中首先会通过_mesa_lookup_bufferobj 找出之前glGenBuffers产生的gl_buffer_object .
然后将之绑定到gl_context的Array中的ArrayBufferObj中。

```
glBufferData
	get_buffer
		get_buffer_target
	_mesa_buffer_data
		buffer_data_error
			buffer_data

	
```



### 描述符配置初始化


描述符配置初始化是在驱动上下文创建的时候进行初始化的。

流程如下
描述符的初始化是在` si_init_all_descriptors`函数里面进行初始化，  它在si_create_context时候调用。 它主要初始化了si_context中的 si_descriptor，rw_buffers, const_and_shader_buffers变量。

```c
/* Indices into sctx->descriptors, laid out so that gfx and compute pipelines
 * are contiguous:
 *
 *  0 - rw buffers
 *  1 - vertex const and shader buffers
 *  2 - vertex samplers and images
 *  3 - fragment const and shader buffer
 *   ...
 *  11 - compute const and shader buffers
 *  12 - compute samplers and images
 */
enum {
	SI_SHADER_DESCS_CONST_AND_SHADER_BUFFERS,
	SI_SHADER_DESCS_SAMPLERS_AND_IMAGES,
	SI_NUM_SHADER_DESCS,
};

#define SI_DESCS_RW_BUFFERS            0
#define SI_DESCS_FIRST_SHADER          1
#define SI_DESCS_FIRST_COMPUTE         (SI_DESCS_FIRST_SHADER + \
                                        PIPE_SHADER_COMPUTE * SI_NUM_SHADER_DESCS)
#define SI_NUM_DESCS                   (SI_DESCS_FIRST_SHADER + \
                                        SI_NUM_SHADERS * SI_NUM_SHADER_DESCS)

#define SI_DESCS_SHADER_MASK(name) \
	u_bit_consecutive(SI_DESCS_FIRST_SHADER + \
			  PIPE_SHADER_##name * SI_NUM_SHADER_DESCS, \
			  SI_NUM_SHADER_DESCS)

struct si_context {

	...
	...
/*着色其描述符 */
struct si_descriptors descriptors[SI_NUM_DESC];
struct si_buffer_resources rw_buffers;
struct si_buffer_resources const_and_shader_buffers[SI_NUM_SHADERS];
	...
	...

struct si_shader_data		shader_pointers;

	...
	...

}

```
descriptors[SI_DESCS_RW_BUFFERS] 存放的是rw_buffers
descriptors从SI_DESCS_FIRST_SHADER索引开始为每个shader存放const_shaderbuffer和samplers_imagesbuffer,每个各占两个。

 每个描述符所分配的buffer数量为SI_NUM_BUFFERS（16) + SI_NUM_CONST_BUFFERS(16) = 32个。
 还会设置每个shader所用用户数据常量的寄存器基地址，把他放进shader_pointers里面.
 默认设置是：

 | shader              |  寄存器  |
 |---|---|
 | PIPE_SHADER_VERTEX  | R_00B130_SPI_SHADER_USER_DATA_VS_0|
 | PIPE_SHADER_TESS_CTRL | R_00B430_SPI_SHADER_USER_DATA_HS_0 (<GFX9)| 
 | PIPE_SHADER_GEOMETRY | R_00B230_SPI_SHADER_USER_DATA_GS_0(<GFX9)| 
 | PIPE_SHADER_FRAGMENT | R_00B030_SPI_SHADER_USER_DATA_PS_0| 

它的功能总结如下:

1. 针对每种类型的着色器（vertex、tessellation control、geometry、fragment），它初始化了这些着色器的缓冲区资源描述符以及采样器和图像描述符。

2. 初始化读写缓冲区资源描述符。

3. 初始化可绑定的（bindless）纹理和图像描述符。

4. 初始化一些相关的管线状态和设置。

下面对每个主要部分进行详细解释：

1. 对每种类型的着色器（vertex、tessellation control、geometry、fragment），它：
   - 计算描述符的相关参数，如偏移量和槽数量。
   - 使用这些参数调用 `si_init_buffer_resources` 初始化缓冲区资源。
   - 初始化采样器和图像描述符。
   - 设置默认和不可变的用户数据映射。

2. 对于读写缓冲区资源描述符，它：
   - 使用 `si_init_buffer_resources` 初始化读写缓冲区资源。
   - 设置相关的使用和优先级。

3. 对于可绑定的（bindless）纹理和图像描述符，它：
   - 调用 `si_init_bindless_descriptors` 初始化描述符。
   - 设置描述符的相关参数，如偏移量和槽数量。

4. 初始化管线状态和设置，它：
   - 设置一些管线函数，如绑定采样器状态、设置着色器图像、设置常量缓冲区等。
   - 设置一些相关的管线状态，如 shader user data 的处理方式。

```


## 顶点资源描述符上传



```c

struct si_vertex_elements
{
	// 指向一个 r600_resource 结构体，该资源用于存储实例分频因子的缓冲区。
	struct r600_resource *instance_divisor_factor_buffer;

	// 一个包含 SI_MAX_ATTRIBS 个元素的数组，用于存储每个顶点属性的 rsrc_word3 值。
	uint32_t rsrc_word3[SI_MAX_ATTRIBS];

	// 一个包含 SI_MAX_ATTRIBS 个元素的数组，用于存储每个顶点属性在数据中的偏移量。
	uint16_t src_offset[SI_MAX_ATTRIBS];

	// 一个包含 SI_MAX_ATTRIBS 个元素的数组，用于存储每个顶点属性是否需要修复获取（fix fetch）。
	uint8_t fix_fetch[SI_MAX_ATTRIBS];

	// 一个包含 SI_MAX_ATTRIBS 个元素的数组，用于存储每个顶点属性的格式大小。
	uint8_t format_size[SI_MAX_ATTRIBS];

	// 一个包含 SI_MAX_ATTRIBS 个元素的数组，用于存储每个顶点属性所属的顶点缓冲区索引。
	uint8_t vertex_buffer_index[SI_MAX_ATTRIBS];

	// 用于存储顶点属性的数量。
	uint8_t count;

	// 一个布尔值，指示是否使用了实例分频因子。
	bool uses_instance_divisors;

	// 一个用于存储使用第一个顶点缓冲区的位掩码。
	uint16_t first_vb_use_mask;

	// 用于存储顶点缓冲区描述符列表的字节大小，以便进行最优预取（prefetch）。
	uint16_t desc_list_byte_size;

	// 一个位掩码，用于指示哪些输入的实例分频因子为1。
	uint16_t instance_divisor_is_one;

	// 一个位掩码，用于指示哪些输入的实例分频因子是通过获取获得的。
	uint16_t instance_divisor_is_fetched;
};



struct si_context {

	...
	...
	
	struct si_vertex_elements *vertex_elements;

	...
	...

}

```
顶点描述副资源上传功能在si_upload_vertex_buffer_descriptors

si_draw_vbo -> si_upload_buffer_descriptors

这时已经进行shader binary的编译下发，顶点数据都已经获取。

 **`si_upload_vertex_buffer_descriptors`：** 目的是将顶点缓冲区的描述符上传到 GPU。描述符是用于描述顶点缓冲区在 GPU 内部的位置和属性的数据。


在上下文中有一种情况需要特别注意。当在图形渲染过程中需要上传顶点缓冲区描述符时，这个操作必须在以下两个操作之后进行：

1. **`si_need_cs_space` 的调用：** 由于上传顶点缓冲区描述符可能需要发送命令到 GPU（可能需要额外的 CS 空间），确保先检查命令流的空间是否足够。

2. **`si_context_add_resource_size` 的调用：** 在调用这个函数之前，应该先上传顶点缓冲区描述符。因为这个函数可能会计算资源的内存大小，上传描述符后的资源大小可能会影响这个计算。

如果在上传描述符之前调用了 `si_context_add_resource_size`，可能会导致描述符的内存大小被错误地计算，从而影响资源的管理和使用。



在si_upload_vertex_buffer_descriptors 内部， 该函数首先调用了u_upload_alloc.

由u_upload_alloc注释可知 顶点缓冲区描述符是唯一一个直接通过分段缓冲区进行上传的，不经过细粒度上传路径。 


### 分段缓冲 (staging buffer)

分段缓冲（staging buffer）是一种在图形编程中常用的技术，用于在 CPU 和 GPU 之间传输数据。它的主要目的是为了在数据传输时提供一种高效的方式，以优化性能和内存使用。

分段缓冲（staging buffer）的概念：

1. **CPU 和 GPU 内存：** 在计算机系统中，CPU 和 GPU 拥有各自的内存。CPU 内存通常称为系统内存，而 GPU 内存通常称为显存。这两种内存之间的数据传输涉及到不同的总线和延迟。

2. **数据传输问题：** 在图形渲染中，有时需要在 CPU 和 GPU 之间传输数据，例如纹理、顶点数据等。但是，直接从 CPU 内存传输到 GPU 内存可能效率较低，因为 CPU 和 GPU 内存之间的传输需要经过 PCIe 总线，存在延迟。

3. **分段缓冲的解决方案：** 分段缓冲是一种中间存储区域，位于 CPU 内存中，但与 GPU 内存进行更有效的数据传输。CPU 可以更快地写入分段缓冲，然后一次性将整个缓冲区上传到 GPU 内存，从而减少了 CPU-GPU 之间的频繁传输。这种方法利用了 CPU 内存和 GPU 内存之间更快速的数据传输速度。

总之，分段缓冲（staging buffer）是一种在 CPU 和 GPU 之间进行数据传输的优化技术，通过使用中间缓冲区来减少频繁的数据传输，从而提高性能并优化内存使用。这在图形渲染、纹理加载和数据上传等场景中经常被使用。



### 细粒度上传路径(Fine-grained upload path)


"Fine-grained upload path" 是指细粒度上传路径，它在图形编程中用来描述一种更精细、更优化的数据上传方式。在这个上下文中，上传是指从 CPU 内存向 GPU 内存传输数据，通常是为了在渲染过程中使用这些数据。

细粒度上传路径的特点包括：

1. **更小的数据块：** 在细粒度上传路径中，数据通常以更小的块（或称为 "粒度"）上传到 GPU 内存。这样可以减小单次传输的数据量，从而提高效率。

2. **更频繁的传输：** 细粒度上传路径可能涉及更频繁的数据传输。这是因为以较小的粒度上传，所以可能需要更多次的传输来完成整个数据上传过程。

3. **更精细的控制：** 使用细粒度上传路径时，开发人员可以更精细地控制数据的传输和使用。例如，可以根据需求选择哪些数据需要在渲染过程中上传，以及何时上传。

4. **减少内存占用：** 由于细粒度上传通常以小块数据传输，所以可以在上传后释放 CPU 内存，从而减少内存占用。

细粒度上传路径在某些情况下可以提供更好的性能，特别是当需要快速更新部分数据，或者在帧之间进行数据交换时。然而，细粒度上传路径通常需要更多的编程和管理工作，因为开发人员需要更精细地管理传输的时机和数据块。

综上所述，"fine-grained upload path" 表示一种更小块、更频繁的数据上传方式，用于在图形编程中优化数据传输的性能和效率。



## u_upload_mgr

这个工具代码位于src/gallium/axiliary/util/目录下

这个工具代码提供了一种便捷的方式来上传用户数据到 GPU 并管理缓冲区。它还通过将小缓冲区合并为更大的缓冲区来优化数据传输和存储。

u_upload_mgr结构:


```c

struct si_context {

	...

	struct u_upload_mgr		*cached_gtt_allocator;	

	...
}

struct pipe_context {

		...
		...

		/**
		 * 由驱动程序创建的流式上传管理器。所有驱动程序、状态跟踪器和模块都应该使用它们。
		 *
		 * 使用 u_upload_alloc 或 u_upload_data 多次。
		 * 完成后，使用 u_upload_unmap。
		 */
		struct u_upload_mgr *stream_uploader; /* 除了着色器常量外的所有内容上传 */
		struct u_upload_mgr *const_uploader;  /* 仅用于着色器常量 */

		...	
		...

}


struct u_upload_mgr {
    struct pipe_context *pipe;  /* 图形API上下文 */

    unsigned default_size;  /* 默认上传缓冲区的最小大小，以字节为单位 */
    unsigned bind;          /* 缓冲区绑定标志的位掩码，表示绑定到GPU的资源类型 */

    enum pipe_resource_usage usage;  /* 上传缓冲区的资源使用情况 */

    unsigned flags;  /* 配置上传管理器行为的标志位 */

    unsigned map_flags;  /* 映射上传缓冲区的标志位的位掩码 */

    boolean map_persistent; /* 指示是否支持持久性映射 */

    struct pipe_resource *buffer;   /* 上传缓冲区，用于在GPU内存中存储上传的数据 */
    struct pipe_transfer *transfer; /* 上传缓冲区传输对象，用于CPU和GPU之间的数据传输 */

    uint8_t *map;    /* 映射上传缓冲区的指针，允许CPU和GPU共享同一片内存 */
    unsigned offset; /* 指向上传缓冲区中下一个未使用的字节的偏移量 */
};

```


### umgr创建

stream_uploader 和 const_uploader 创建是在si_create_context中调用u_upload_create进行创建。
u_upload_create 内部实际上就是对u_upload_mgr的初始化，并没有进行实际的buffer申请。

u_upload_mgr最重要的字段就是buffer和map, buffer保存实际上为r600_resource

### u_upload_alloc

它是从上传缓冲区中分配新内存的子分配函数。
u_upload_alloc 逻辑是首先判断buffer是否创建， 如果已经创建再根据当前偏移offset加上此次分配的大小是否大于buffersize， 出现任何一种 则进行buffer的再次分配，调用u_upload_alloc_buffer， 这个函数里面进而调用winsys的bo_create申请gpu buffer . 然后调用cpu_map 进行cpu地址的map，将bo分配的cpu地址保存在map字段。每次分配都会将map将上当前的偏移返回给上层调用，这样就获取了子分配的使用地址。同时将outbuf指向这个buffer地址。


### u_upload_data

该接口是u_upload_alloc的封装， 将data复制在从u_upload_alloc获得的cpu地址。


## 顶点属性的设置

### vertex_elements 的存储



```c

struct gl_context {

	...
	...

	gl_array_attrib Array; // 顶点数组

	...
	...

}

/**
 * 顶点数组状态
 */
struct gl_array_attrib
{
   /** 当前绑定的顶点数组对象 */
   struct gl_vertex_array_object *VAO;

   /** 默认的顶点数组对象 */
   struct gl_vertex_array_object *DefaultVAO;

   /** 上次通过DSA函数访问的顶点数组对象 */
   struct gl_vertex_array_object *LastLookedUpVAO;

   /** 数组对象（GL_ARB_vertex_array_object） */
   struct _mesa_HashTable *Objects;
   GLint ActiveTexture;       /**< 当前活动的纹理单元 */
   GLuint LockFirst;          /**< GL_EXT_compiled_vertex_array */
   GLuint LockCount;          /**< GL_EXT_compiled_vertex_array */

   /**
    * \name 原始重启控制
    *
    * 如果设置了 \c PrimitiveRestart 或 \c PrimitiveRestartFixedIndex，将启用原始重启。
    */
   /*@{*/
   GLboolean PrimitiveRestart;
   GLboolean PrimitiveRestartFixedIndex;
   GLboolean _PrimitiveRestart;
   GLuint RestartIndex;
   /*@}*/

   /* GL_ARB_vertex_buffer_object */
   struct gl_buffer_object *ArrayBufferObj;

   /**
    * 与当前活动绘制命令一起使用的顶点数组对象。
    * _DrawVAO 可能会被设置为当前绑定的VAO（用于数组类型绘制），
    * 也可能会被设置为vbo模块设置的内部VAO，用于执行立即模式或显示列表绘制。
    */
   struct gl_vertex_array_object *_DrawVAO;
   /**
    * 从当前 _DrawVAO 引用的VAO中有效的 VERT_BIT_* 位。
    * 这始终是 _mesa_get_vao_vp_inputs(_DrawVAO) 的子集，
    * 但可能省略那些在当前 gl_vertex_program_state::_VPMode 下不会被引用的数组。
    * 例如，在执行固定功能数组绘制时，会从 _DrawVAO 的启用数组中省略通用属性（generic attributes）。
    */
   GLbitfield _DrawVAOEnabledAttribs;
   /**
    * 最初或者如果 _DrawVAO 引用的VAO被删除，则 _DrawVAO 指针会设置为 _EmptyVAO，
    * 这是一个始终为空的VAO。
    */
   struct gl_vertex_array_object *_EmptyVAO;

   /** 合法的数组数据类型及其计算API */
   GLbitfield LegalTypesMask;
   gl_api LegalTypesMaskAPI;
};


/**
 * 描述顶点数组属性的结构体。
 *
 * 包含大小、类型、格式和归一化标志，以及顶点缓冲绑定点的索引。
 *
 * 需要注意，`Stride` 字段对应于 `VERTEX_ATTRIB_ARRAY_STRIDE`，仅出于向后兼容性而存在。
 * 渲染始终使用 `VERTEX_BINDING_STRIDE`。`gl*Pointer()` 函数将同时设置
 * `VERTEX_ATTRIB_ARRAY_STRIDE` 和 `VERTEX_BINDING_STRIDE` 的值，而
 * `glBindVertexBuffer()` 仅会设置 `VERTEX_BINDING_STRIDE`。
 */
struct gl_array_attributes
{
   /** 指向客户端数组数据的指针。当绑定了顶点缓冲对象（VBO）时不使用。 */
   const GLubyte *Ptr;
   /** 相对于绑定偏移的第一个元素的偏移量 */
   GLuint RelativeOffset;
   GLshort Stride;          /**< 通过 gl*Pointer() 指定的步长 */
   GLenum16 Type;           /**< 数据类型：GL_FLOAT、GL_INT 等 */
   GLenum16 Format;         /**< 默认格式：GL_RGBA，但可能是 GL_BGRA 等 */
   GLboolean Enabled;       /**< 数组是否启用 */
   GLubyte Size;            /**< 每个元素的分量个数（1、2、3、4） */
   unsigned Normalized:1;   /**< 将定点值转换为浮点值时，是否对定点值进行归一化 */
   unsigned Integer:1;      /**< 定点值是否不转换为浮点值 */
   unsigned Doubles:1;      /**< 双精度值是否不转换为浮点值 */
   unsigned _ElementSize:8; /**< 每个元素的大小（以字节为单位） */
   /** 索引，指向 gl_vertex_array_object::BufferBinding[] 数组 */
   unsigned BufferBindingIndex:6;

   /**
    * 派生的有效缓冲绑定索引
    *
    * 指向 vao 的 gl_vertex_buffer_binding 数组的索引。
    * 与 BufferBindingIndex 类似，但应用了位置/通用属性的映射，并将相同的
    * gl_vertex_buffer_binding 条目折叠到 vao 中的单个条目。
    *
    * 该值在调用 _mesa_update_vao_derived_arrays 之后仍然有效。
    * 需要注意，当将 VAO 绑定到 Array._DrawVAO 时，会调用 _mesa_update_vao_derived_arrays。
    */
   unsigned _EffBufferBindingIndex:6;
   /**
    * 派生的有效相对偏移。
    *
    * 相对于 gl_vertex_buffer_binding::_EffOffset 中的有效缓冲偏移的相对偏移。
    *
    * 该值在调用 _mesa_update_vao_derived_arrays 之后仍然有效。
    * 需要注意，当将 VAO 绑定到 Array._DrawVAO 时，会调用 _mesa_update_vao_derived_arrays。
    */
   GLushort _EffRelativeOffset;
};



/**
 * GL_ARB_vertex/pixel_buffer_object 缓冲对象
 */
struct gl_buffer_object
{
   GLint RefCount;               /**< 引用计数，用于跟踪对象的使用情况 */
   GLuint Name;                  /**< 缓冲对象的标识符/名称 */
   GLchar *Label;                /**< 与缓冲对象关联的调试标签（GL_KHR_debug） */
   GLenum16 Usage;               /**< 缓冲使用提示，如 GL_STREAM_DRAW_ARB、GL_STREAM_READ_ARB 等 */
   GLbitfield StorageFlags;      /**< 标志位，指示存储属性，如 GL_MAP_PERSISTENT_BIT 等 */
   GLsizeiptrARB Size;           /**< 缓冲存储的大小，以字节为单位 */
   GLubyte *Data;                /**< 存储位置的指针（在 RAM 或 VRAM 中） */
   GLboolean DeletePending;      /**< 标志，指示缓冲对象是否被标记为删除 */
   GLboolean Written;            /**< 标志，指示数据是否曾写入缓冲中（用于调试） */
   GLboolean Purgeable;          /**< 标志，指示缓冲是否可在内存压力下清除 */
   GLboolean Immutable;          /**< 标志，指示缓冲数据是否不可变（GL_ARB_buffer_storage） */
   gl_buffer_usage UsageHistory; /**< 记录缓冲如何在历史中被使用 */

   /** 用于缓冲使用警告的计数器 */
   GLuint NumSubDataCalls;       /**< subdata 更新调用的次数 */
   GLuint NumMapBufferWriteCalls;/**< map buffer 写入调用的次数 */

   struct gl_buffer_mapping Mappings[MAP_COUNT]; /**< 缓冲的映射数组 */

   /** 针对静态索引缓冲的最小/最大索引计算的缓存 */
   simple_mtx_t MinMaxCacheMutex; /**< 用于保护最小/最大索引缓存的互斥体 */
   struct hash_table *MinMaxCache;/**< 存储计算出的最小/最大索引的缓存 */
   unsigned MinMaxCacheHitIndices;/**< 最小/最大索引计算中缓存命中的次数 */
   unsigned MinMaxCacheMissIndices;/**< 最小/最大索引计算中缓存未命中的次数 */
   bool MinMaxCacheDirty;        /**< 标志，指示缓存是否需要更新 */

   bool HandleAllocated;         /**< 标志，指示是否为绑定无限纹理分配了句柄（GL_ARB_bindless_texture） */
};




/**
 * 表示OpenGL 3.1+ 或 GL_ARB_vertex_array_object 扩展中的"顶点数组对象"（VAO）。
 */
struct gl_vertex_array_object
{
   /** 从 glGenVertexArray 接收到的 VAO 名称。 */
   GLuint Name;

   GLint RefCount;

   GLchar *Label;       /**< GL_KHR_debug */

   /**
    * 这个数组对象是否已绑定？
    */
   GLboolean EverBound;

   /**
    * 如果对象在上下文之间共享且不可变，则标记为 true。
    * 然后使用原子操作和线程安全进行引用计数。
    * 用于显示列表（dlist）VAO。
    */
   bool SharedAndImmutable;

   /** 顶点属性数组 */
   struct gl_array_attributes VertexAttrib[VERT_ATTRIB_MAX];

   /** 顶点缓冲绑定 */
   struct gl_vertex_buffer_binding BufferBinding[VERT_ATTRIB_MAX];

   /** 指示哪些顶点数组有与之关联的顶点缓冲。 */
   GLbitfield VertexAttribBufferMask;

   /** 使用 VERT_BIT_* 值的掩码，表示哪些数组已启用 */
   GLbitfield _Enabled;

   /**
    * 使用 VERT_BIT_* 值的掩码，表示超过位置/generic0 映射的已启用数组
    *
    * 该值在调用 _mesa_update_vao_derived_arrays 后是有效的。
    * 注意，当将 VAO 绑定到 Array._DrawVAO 时会调用 _mesa_update_vao_derived_arrays。
    */
   GLbitfield _EffEnabledVBO;

   /** 表示位置/generic0 属性的映射方式 */
   gl_attribute_map_mode _AttributeMapMode;

   /** 使用 VERT_BIT_* 值的掩码，表示已更改/脏的数组 */
   GLbitfield NewArrays;

   /** 索引缓冲（也称为OpenGL中的元素数组缓冲）。 */
   struct gl_buffer_object *IndexBufferObj;
};


/**
 * 描述用于顶点数组（或多个顶点数组）的缓冲对象。
 * 如果 BufferObj 指向默认/空缓冲对象，则顶点数组位于用户内存中，而不是 VBO 中。
 */
struct gl_vertex_buffer_binding
{
   GLintptr Offset;                    /**< 用户指定的偏移量 */
   GLsizei Stride;                     /**< 用户指定的步长 */
   GLuint InstanceDivisor;             /**< GL_ARB_instanced_arrays */
   struct gl_buffer_object *BufferObj; /**< GL_ARB_vertex_buffer_object */
   GLbitfield _BoundArrays;            /**< 绑定到此绑定点的数组 */

   /**
    * 派生的有效绑定数组。
    *
    * 有效绑定处理超过位置/generic0 属性映射的已启用数组，
    * 并将引用的 gl_vertex_buffer_binding 条目减少为唯一子集。
    *
    * 该值在调用 _mesa_update_vao_derived_arrays 后是有效的。
    * 注意，当将 VAO 绑定到 Array._DrawVAO 时会调用 _mesa_update_vao_derived_arrays。
    */
   GLbitfield _EffBoundArrays;
   /**
    * 派生的偏移量。
    *
    * 绝对偏移量，使得我们可以将一些属性折叠到此唯一的有效绑定中。
    * 对于用户空间数组绑定，它包含绑定和交错数组中最小的指针值。
    * 对于 VBO 绑定，它包含一个偏移量，使得属性 _EffRelativeOffset 保持正值，并在 Const.MaxVertexAttribRelativeOffset 范围内。
    *
    * 该值在调用 _mesa_update_vao_derived_arrays 后是有效的。
    * 注意，当将 VAO 绑定到 Array._DrawVAO 时会调用 _mesa_update_vao_derived_arrays。
    */
   GLintptr _EffOffset;
};


```
可以看到mesa将所有的属性放到一个VertexAttrib的属性数组里面。这样就可以根据属性枚举值获取相关属性在顶点数组的信息。获取到
gl_array_attributes后又可以根据gl_array_attributes里的_EffBufferBindingIndex 找到保存在BufferBinding里的相关gl_vertex_buffer_binding相关信息。从而找到bufferobject

顶点数据存放在pipe_vertex_buffer,描述顶点属性相关信息的结构放在pipe_vertex_element.
通常，绘制某个图形所需的所有顶点数据和属性都会放在一个缓冲区中。 但也可以根据需要将不同属性的数据分别放在不同的缓冲区中。例如，可以将颜色数据放在一个缓冲区中，将纹理坐标数据放在另一个缓冲区中。

	sctx->b.create_vertex_elements_state = si_create_vertex_elements;
	sctx->b.bind_vertex_elements_state = si_bind_vertex_elements;
	sctx->b.delete_vertex_elements_state = si_delete_vertex_element;
	sctx->b.set_vertex_buffers = si_set_vertex_buffers;

_mesa_draw_arrays 
	st_draw_vbo
		prepare_draw
			st_validate_draw
				st_upload_array
					set_vertex_attribs
						cso_set_vertex_elements
							v_vbuf_set_vertex_elements
								u_vbuf_set_vertex_elements_internal
									si_bind_vertex_elements




## vbuf 模块

vbuf模块位于src/gallium/auxiliary/util/中。

下面为buf的注释说明。

```plaintext
/***********************************************************************/

/**
 * 该模块负责上传用户缓冲区，并将包含不兼容顶点的顶点缓冲区进行转换，
 * 转换为兼容的顶点，基于Gallium CAPs的支持情况。
 *
 * 该模块不上传索引缓冲区。
 *
 * 该模块大量使用位掩码来表示每个缓冲区和每个顶点元素的标志，
 * 以避免在循环遍历缓冲区列表时查看是否存在非零的步幅、用户缓冲区、不支持的格式等情况。 * * 有三类顶点元素，会分别进行处理： * - 每个顶点属性（步幅 != 0，实例除数 == 0）
 * - 实例化的属性（步幅 != 0，实例除数 > 0）
 * - 常量属性（步幅 == 0）
 *
 * 所有所需的上传和转换都在每次绘制命令中执行，但只上传或
 * 转换绘制命令所需的子集的顶点。（模块从不转换整个缓冲区）
 *
 *
 * 该模块由两个主要部分组成：
 *
 *
 * 1) 转换（u_vbuf_translate_begin/end）
 *
 * 这实际上是一个顶点提取的回退机制。它将顶点从一个顶点缓冲区翻译到
 * 另一个未使用的顶点缓冲区槽中。它执行使顶点可被硬件读取所需的操作
 * （更改顶点格式、对齐偏移和步幅等）。在此使用转换模块。
 *
 * 每个类别的三个类别都会被翻译成一个单独的缓冲区。
 * 仅翻译[min_index，max_index]范围内的顶点。对于实例化的属性，
 * 范围是[start_instance，start_instance+instance_count]。对于常量属性，
 * 范围是[0，1]。
 *
 *
 * 2) 用户缓冲区上传（u_vbuf_upload_buffers）
 *
 * 仅上传[min_index，max_index]范围内的顶点（与转换类似），
 * 使用一个memcpy操作。
 *
 * 此方法对于非索引绘制操作或索引绘制操作特别有效，
 * 其中[min_index，max_index]范围不会远大于顶点数量。
 *
 * 如果范围太大（例如，带有索引{0，1，10000}的一个三角形），
 * 则通过转换模块将每个顶点属性上传到一个顶点缓冲区中，然后在此过程中将索引绘制调用转换为非索引绘制调用。这会为转换部分增加额外的复杂性，但它可以防止不良应用程序降低帧率。
 *
 *
 * 如果没有需要处理的情况，它会将每个命令转发到驱动程序。
 * 该模块还具有自己的顶点元素状态的CSO缓存。
 */
```

这段注释描述了一个模块，它在 radeonsi 驱动中负责上传用户缓冲区，并根据硬件兼容性对不兼容的顶点缓冲区进行转换。它使用位掩码来优化缓冲区和顶点元素的标志管理，以避免不必要的循环。

该模块将顶点元素分为三个类别进行处理：每个顶点属性、实例化的属性和常量属性。它使用两个主要部分来实现功能：转换部分和用户缓冲区上传部分。

转换部分（`u_vbuf_translate_begin/end`）负责将不兼容的顶点从一个缓冲区转换到另一个缓冲区，以使它们适用于硬件。每个类别的顶点会被翻译到不同的缓冲区中。

用户缓冲区上传部分（`u_vbuf_upload_buffers`）将用户指定的顶点数据上传到缓冲区中。对于大范围的上传，它会使用转换模块将数据重新排列。

总之，这个模块在 radeonsi 驱动中用于优化顶点数据的上传和转换，以提高渲染性能。它将复杂的逻辑分成了转换和上传两个部分，并通过使用位掩码来管理标志，提高了效率。



### 顶点属性

顶点属性的枚举定义在gl_vertex_attrib。

以下是对每个枚举常量的解释：

- `VERT_ATTRIB_POS`：顶点位置属性索引。
- `VERT_ATTRIB_NORMAL`：法线属性索引。
- `VERT_ATTRIB_COLOR0`：颜色0属性索引。
- `VERT_ATTRIB_COLOR1`：颜色1属性索引。
- `VERT_ATTRIB_FOG`：雾属性索引。
- `VERT_ATTRIB_COLOR_INDEX`：颜色索引属性索引。
- `VERT_ATTRIB_EDGEFLAG`：边缘标志属性索引。
- `VERT_ATTRIB_TEX0` 到 `VERT_ATTRIB_TEX7`：纹理坐标0到7的属性索引。
- `VERT_ATTRIB_POINT_SIZE`：点大小属性索引。
- `VERT_ATTRIB_GENERIC0` 到 `VERT_ATTRIB_GENERIC15`：通用顶点属性0到15的索引。

`VERT_ATTRIB_MAX` 是一个标志，用于表示属性索引的最大值。

这个枚举类型在OpenGL中用于标识顶点属性的索引，使开发者能够在着色器中访问不同类型的顶点数据。每个属性索引对应于不同的顶点属性，如位置、法线、颜色等。

#
