
## VBO创建流程

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
整个流程基本就是如果绑定目标为GL_ARRAY_BUFFER,bind_buffer_object中首先会通过_mesa_lookup_bufferobj 找出之前glGenBuffers产生的gl_buffer_object .
然后将之绑定到gl_context的Array中的ArrayBufferObj中。

```
glBufferData
	get_buffer
		get_buffer_target
	_mesa_buffer_data
		buffer_data_error
			buffer_data
				_mesa_buffer_unmap_all_mappings
				st_bufferobj_data
					bufferobj_data
						si_buffer_create
						pipe_buffer_write
							si_buffer_subdata
								si_buffer_transfer_map
									si_buffer_map_sync_with_rings
	
```

当 MESA_VERBOSE 设置了state时 ，可以看到flush vertices.
### bo的创建 
```c
static ALWAYS_INLINE GLboolean
bufferobj_data(struct gl_context *ctx,
               GLenum target,
               GLsizeiptrARB size,
               const void *data,
               struct gl_memory_object *memObj,
               GLuint64 offset,
               GLenum usage,
               GLbitfield storageFlags,
               struct gl_buffer_object *obj)
{
	
	...

   if (ST_DEBUG & DEBUG_BUFFER) {
      debug_printf("Create buffer size %" PRId64 " bind 0x%x\n",
                   (int64_t) size, bindings);
   }

  ...

      else {
         st_obj->buffer = screen->resource_create(screen, &buffer);

         if (st_obj->buffer && data)
            pipe_buffer_write(pipe, st_obj->buffer, 0, size, data);
      }
	  
	  ...
}

```
在bufferobj_data中， `export ST_DEBUG=buffer`可以看到 `create buffer....`输出。

bufferobj_data 会调用驱动层接口si_buffer_create进行实际bo的创建

### 顶点数据的拷贝

在si_buffer_subdata中， 首先通过si_buffer_transfer_map amdgpu_bo_cpu_map得到map 地址后，通过memcpy拷贝顶点数组的数据到这个地址中。

```
static void si_buffer_subdata(struct pipe_context *ctx,
			      struct pipe_resource *buffer,
			      unsigned usage, unsigned offset,
			      unsigned size, const void *data)
{
	struct pipe_transfer *transfer = NULL;
	struct pipe_box box;
	uint8_t *map = NULL;

	u_box_1d(offset, size, &box);
	map = si_buffer_transfer_map(ctx, buffer, 0,
				       PIPE_TRANSFER_WRITE |
				       PIPE_TRANSFER_DISCARD_RANGE |
				       usage,
				       &box, &transfer);
	if (!map)
		return;

	memcpy(map, data, size);
	si_buffer_transfer_unmap(ctx, transfer);
}

```

### 顶点数据的下发

顶点数据是在flush的时候调用 si_vertex_buffers_begin_new_cs进行下发的。
如果开启ddebug则是在si_draw_vbo完成之后，在dd_after_draw里面触发flush;如果未开启ddebug则是在glutSwapBuffers触发, glutSwapBuffers实则调用了glFlush。
```
(glutSwapBuffers)
_mesa_Flush
	_mesa_flush
		st_glFlush
			st_flush
				st_flush_from_st
					si_flush_gfx_cs
						si_begin_new_gfx_cs
							si_all_descriptors_begin_new_cs
								si_vertex_buffers_begin_new_cs
```
							

```
/* VERTEX BUFFERS */

static void si_vertex_buffers_begin_new_cs(struct si_context *sctx)
{
	int count = sctx->vertex_elements ? sctx->vertex_elements->count : 0;
	int i;

	for (i = 0; i < count; i++) {
		int vb = sctx->vertex_elements->vertex_buffer_index[i];

		if (vb >= ARRAY_SIZE(sctx->vertex_buffer))
			continue;

		// resouce保存的是bo的申请地址
		if (!sctx->vertex_buffer[vb].buffer.resource)
			continue;

		//加入此次submit_ib使用到的bo列表，用RADEON_PRIO_VERTEX_BUFFER 标志
		radeon_add_to_buffer_list(sctx, sctx->gfx_cs,
				      r600_resource(sctx->vertex_buffer[vb].buffer.resource),
				      RADEON_USAGE_READ, RADEON_PRIO_VERTEX_BUFFER);
	}

	if (!sctx->vb_descriptors_buffer)
		return;
	radeon_add_to_buffer_list(sctx, sctx->gfx_cs,
				  sctx->vb_descriptors_buffer, RADEON_USAGE_READ,
				  RADEON_PRIO_DESCRIPTORS);
}



```
至此bo数据下发结束。


调试日志

首先在bufferobj_data创建bffer打印地址可以看到
创建的虚拟地址为0x100000c00，创由于采用了slab分配,创建大小32字节小于128KB，同时累积并未超出上次分配的bo大小  ， 所以分配的bo地址并不是直接用drm进行申请的，而是上次分配的bo中的一块。 在amdgpu_winsys_bo->u.slab.real可以看到实际va地址为0x100000000,这一点可以从Buffer list输出可以看到。这个地址分配了 Ib, shader_binary， vertex_buffer相关bo数据.

## VAO 的创建流程

```
glGenVertexArrays
		gen_vertex_arrays_err
			gen_vertex_arrays
				_mesa_new_vao
					_mesa_initialize_vao

```

_mesa_initialize_vao 首先对nomal,color1, fog, color_index,edgeflag, point_size 进行不同维度的初始化， 而对其他的属性都设置为4个分量。并将各个属性的vbo绑定初始化无vbo的buffe对象NullBufferObj。	

```
glBindVertexArrays
	_mesa_BindVertexArray
		bind_vertex_array
			_mesa_lookup_vao
			_mesa_set_draw_vao
				vao->NewArrays? _mesa_update_vao_derived_arrays		
				_mesa_set_varying_vp_inputs


```
_mesa_set_draw_vao 会设置_DrawVAO和净启用的数组。
```
void
_mesa_set_draw_vao(struct gl_context *ctx, struct gl_vertex_array_object *vao,
                   GLbitfield filter)
{
   struct gl_vertex_array_object **ptr = &ctx->Array._DrawVAO;
   bool new_array = false;
   if (*ptr != vao) {
      _mesa_reference_vao_(ctx, ptr, vao);

      new_array = true;
   }

   if (vao->NewArrays) {
      _mesa_update_vao_derived_arrays(ctx, vao);
      vao->NewArrays = 0;

      new_array = true;
   }

   /* May shuffle the position and generic0 bits around, filter out unwanted */
   const GLbitfield enabled = filter & _mesa_get_vao_vp_inputs(vao);
   if (ctx->Array._DrawVAOEnabledAttribs != enabled)
      new_array = true;

   if (new_array)
      ctx->NewDriverState |= ctx->DriverFlags.NewArray;

   ctx->Array._DrawVAOEnabledAttribs = enabled;
   _mesa_set_varying_vp_inputs(ctx, enabled);
}


```

* VAO的数据结构gl_vertex_array_object

```
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


````
* vao->_Enabled 位掩码会根据存储在 vao->_AttributeMapMode 中的 position/generic0 进行转换。
* mesa使用_Enabled这个位数组来表示哪些属性被启用， 通过u_bit_scan来判断。在_mesa_set_draw_vao中`filter& _mesa_get_vao_vp_inputs`初始化这个_Enabled.这个会在之后的Vertex_element绑定时用到。
* mesa将所有的属性放到一个VertexAttrib的属性数组里面。这样就可以根据属性枚举值获取相关属性在顶点数组的信息。
* 还可以根据gl_array_attributes里的_EffBufferBindingIndex 找到保存在BufferBinding里的相关gl_vertex_buffer_binding相关信息。从而找到bufferobject

### 顶点属性

* 顶点属性分类枚举

```
typedef enum
{
   VERT_ATTRIB_POS,
   VERT_ATTRIB_NORMAL,
   VERT_ATTRIB_COLOR0,
   VERT_ATTRIB_COLOR1,
   VERT_ATTRIB_FOG,
   VERT_ATTRIB_COLOR_INDEX,
   VERT_ATTRIB_EDGEFLAG,
   VERT_ATTRIB_TEX0,
   VERT_ATTRIB_TEX1,
   VERT_ATTRIB_TEX2,
   VERT_ATTRIB_TEX3,
   VERT_ATTRIB_TEX4,
   VERT_ATTRIB_TEX5,
   VERT_ATTRIB_TEX6,
   VERT_ATTRIB_TEX7,
   VERT_ATTRIB_POINT_SIZE,
   VERT_ATTRIB_GENERIC0,
   VERT_ATTRIB_GENERIC1,
   VERT_ATTRIB_GENERIC2,
   VERT_ATTRIB_GENERIC3,
   VERT_ATTRIB_GENERIC4,
   VERT_ATTRIB_GENERIC5,
   VERT_ATTRIB_GENERIC6,
   VERT_ATTRIB_GENERIC7,
   VERT_ATTRIB_GENERIC8,
   VERT_ATTRIB_GENERIC9,
   VERT_ATTRIB_GENERIC10,
   VERT_ATTRIB_GENERIC11,
   VERT_ATTRIB_GENERIC12,
   VERT_ATTRIB_GENERIC13,
   VERT_ATTRIB_GENERIC14,
   VERT_ATTRIB_GENERIC15,
   VERT_ATTRIB_MAX
} gl_vert_attrib;

```

* 顶点属性数组

```c
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
```


* 顶点属性数据结构

```
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

```

## 顶点属性绑定VBO
将index属性相关信息传入通过glVertexAttribPointer*。根据shader的layout来分辨哪些属性启用。

```
glVertexAttribPointer
	_mesa_VertexAttribPointer
		get_array_format
		validate_array_and_format
			validate_array
			validate_array_and_format
				validate_array
				validate_array_format
		update_array 
			_mesa_update_array_format
			_mesa_vertex_attrib_binding
			_mesa_bind_vertex_buffer

```

* validate_array会验证vao,vbo是否存在以及stride是否小于0
* _mesa_vertex_attrib_binding会重置当前vao的顶点属性绑定
update_array会各个顶点的属性绑定到si_context中。
* _mesa_bind_vertex_buffer会设置gl_context->Array.ArrayBufferObj->BufferBinding在 VERT_ATTRIB_GENERIC0+index的 被绑定的gl_vertex_buffer的Offset,Stride


### 顶点属性下发Draw

顶点相关属性通过顶点描述符下发的。


####

radeonsi将顶点相关信息放到si_context中的vertex_element数组。然后根据这个信息顶点描述符相关寄存器写值。

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

```

si_context的这个vertex_element副值 是在实际draw之前在prepare_draw操作中完成的。
```
_mesa_exec_DrawArrays
	_mesa_draw_arrays 
		st_draw_vbo
			prepare_draw
				st_validate_draw
					st_update_array
						set_vertex_attribs
							cso_set_vertex_elements
								v_vbuf_set_vertex_elements
									u_vbuf_set_vertex_elements_internal
										si_create_vertex_elements
										si_bind_vertex_elements

			cso_draw_vbo	
				u_vbuf_draw_vbo
					u_vbuf_set_driver_vertex_buffers
						u_vbuf_set_vertex_buffers
							si_set_vertex_buffers
								si_context_and_resource_size

					si_draw_vbo
						u_vbuf_draw_vbo
					---------------------------

						dd_contex_draw_vbo
						dd_after_draw            dd_context存在
							flush------	
					----------------------------

````


### 描述符配置初始化

描述符配置初始化是在驱动上下文创建的时候进行初始化的。

描述符的初始化是在` si_init_all_descriptors`函数里面进行初始化，  它在si_create_context时候调用。 它主要初始化了si_context中的 si_descriptor，rw_buffers, const_and_shader_buffers变量。

```c
/* Indices into sctx->descriptors, laid out so that gfx and compute pipelines
 * are contiguous: * *  0 - rw buffers *  1 - vertex const and shader buffers *  2 - vertex samplers and images
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

## 顶点描述符上传

具体对USER_DATA_VS*相关寄存器副值按照下列流程

```
si_draw_vbo
	si_upload_vertex_buffer_descriptors
	si_upload_graphics_shader_descriptors
		si_upload_descriptors	
		si_upload_bindless_descriptors
	si_emit_all_states
		si_emit_graphics_shader_pointers
			si_emit_shader_pointer
			si_emit_consecutive_shader_pointers
		si_emit_vs_state
	si_emit_draw_packets

```


关于USER_DATA寄存器地址的定义，从定义上看数量并不是手册上说的15个。

```
#define R_00B130_SPI_SHADER_USER_DATA_VS_0                              0x00B130
#define R_00B134_SPI_SHADER_USER_DATA_VS_1                              0x00B134
#define R_00B138_SPI_SHADER_USER_DATA_VS_2                              0x00B138
#define R_00B13C_SPI_SHADER_USER_DATA_VS_3                              0x00B13C
#define R_00B140_SPI_SHADER_USER_DATA_VS_4                              0x00B140
#define R_00B144_SPI_SHADER_USER_DATA_VS_5                              0x00B144
#define R_00B148_SPI_SHADER_USER_DATA_VS_6                              0x00B148
#define R_00B14C_SPI_SHADER_USER_DATA_VS_7                              0x00B14C
#define R_00B150_SPI_SHADER_USER_DATA_VS_8                              0x00B150
#define R_00B154_SPI_SHADER_USER_DATA_VS_9                              0x00B154
#define R_00B158_SPI_SHADER_USER_DATA_VS_10                             0x00B158
#define R_00B15C_SPI_SHADER_USER_DATA_VS_11                             0x00B15C
#define R_00B160_SPI_SHADER_USER_DATA_VS_12                             0x00B160
#define R_00B164_SPI_SHADER_USER_DATA_VS_13                             0x00B164
#define R_00B168_SPI_SHADER_USER_DATA_VS_14                             0x00B168
#define R_00B16C_SPI_SHADER_USER_DATA_VS_15                             0x00B16C
#define R_00B170_SPI_SHADER_USER_DATA_VS_16                             0x00B170
#define R_00B174_SPI_SHADER_USER_DATA_VS_17                             0x00B174
#define R_00B178_SPI_SHADER_USER_DATA_VS_18                             0x00B178
#define R_00B17C_SPI_SHADER_USER_DATA_VS_19                             0x00B17C
#define R_00B180_SPI_SHADER_USER_DATA_VS_20                             0x00B180
#define R_00B184_SPI_SHADER_USER_DATA_VS_21                             0x00B184
#define R_00B188_SPI_SHADER_USER_DATA_VS_22                             0x00B188
#define R_00B18C_SPI_SHADER_USER_DATA_VS_23                             0x00B18C
#define R_00B190_SPI_SHADER_USER_DATA_VS_24                             0x00B190
#define R_00B194_SPI_SHADER_USER_DATA_VS_25                             0x00B194
#define R_00B198_SPI_SHADER_USER_DATA_VS_26                             0x00B198
#define R_00B19C_SPI_SHADER_USER_DATA_VS_27                             0x00B19C
#define R_00B1A0_SPI_SHADER_USER_DATA_VS_28                             0x00B1A0
#define R_00B1A4_SPI_SHADER_USER_DATA_VS_29                             0x00B1A4
#define R_00B1A8_SPI_SHADER_USER_DATA_VS_30                             0x00B1A8
#define R_00B1AC_SPI_SHADER_USER_DATA_VS_31                             0x00B1AC
#define R_00B1F0_SPI_SHADER_PGM_RSRC2_GS_VS                             0x00B1F0

```
在si_upload_vertex_buffer_descriptors中:
1. 首先使用u_upload_alloc向winsys申请bo, 得到resource地址以及map地址ptr后，将bo地址存入si_context的vb_descriptors_buffer,将ptr存入si_context的vb_descriptors_gpu_list.
2. 然后把bo 设定RADEON_PRIO_DESCRIPTORS priority 加入到此次ib 提交引用的buffer列表中.
3. 根据count计数对每个属性开始对这个ptr地址开始写入4个32位数， 分别是: vbo的虚拟地址gpu_address加上该顶点属性在在vbo的偏移量velems->src_offset。

在si_emit_all_states中
首先会根据si_context中的dirty_atoms确定哪些原子状态需要发射，这里会调用之前描述符初始化的emit注册函数如scissor, si_emit_graphics_shader_pointers

在 si_emit_graphics_shader_pointers 中:
1. si_emit_global_shader_pointers 会设置R_00B130_SPI_SHADER_USER_DATA_VS_0的值为描述符的gpu_address
2. si_emit_consecutive_shader_pointers 会对R_00B13C_SPI_SHADER_USER_DATA_VS_3地址开始写入两个寄存器地址信息， 信息都为描述符的gpu_address, 
3. 如果当sctx->vertex_buffer_pointer_dirty为真时，也就是vbo地址未曾下法时， 开始下发当前vbo地址， 这个地址 通过sctx->vb_descriptors_buffer->gpu_address + sctx->vb_descriptors_offset 得到在si_upload_vertex_buffer_descriptors 中通过u_upload_alloc 获取了这个sctx->vb_descriptors_buffer 和当前的sctx->vb_descriptors_offset。这里使用寄存器为VS_8，VS_8通过SI_VS_NUM_USER_SGPR决定， 这里是8

* 从描述符初始化部分已经知道 sctx->shader_pointers.sh_base[PIPE_SHADER_VERTEX] 寄存器为R_00B130_SPI_SHADER_USER_DATA_VS_0, 而在HAVE_32BIT_POINTERS前提下 SI_SGPR_VS_STATE_BITS的枚举值为 4, 所以设置的实际寄存器其实地址为R_00B130_SPI_SHADER_USER_DATA_VS_4.


返回si_draw_vbo后接着开始发送vs_state ，在si_emit_vs_state 中将si_context中的current_vs_state 写入vs_4 0x140 

关于vs状态可参考
```
/* Fields of driver-defined VS state SGPR. */
/* Clamp vertex color output (only used in VS as VS). */
#define S_VS_STATE_CLAMP_VERTEX_COLOR(x)	(((unsigned)(x) & 0x1) << 0)
#define C_VS_STATE_CLAMP_VERTEX_COLOR		0xFFFFFFFE
#define S_VS_STATE_INDEXED(x)			(((unsigned)(x) & 0x1) << 1)
#define C_VS_STATE_INDEXED			0xFFFFFFFD
#define S_VS_STATE_LS_OUT_PATCH_SIZE(x)		(((unsigned)(x) & 0x1FFF) << 8)
#define C_VS_STATE_LS_OUT_PATCH_SIZE		0xFFE000FF
#define S_VS_STATE_LS_OUT_VERTEX_SIZE(x)	(((unsigned)(x) & 0xFF) << 24)
#define C_VS_STATE_LS_OUT_VERTEX_SIZE		0x00FFFFFF

```
之后在si_draw_vbo内部, si_emit_draw_packets开始写入地址为vs_5,6,7 0xb144 0xb148 0xb14c,写入的值为分别base_index,  draw的start_instance, 以及drawid

base_vertex_-> draw_info.start 

