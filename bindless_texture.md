本文作为shader分析bindless teximage的补充

## 基本知识 from [Bindless_Texture](https://www.khronos.org/opengl/wiki/Bindless_Texture)


这不是任何OpenGL版本的核心特性；目前，它只存在作为一个扩展。这并不是扩展可能普及的原因之一。而是一个实际性的问题。

如果OpenGL 4.4要求支持无绑定纹理，那么只有能够支持无绑定纹理的硬件才能实现符合4.4规范的实现。但并非所有4.3硬件都能处理无绑定纹理。OpenGL ARB决定将这个功能作为一个扩展，使其成为可选的。

它在OpenGL 4.x硬件范围内得到了实施。英特尔的支持较少，但AMD和NVIDIA都有很多可以处理它的硬件。

## 目的

通过使用Uniform Buffer Objects、Shader Storage Buffer Objects和各种其他手段，可以将状态信息传递给着色器，而无需修改任何OpenGL上下文状态。您只需将数据设置到适当的缓冲对象中，然后确保在渲染着色器时绑定这些缓冲对象。

有一些情况下这种通信方式不起作用。具体来说，GLSL中的不透明类型：采样器、图像和原子计数器。所有这些类型都基于在呼叫渲染时绑定到OpenGL上下文中的位置的对象来获取其数据。

这带来了两个性能方面的后果。首先，性能成本是在渲染需要这些纹理/图像的对象之前必须将纹理和图像绑定到上下文。这个过程有固有的成本，每个需要使用不同纹理的对象都必须支付这个成本。

第二个后果是必须发出更多的渲染呼叫。原因是需要为不同的对象切换纹理。如果对象可以仅从内存中获取要使用的纹理（UBOs、SSBOs等），那么可以使用相同的多绘制绘图命令渲染多个对象。实际上，使用间接渲染，可以在GPU上生成这些渲染命令。在理想的情况下，这将使渲染几乎仅仅是一个计算调度操作，然后是一个单一的多绘制间接操作。是的，你仍然需要绑定缓冲区，但你可以在帧开始时进行一次绑定，然后让每个人根据渲染命令提供的状态找到其数据。

只有在着色器可以从内存中的值而不是绑定到上下文的值中获取其纹理时，才能使用这种方法。这就是绑定纹理的目的；允许用户从内存中或通过顶点着色器传递并可能通过渲染管线传递的数据中找到纹理。

## 概述

绑定纹理有点复杂。这代表了这个过程和它使用的概念的一般概述。

### 纹理句柄

绑定纹理的概念基础是能够将纹理对象转换为表示该纹理的整数。可以通过通常的机制将这个整数提供给任何特定的着色器阶段，一旦整数到达它需要到达的地方，它可以被转换回用于纹理提取的采样器。

这个过程中的整数称为句柄，它是一个无符号的64位整数。请注意，像纹理这样的OpenGL对象已经使用整数编号，但绑定句柄与纹理的名称是不同的。

注意：句柄的值可能或可能不是实际的GPU地址。您不应该假设它是，也不应该做出关于它相对于任何其他句柄的值的假设（除了不同纹理将具有不同的句柄值的事实）。将其视为纯粹的引用。
句柄可以仅从纹理对象创建。这样的句柄是使用在提取句柄时设置到纹理对象中的采样器参数来引用纹理。句柄也可以从纹理和采样器对象中创建。从纹理+采样器创建的句柄表示使用该纹理与特定的采样器对象。通过这两个过程之一创建的句柄称为纹理句柄。

最后，句柄还可以从纹理中的特定图像创建。这些称为图像句柄。图像句柄用于Image Load Store操作，不能用于常规采样器访问。同样，图像句柄不能用于图像加载/存储访问。

由上述内容产生的一个结果是，一个对象（纹理或采样器）可以与多个句柄关联。纹理可以使用不同的采样器获取

纹理句柄，或者使用纹理和图像句柄，或者为其存储的多个图像使用多个图像句柄。一个采样器对象可以与多个纹理关联。

一旦任何这样的对象（纹理或采样器）至少有一个与之关联的句柄，该对象的状态立即变为不可变。不能修改存储在纹理或采样器中的任何状态的函数将起作用。请注意，这种不可变性仅适用于状态值；您可以更新这些纹理的存储内容，但不能更新它们的参数。

此外，没有办法撤消这种不可变性。一旦获得与该对象关联的句柄，其状态就永久冻结。

请注意，您仍然可以从具有句柄的纹理创建视图纹理，这些视图将具有可变状态（尽管是不可变存储纹理的可变状态）。

### 驻留性(Residency)

一旦创建了适当的句柄，该句柄在任何程序使用之前都不能使用，直到它被驻留。纹理和图像句柄使用不同的驻留函数，但概念是相同的。

句柄可以保持驻留的时间可以根据您的需求而不断增加。驻留性可以通过单独的函数调用来移除。

注意：句柄的驻留性不影响可以在涉及的纹理/采样器上执行的操作。然而，这并不意味着驻留性是廉价或不重要的。只有对需要驻留的句柄进行维护。

### 使用

绑定纹理使用的基础是GLSL能够将64位无符号整数纹理句柄值转换为采样器或图像变量。因此，这些类型不再是真正的不透明类型，尽管您可以对它们执行的操作仍然非常有限（例如没有算术操作）。

在默认块统一变量中使用的采样器和图像类型可以由句柄而不是绑定点的索引进行填充。这些类型现在也可以作为Shader Stage输入/输出传递（在需要时使用`flat`插值限定符）。它们可以用作顶点属性，它们在OpenGL端被视为64位整数。它们还可以用于各种接口块；基于缓冲区的接口块将它们视为64位整数。

### 安全性

绑定纹理不是安全的。API为程序员提供了更少的机会来确保合理的行为；维护完整性由程序员负责。此外，出现错误的后果更为严重。通常情况下，在OpenGL中，如果做错了什么，您要么会得到OpenGL错误，要么会得到未定义行为。而使用绑定纹理，如果出现错误，GPU可能会崩溃或程序可能会终止。它甚至可能导致整个操作系统崩溃。

需要牢记的事项：

1. 当将整数句柄转换为采样器/图像变量时，采样器/图像的类型必须与句柄匹配。
2. 用于句柄的整数值必须是由句柄API返回的实际句柄，并且在使用时必须处于驻留状态。因此，您不能对它们执行"指针算术"或类似的操作；将其视为纯粹的值，这些值恰好是64位无符号整数。
3. 必须在使用渲染命令之前将句柄设置为驻留状态。

## 句柄创建

纹理句柄是使用`glGetTextureHandleARB`或`glGetTextureSamplerHandleARB`创建的。

```c
GLuint64 glGetTextureHandleARB(GLuint texture​);
GLuint64 glGetTextureSamplerHandleARB(GLuint texture​, GLuint sampler​);
```

这些函数接受纹理对象的名称和可选的采样器对象，以生成一个纹理句柄。对于相同的纹理（或纹理/采样器对）进行多次调用将产生相同的句柄。

一旦为纹理/采样器创建了句柄，它的任何状态都无法更改。对于缓冲区纹理，这包括当前附加到它的缓冲对象（这也意味着您不能为没有附加缓冲区的缓冲区纹理创建句柄）。不仅如此，在这种情况下，缓冲对象本身变为不可变；不能使用`glBufferData`重新分配它。尽管与纹理一样，它的存储仍然可以被映射，并且可以像正常情况下的其他函数一样修改其数据。

### 边框颜色

绑定纹理的边框颜色非常奇怪。用于句柄的适用边框颜色（`glGetTextureHandleARB`的`texture​`中的采样参数或`glGetTextureSamplerHandleARB`的`sampler​`中的采样器）必须是以下值集之一：

对于浮点（包括标准整数）或深度格式：
- (0.0,0.0,0.0,0.0)
- (0.0,0.0,0.0,1.0)
- (1.0,1.0,1.0,0.0)
- (1.0,1.0,1.0,1.0)

对于有符号/无符号颜色或模板格式：
- (0,0,0,0)
- (0,0,0,1)
- (1,1,1,0)
- (1,1,1,1)

任何其他值都会引发错误。

###  图像句柄

图像句柄是使用`glGetImageHandleARB`创建的。

```c
GLuint64 glGetImageHandleARB( GLuint texture​, GLint level​, GLboolean layered​, GLint layer​, GLenum format​ );
```

这些参数的含义与`glBindImageTexture`中的含义相同。

对于每个参数值组合，将返回唯一的句柄，多次使用相同的值将产生相同的句柄。

句柄不能被显式释放。当相关的底层对象被删除时，句柄将自动回收。

### 句柄的驻留性

在句柄可以在无绑定操作中使用之前，必须使与其关联的数据成为驻留状态。要影响句柄的驻留性，请使用以下函数：

```c
void glMakeTextureHandleResidentARB(GLuint64 handle​);
void glMakeImageHandleResidentARB(uint64 handle​, enum access​);
void glMakeTextureHandleNonResidentARB(GLuint64 handle​);
void glMakeImageHandleNonResidentARB(uint64 handle​);
```

对于`glMakeImageHandleResidentARB`，`access​`指定着色器是否将从句柄后面的图像读取、写入或同时进行读取和写入。枚举值为`GL_READ_ONLY`、`GL_WRITE_ONLY`或`GL_READ_WRITE`。如果着色器违反了这一限制（从只写图像中读取或反之亦然），将导致未定义行为，包括可能导致崩溃。或者更糟。

请注意，正在进行驻留的是句柄，而不是纹理。因此，如果您有引用相同纹理存储的句柄，使一个句柄成为驻留的是不足以使用其他句柄的。驻留性影响的不仅仅是图像数据。

从概念上讲，图像数据的驻留意味着它直接存在于GPU可访问的内存中。可供驻留图像/纹理使用的存储空间可能小于可用于纹理的总存储空间。因此，应尽量减少纹理保持驻留的时间。不要尝试在每一帧或其他地方使纹理保持驻留/不保持驻留等步骤。但是，如果您有一段时间不使用纹理，请将其取消保持驻留。


## GLSL 句柄是使用


在所有情况下，当向着色器提供一个纹理的句柄时，该句柄必须是驻留的，也必须与纹理/图像的类型匹配；因此，2D纹理句柄必须与`sampler2D`变量一起使用。

警告：除非支持`NV_gpu_shader5`扩展，否则纹理和图像访问函数必须使用动态统一表达式提供的采样器/图像值。这意味着您不能跨调用组（用于渲染操作的渲染命令）访问不同的纹理。特别是，您不能使每个三角形使用不同的纹理。

### 直接句柄使用

这个功能允许采样器和图像类型直接在更多的着色器接口中使用。采样器和图像类型可以在以下情况下使用：

1. 着色器阶段的输入和输出（除了片段着色器的输出）。
2. 默认块统一变量（不在统一块中的统一变量）。
3. 所有种类的接口块。

当作为顶点属性使用时，句柄通过 `glVertexAttribLPointer` 等函数进行设置，使用 `GL_UNSIGNED_INT64_ARB` 数据类型。当作为其他输入/输出类型使用时（无论是在接口块内还是外部），它们与任何整数类型具有相同的限制：如果它们被插值，它们必须使用 `flat` 插值限定符。

使用缓冲区支持的接口块（UBOs/SSBOs）将采样器和图像类型视为64位整数值。这些值具有8字节对齐，因此使用 `std140`/`std430`布局的此类类型数组具有8字节的数组跨度。因此，以下定义：

```c
uniform samplers
{
  sampler2D arr[10];
};
```

匹配以下C++结构：

```cpp
struct samplers
{
  std::uint64_t arr[10];
};
```

采样器或图像类型的默认块统一变量假定从上下文状态中获取其纹理或图像数据。要允许它们使用句柄，必须使用特殊的布局限定符进行声明：

```c
layout(bindless_sampler) uniform sampler2D bindless;
layout(bindless_image) uniform image2D bindless2;
```

布局限定符的类型（采样器或图像）必须与变量的类型相匹配。如果要使所有这样的统一变量使用绑定句柄，可以全局声明如下：

```c
layout(bindless_sampler) uniform; layout(bindless_image) uniform;
```

`bound_sampler`/`bound_image`限定符存在表示采样器/图像从上下文状态获取其数据。`bound`是默认值。

作为绑定句柄定义的采样器和图像仍然可以使用上下文状态，如果你希望的话。它们使用取决于如何从OpenGL侧设置统一变量。如果像通常一样使用 `glUniform1i`，它将使用上下文状态。

要将句柄传递给这种统一变量，必须使用以下函数之一：

```c
void glUniformHandleui64ARB(GLint location, GLuint64 value);
void glUniformHandleui64vARB(GLint location, GLsizei count, const GLuint64 *value);
void glProgramUniformHandleui64ARB(GLuint program, GLint location, GLuint64 value);
void glProgramUniformHandleui64vARB(GLuint program, GLint location, GLsizei count, const GLuint64 *values);
```

### 句柄作为整数

如果你可以访问 `NV_vertex_attrib_integer_64bit` 或 `ARB_gpu_shader_int64`，则可以在着色器中使用 `uint64_t` 类型。这可以用作输入和输出的整数值（包括由顶点属性提供）。

它们还可以用于接口块。它们的大小为8字节，`std140`/`std430`布局中，它们的对齐方式为8字节。

`uint64_t` 值可以使用构造函数转换为任何采样器或图像类型，例如 `sampler2DArray(some_uint64)`。它们也可以转换回64位整数。

如果 `NV_vertex_attrib_integer_64bit` 和 `ARB_gpu_shader_int64` 不可用，则可以使用 `uvec2` 类型实现相同的效果。第一个分量是整数的最低有效4字节，第二个分量是整数的最高有效4字节。因此，如果在小端机器上存储64位整数，可以直接将其读取为 `uvec2`。

不能将64位顶点属性传递给 `uvec2`，但可以使用相同的数据并将其作为2元素无符号整数向量进行传递。类似地，您可以在接口块中使用 `uvec2` 以传递64位整数。

请注意，使用 `std140` 布局时，`uvec2` 数组将具有与 `vec4` 相同的数组跨度和对齐：16字节。为了避免这种情况，可以将其声明为半尺寸（向上舍入）的 `uvec4` 数组，然后选择所需的两个元素。

与 `uint64_t` 一样，构造函数可以在 `uvec2` 和采样器/图像类型之间进行转换。

### 使用句柄

可以将采样器/图像变量声明为局部变量，并可以从其他采样器/图像变量或通过从整数值进行转换进行初始化。也可以将采样器/图像变量保留未初始化，直到以后。采样器/图像变量可以传递到函数中（但不能用作返回值），但必须确保将函数参数与传递的采样器/图像变量完全相同（包括布局限定符）。

否则，不能对采样器/图像执行任何算术操作。

一旦拥有采样器/图像变量，就可以将其传递给纹理/图像函数并像正常一样使用。



## mesa18实现分析


### 创建

```c
GLuint64 glGetTextureHandleARB(GLuint texture​);
_mesa_GetTextureHandleARB(GLuint texture)
      texObj = _mesa_lookup_texture(ctx, texture);
      if (!_mesa_is_texture_complete(texObj, &texObj->Sampler)) {
      if (!is_sampler_border_color_valid(&texObj->Sampler)) {
      return get_texture_handle(ctx, texObj, &texObj->Sampler);

GLuint64 glGetTextureSamplerHandleARB(GLuint texture​, GLuint sampler​);
_mesa_GetTextureSamplerHandleARB(GLuint texture, GLuint sampler)
      texObj = _mesa_lookup_texture(ctx, texture);
      sampObj = _mesa_lookup_samplerobj(ctx, sampler);
      if (!_mesa_is_texture_complete(texObj, sampObj)) {
      if (!is_sampler_border_color_valid(sampObj)) {

      return get_texture_handle(ctx, texObj, sampObj);

get_texture_handle(ctx, texObj, &texObj->Sampler);
      [return] texHandleObj = find_texhandleobj(texObj, separate_sampler ? sampObj : NULL);

      handle = ctx->Driver.NewTextureHandle(ctx, texObj, sampObj);
      texHandleObj = CALLOC_STRUCT(gl_texture_handle_object);

      /* Store the handle into the texture object. */
      texHandleObj->texObj = texObj;
      texHandleObj->sampObj = separate_sampler ? sampObj : NULL;
      texHandleObj->handle = handle;

      texObj->HandleAllocated = true;

GLuint64 glGetImageHandleARB( GLuint texture​, GLint level​, GLboolean layered​, GLint layer​, GLenum format​ );
_mesa_GetImageHandleARB(GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format)
   if (!_mesa_has_ARB_bindless_texture(ctx) ||
   texObj = _mesa_lookup_texture(ctx, texture);
   return get_image_handle(ctx, texObj, level, layered, layer, format);
        [return] imgHandleObj = find_imghandleobj(texObj, level, layered, layer, format);

        handle = ctx->Driver.NewImageHandle(ctx, &imgObj);
        imgHandleObj = CALLOC_STRUCT(gl_image_handle_object);

        /* Store the handle into the texture object. */
        memcpy(&imgHandleObj->imgObj, &imgObj, sizeof(struct gl_image_unit));
        imgHandleObj->handle = handle;

        texObj->HandleAllocated = true;

        texObj->Sampler.HandleAllocated = true;
        [return] handle

st_NewTextureHandle(struct gl_context *ctx, struct gl_texture_object *texObj,
                    struct gl_sampler_object *sampObj)

      struct pipe_sampler_view *view;
      // for tbo
      view = st_get_texture_sampler_view_from_stobj(st, stObj, sampObj, 0, true);

      // for non-tbo
      view = st_get_buffer_sampler_view_from_stobj(st, stObj);


      return pipe->create_texture_handle(pipe, view, &sampler);


static uint64_t si_create_texture_handle(struct pipe_context *ctx,
					 struct pipe_sampler_view *view,
					 const struct pipe_sampler_state *state)
	uint32_t desc_list[16];
	tex_handle = CALLOC_STRUCT(si_texture_handle);
	memset(desc_list, 0, sizeof(desc_list));
	sstate = ctx->create_sampler_state(ctx, state);
	si_set_sampler_view_desc(sctx, sview, sstate, &desc_list[0]);
	memcpy(&tex_handle->sstate, sstate, sizeof(*sstate));

    
	tex_handle->desc_slot = si_create_bindless_descriptor(sctx, desc_list,
							      sizeof(desc_list));
	handle = tex_handle->desc_slot;


si_create_bindless_descriptor(struct si_context *sctx, uint32_t *desc_list,
			      unsigned size)
	struct si_descriptors *desc = &sctx->bindless_descriptors;
	desc_slot = si_get_first_free_bindless_slot(sctx);
	desc_slot_offset = desc_slot * 16;

	/* Copy the descriptor into the array. */
	memcpy(desc->list + desc_slot_offset, desc_list, size);

	desc_slot_offset = desc_slot * 16;
	if (!si_upload_descriptors(sctx, desc))


si_upload_descriptors (,...)
    // direct bind for const_buf
	if ((int)desc->first_active_slot == desc->slot_index_to_bind_directly &&
	    desc->num_active_slots == 1) 
        ...

    // upload descriptor
	uint32_t *ptr;
	u_upload_alloc(sctx->b.const_uploader, first_slot_offset, upload_size,
		       si_optimal_tcc_alignment(sctx, upload_size),
		       &buffer_offset, (struct pipe_resource**)&desc->buffer,
		       (void**)&ptr);
	util_memcpy_cpu_to_le32(ptr, (char*)desc->list + first_slot_offset,
				upload_size);
```

* GetTextureSamplerHandleARB内部流程位查找纹理对象采样对象校验错误，相比glGetTextureHandleARB 多了查找采样对象的步骤，最后调用st层get_texture_handle获取句柄
* 在get_texture_handle内，首先插值句柄是否存在，不存在调用stack_tracker获取句柄
* 在st_NewTextureHandle内， 对tbo和非tbo获取纹理预览与纹理分析流程一致， 最后调用实际驱动获取句柄
* 在si_create_texture_handle 内，

```
struct si_texture_handle
{
	unsigned			desc_slot;
	bool				desc_dirty;
	struct pipe_sampler_view	*view;
	struct si_sampler_state		sstate;
};
```
* 首先构建si_texture_handle结构，调用si_create_sampler_view 初始化view, 调用si_create_sampler_state 初始化state. 两个接口同纹理分析,只不过这次没有通过设置驱动状态来更新.

* 在si_create_bindless_descriptor 内，使用16dword 大小slot, 找到空闲slot后调用si_upload_descriptors上传描述符
* si_upload_descriptors 内， 如果当前只有一个写入的描述符，直接写入desc->list中，只适用constbuf，直接绑定表示该slot就可表示一个实际描述符资源.
* 否则向winsys申请bo， 将在si_create_bindless_descriptor复制写入的samplerview,state描述符数据写入bo映射的ptr中,上传完成

### 驻留

```c
void glMakeTextureHandleResidentARB(GLuint64 handle​);
    _mesa_MakeTextureHandleResidentARB_no_error(GLuint64 handle)
        texHandleObj = lookup_texture_handle(ctx, handle);
        make_texture_handle_resident(ctx, texHandleObj, true);

void glMakeTextureHandleNonResidentARB(GLuint64 handle​);
    _mesa_MakeTextureHandleNonResidentARB_no_error(GLuint64 handle)
        texHandleObj = lookup_texture_handle(ctx, handle);
        make_texture_handle_resident(ctx, texHandleObj, false);

void glMakeImageHandleResidentARB(uint64 handle​, enum access​);  
    _mesa_MakeImageHandleResidentARB_no_error(GLuint64 handle, GLenum access)
        imgHandleObj = lookup_image_handle(ctx, handle);
        make_image_handle_resident(ctx, imgHandleObj, access, true);

void glMakeImageHandleNonResidentARB(uint64 handle​);
    _mesa_MakeImageHandleResidentARB_no_error(GLuint64 handle, GLenum access)
   imgHandleObj = lookup_image_handle(ctx, handle);
   make_image_handle_resident(ctx, imgHandleObj, access, true);

make_texture_handle_resident(struct gl_context *ctx,
                             struct gl_texture_handle_object *texHandleObj,
                             bool true)
-----------------------------------------------------------------
      ctx->Driver.MakeTextureHandleResident(ctx, handle, GL_TRUE);
         pipe->make_texture_handle_resident(ctx->pipe, handle, 1);

            // entry = sctx->tex_handles[handle]
	        tex_handle = (struct si_texture_handle *)entry->data;
	        sview = (struct si_sampler_view *)tex_handle->view;

            //for non-tbo
			si_update_bindless_texture_descriptor(sctx, tex_handle);
	            uint32_t desc_list[16];
              	memcpy(desc_list, desc->list + desc_slot_offset, sizeof(desc_list));

              	si_set_sampler_view_desc(sctx, sview, &tex_handle->sstate,
              				 desc->list + desc_slot_offset);

	            if (memcmp(desc_list, desc->list + desc_slot_offset,
                    tex_handle->desc_dirty = true;
                    sctx->bindless_descriptors_dirty = true;

            // for tbo
			si_update_bindless_buffer_descriptor(sctx,
							     tex_handle->desc_slot,
							     sview->base.texture,
							     sview->base.u.buf.offset,
							     &tex_handle->desc_dirty);
                si_set_buf_desc_address(buf, offset, &desc_list[0]);
		        if (tex_handle->desc_dirty)
                    sctx->bindless_descriptors_dirty = true;

make_texture_handle_resident(struct gl_context *ctx,
                             struct gl_texture_handle_object *texHandleObj,
                             bool false)
   _mesa_hash_table_u64_remove(ctx->ResidentTextureHandles, handle);

   ctx->Driver.MakeTextureHandleResident(ctx, handle, GL_FALSE);
       pipe->make_texture_handle_resident(pipe, handle, 0 );
           	/* Remove the texture handle from the per-context list. */
	     	util_dynarray_delete_unordered(&sctx->resident_tex_handles,
	     				       struct si_texture_handle *,
	     				       tex_handle);

	     	if (sview->base.texture->target != PIPE_BUFFER) {
	     		util_dynarray_delete_unordered(
	     			&sctx->resident_tex_needs_depth_decompress,
	     			struct si_texture_handle *, tex_handle);

	     		util_dynarray_delete_unordered(
	     			&sctx->resident_tex_needs_color_decompress,
	     			struct si_texture_handle *, tex_handle);
	     	}
   texObj = texHandleObj->texObj;
   _mesa_reference_texobj(&texObj, NULL);

   if (texHandleObj->sampObj) {
      sampObj = texHandleObj->sampObj;
      _mesa_reference_sampler_object(ctx, &sampObj, NULL);
   }


make_image_handle_resident(struct gl_context *ctx,
                           struct gl_image_handle_object *imgHandleObj,
                           GLenum access, bool resident)
   GLuint64 handle = imgHandleObj->handle;
   ctx->Driver.MakeImageHandleResident(ctx, handle, access, GL_TRUE);
       st_MakeImageHandleResident(struct gl_context *ctx, GLuint64 handle,
           pipe->make_image_handle_resident(ctx->pipe, handle, access, resident);
                si_make_image_handle_resident(struct pipe_context *ctx, uint64_t handle, unsigned access, bool resident)


si_make_image_handle_resident

resident=GL_TRUE:
	img_handle = (struct si_image_handle *)entry->data;
	view = &img_handle->view;
	res = r600_resource(view->resource);

    // for non-tbo
    struct si_texture *tex = (struct si_texture *)res;
    si_update_bindless_image_descriptor(sctx, img_handle);

    // for tbo
	si_update_bindless_buffer_descriptor(sctx,
	   			     img_handle->desc_slot,
	   			     view->resource,
	   			     view->u.buf.offset,
	   			     &img_handle->desc_dirty);
resident=GL_FALSE
	/* Remove the image handle from the per-context list. */
		util_dynarray_delete_unordered(&sctx->resident_img_handles,
					       struct si_image_handle *,
					       img_handle);

		if (res->b.b.target != PIPE_BUFFER) {
			util_dynarray_delete_unordered(
				&sctx->resident_img_needs_color_decompress,
				struct si_image_handle *,
				img_handle);
		}



static void si_update_bindless_image_descriptor(struct si_context *sctx,
						struct si_image_handle *img_handle)
	struct si_descriptors *desc = &sctx->bindless_descriptors;
	unsigned desc_slot_offset = img_handle->desc_slot * 16;
	struct pipe_image_view *view = &img_handle->view;
	uint32_t desc_list[8];

    // 用来判断bindless是否需要更新
	memcpy(desc_list, desc->list + desc_slot_offset,
	       sizeof(desc_list));

	si_set_shader_image_desc(sctx, view, true,
				 desc->list + desc_slot_offset, NULL);

si_set_shader_image_desc(...)

    res = r600_resource(view->resource);
    // for tbo
	si_make_buffer_descriptor(screen, res,
				  view->format,
				  view->u.buf.offset,
				  view->u.buf.size, desc);
	si_set_buf_desc_address(res, view->u.buf.offset, desc + 4);
    
    // for texture
    struct si_texture *tex = (struct si_texture *)res;
    si_make_texture_descriptor(screen, tex,
		   false, res->b.b.target,
		   view->format, swizzle,
		   hw_level, hw_level,
		   view->u.tex.first_layer,
		   view->u.tex.last_layer,
		   width, height, depth,
		   desc, fmask_desc);
	si_set_mutable_tex_desc_fields(screen, tex,
		   &tex->surface.u.legacy.level[level],
		   level, level,
		   util_format_get_blockwidth(view->format),
		   false, desc);
	
```

* MakeTexture(Image)HandleResident作用就是更新bindless buffer中的sampler_view和sampler,image, 如果需要更新设定脏位sctx->bindless_descriptors_dirty = true;这样draw时才会下发数据
* 而MakeTexture(Image)HandleNonResidentARB 则是移除sctx->resident_tex(img)_handles 中的tex(img)_handle.
* 其他si_set_shader_image_desc和纹理分析一致


### 绑定句柄

```c
void glUniformHandleui64ARB(GLint location, GLuint64 value);
    _mesa_UniformHandleui64ARB(GLint location, GLuint64 value)
       _mesa_uniform_handle(location, 1, &value, ctx, ctx->_Shader->ActiveProgram);
          uni = shProg->UniformRemapTable[location];
          memcpy(&uni->storage[size_mul * components * offset], values,
                 sizeof(uni->storage[0]) * components * count * size_mul);


          _mesa_flush_vertices_for_uniforms
                new_driver_state |= ctx->DriverFlags.NewShaderConstants[index];
                FLUSH_VERTICES(ctx, new_driver_state ? 0 : _NEW_PROGRAM_CONSTANTS);
          _mesa_propagate_uniforms_to_driver_storage(uni, offset, count);



void glUniformHandleui64vARB(GLint location, GLsizei count, const GLuint64 *value);
    _mesa_UniformHandleui64vARB(GLint location, GLsizei count,
                                const GLuint64 *value)
       _mesa_uniform_handle(location, count, value, ctx,
                        ctx->_Shader->ActiveProgram);

void glProgramUniformHandleui64ARB(GLuint program, GLint location, GLuint64 value);
   struct gl_shader_program *shProg =
      _mesa_lookup_shader_program_err(ctx, program,
            "glProgramUniformHandleui64ARB");
   _mesa_uniform_handle(location, 1, &value, ctx, shProg);

void glProgramUniformHandleui64vARB(GLuint program, GLint location, GLsizei count, const GLuint64 *values);
   struct gl_shader_program *shProg =
      _mesa_lookup_shader_program_err(ctx, program,
            "glProgramUniformHandleui64vARB");
   _mesa_uniform_handle(location, count, values, ctx, shProg);

```

* 四个接口实际上都是调用_mesa_uniform_handle 进行uniform更新
* 在`_mesa_uniform_handle`内， 在程序link之后， 会将程序使用到的unform存入UniformRemapTable， 这样glUniform就可以对指定location位置进行更新数据写入，这由`_mesa_propagate_uniforms_to_driver_storage`完成

* `_mesa_flush_vertices_for_uniforms`会根据每个shader是否使用uniform设定驱动状态ST_NEW_VS{TCS*}_CONSTANTS

* 对于bindless_texture来说，mesa内部视为一个CONST常量而非像其他绑定纹理一样当作sampler类型，这直接影响了后续shader对纹理内建函数对这个纹理的处理

glUiform*即设定状态ST_NEW_VS_CONSTANTS之后

```c
st_update_vs_constants(struct st_context *st)
   st_upload_constants(st, &st->vp->Base);

       // exist boundBindless*
       /* Make all bindless samplers/images bound texture/image units resident in
        * the context.
        */
       st_make_bound_samplers_resident(st, prog);
       st_make_bound_images_resident(st, prog);

       // for sampler
       sampler->bound = true;
       sh->Program->sh.HasBoundBindlessSampler = true;

       // for image
       image->unit = value;
       image->bound = true;
       sh->Program->sh.HasBoundBindlessImage = true;

        /* update constants */
       if (params && params->NumParameters) {

            struct pipe_constant_buffer cb;
            cb.buffer = NULL;
            cb.user_buffer = params->ParameterValues;
            cb.buffer_offset = 0;
            cb.buffer_size = paramBytes;

            cso_set_constant_buffer(st->cso_context, shader_type, 0, &cb);
                pipe->set_constant_buffer(pipe, shader_stage, index, cb);
                    si_pipe_set_constant_buffer
       }

static void si_pipe_set_constant_buffer(struct pipe_context *ctx,
					enum pipe_shader_type shader, uint slot,
					const struct pipe_constant_buffer *input)

	slot = si_get_constbuf_slot(slot);
	si_set_constant_buffer(sctx, &sctx->const_and_shader_buffers[shader],
			       si_const_and_shader_buffer_descriptors_idx(shader),
			       slot, input);


static void si_set_constant_buffer(struct si_context *sctx,
				   struct si_buffer_resources *buffers,
				   unsigned descriptors_idx,
				   uint slot, const struct pipe_constant_buffer *input)

	if (input && (input->buffer || input->user_buffer)) {
		struct pipe_resource *buffer = NULL;
		uint64_t va;

		/* Upload the user buffer if needed. */
		if (input->user_buffer) {
			si_upload_const_buffer(sctx,
					       (struct r600_resource**)&buffer, input->user_buffer,
					       input->buffer_size, &buffer_offset);
			va = r600_resource(buffer)->gpu_address + buffer_offset;
        }
    } else {
			pipe_resource_reference(&buffer, input->buffer);
			va = r600_resource(buffer)->gpu_address + input->buffer_offset;
			/* Only track usage for non-user buffers. */
			r600_resource(buffer)->bind_history |= PIPE_BIND_CONSTANT_BUFFER;
	}
 	/* Set the descriptor. */
    uint32_t *desc = descs->list + slot*4;
    desc[0] = va;
    desc[1] = S_008F04_BASE_ADDRESS_HI(va >> 32) |
    	  S_008F04_STRIDE(0);
    desc[2] = input->buffer_size;
    ....


st_make_bound_samplers_resident(st, prog);
   struct st_bound_handles *bound_handles = &st->bound_texture_handles[shader];

   st_destroy_bound_texture_handles_per_stage(st, shader);
   [loop prog->sh.NumBindlessSamplers]
      struct gl_bindless_sampler *sampler = &prog->sh.BindlessSamplers[i];
      handle = st_create_texture_handle_from_unit(st, prog, sampler->unit);
          st_update_single_texture(st, &view, texUnit, prog->sh.data->Version >= 130, true);
          return pipe->create_texture_handle(pipe, view, &sampler);
      pipe->make_texture_handle_resident(st->pipe, handle, true);
      *(uint64_t *)sampler->data = handle;
      bound_handles->handles[bound_handles->num_handles] = handle;

st_make_bound_images_resident(st, prog);
   struct st_bound_handles *bound_handles = &st->bound_image_handles[shader];

   st_destroy_bound_image_handles_per_stage(st, shader);
   [loop prog->sh.NumBindlessImages]
      struct gl_bindless_image *image = &prog->sh.BindlessImages[i];
      handle = st_create_image_handle_from_unit(st, prog, image->unit);
      pipe->make_image_handle_resident(st->pipe, handle, GL_READ_WRITE, true);
      *(uint64_t *)image->data = handle;
      bound_handles->handles[bound_handles->num_handles] = handle;


static bool si_upload_descriptors(struct si_context *sctx,
				  struct si_descriptors *desc)
{
	unsigned slot_size = desc->element_dw_size * 4;
	unsigned first_slot_offset = desc->first_active_slot * slot_size;
	unsigned upload_size = desc->num_active_slots * slot_size;

	/* If there is just one active descriptor, bind it directly. */
	if ((int)desc->first_active_slot == desc->slot_index_to_bind_directly &&
	    desc->num_active_slots == 1) {
		uint32_t *descriptor = &desc->list[desc->slot_index_to_bind_directly *
						   desc->element_dw_size];

		/* The buffer is already in the buffer list. */
		r600_resource_reference(&desc->buffer, NULL);
		desc->gpu_list = NULL;
		desc->gpu_address = si_desc_extract_buffer_address(descriptor);
		si_mark_atom_dirty(sctx, &sctx->atoms.s.shader_pointers);
		return true;
	}

```


* 对于st_make_bound_images_resident 和st_make_bound_samplers_resident 只处理glsl中显式绑定句柄的纹理或图像
* 之前说道，bindless_texture作为常量存在，所以再si_set_constant_buffer内部申请bo保存这个常量，并将bo地址写入对应槽位的描述符.
* 从调用来看st_make_bound_images_resident内部只是重复了MakeImageHandleResident等操作，所以对于显式绑定,应该无需调用glMakeImageHandleResidentARB
* 再si_upload_descriptors内， 首先对constbuf是否是1作了特殊处理，由于constbuf保存在const_and_shader_buffer的slot16...位置， 当只有一个consttbuf的时候，通过si_desc_extract_buffer_address获取由si_set_constant_buffer 设置的常量bo地址，然后将const_and_shader_buffer的gpu_address设置成这个，这样在vs_2寄存器指向的地址即为实际const_buf slot0实际bo地址而非shaderbuffer slot0 bo 地址


### 关于shader转ir对bindless的处理


```
lp_build_tgsi_llvm(
   struct lp_build_tgsi_context * bld_base,
   const struct tgsi_token *tokens)
   // 获取tgsi
   ....

   // 翻译tgsi实际指令到llvm ir
   while (bld_base->pc != -1) {
      const struct tgsi_full_instruction *instr =
         bld_base->instructions + bld_base->pc;
      if (!lp_build_tgsi_inst_llvm(bld_base, instr)) {
   }


lp_build_tgsi_inst_llvm(
   struct lp_build_tgsi_context * bld_base,
   const struct tgsi_full_instruction * inst)

   struct lp_build_emit_data emit_data;
 
   /* Emit the instructions */ for  TGSI_*_TEX
   emit_data.chan = 0;
   action->emit(action, bld_base, &emit_data);
       build_tex_intrinsic(const struct lp_build_tgsi_action *action,
   

static void build_tex_intrinsic(const struct lp_build_tgsi_action *action,
				struct lp_build_tgsi_context *bld_base,
				struct lp_build_emit_data *emit_data)

	struct ac_image_args args = {};
	LLVMValueRef fmask_ptr = NULL;
	tex_fetch_ptrs(bld_base, emit_data, &args.resource, &args.sampler, &fmask_ptr);


    // for tbo
    LLVMValueRef vindex = lp_build_emit_fetch(bld_base, inst, 0, TGSI_CHAN_X);
    LLVMValueRef result = ac_build_buffer_load_format(&ctx->ac,
    emit_data->output[emit_data->chan] = ac_build_expand_to_vec4(&ctx->ac, result, num_channels);
    return;

    // for non-tbo
	/* Fetch and project texture coordinates */
	args.coords[3] = lp_build_emit_fetch(bld_base, inst, 0, TGSI_CHAN_W);

static void tex_fetch_ptrs(struct lp_build_tgsi_context *bld_base,
			   struct lp_build_emit_data *emit_data,
			   LLVMValueRef *res_ptr, LLVMValueRef *samp_ptr,
			   LLVMValueRef *fmask_ptr)
	LLVMValueRef list = LLVMGetParam(ctx->main_fn, ctx->param_samplers_and_images);
	const struct tgsi_full_instruction *inst = emit_data->inst;
	unsigned target = inst->Texture.Texture;
    index = LLVMConstInt(ctx->i32,si_get_sampler_slot(reg->Register.Index), 0);

  
    // change list and index to bindless for bindless 
	if (reg->Register.File != TGSI_FILE_SAMPLER) {
		list = LLVMGetParam(ctx->main_fn,
				    ctx->param_bindless_samplers_and_images);
		index = lp_build_emit_fetch_src(bld_base, reg,
						TGSI_TYPE_UNSIGNED, 0);

		/* Since bindless handle arithmetic can contain an unsigned integer
		 * wraparound and si_load_sampler_desc assumes there isn't any,
		 * use GEP without "inbounds" (inside ac_build_pointer_add)
		 * to prevent incorrect code generation and hangs.
		 */
		index = LLVMBuildMul(ctx->ac.builder, index, LLVMConstInt(ctx->i32, 2, 0), "");
		list = ac_build_pointer_add(&ctx->ac, list, index);
		index = ctx->i32_0;

    }

    // for tbo
    *res_ptr = si_load_sampler_desc(ctx, list, index, AC_DESC_BUFFER);

    // for non-tbo
    *res_ptr = si_load_sampler_desc(ctx, list, index, AC_DESC_IMAGE);

    ....

lp_build_emit_fetch_src(
   struct lp_build_tgsi_context *bld_base,
   const struct tgsi_full_src_register *reg,
   enum tgsi_opcode_type stype,
   const unsigned chan_index)

   if (bld_base->emit_fetch_funcs[reg->Register.File]) {
      res = bld_base->emit_fetch_funcs[reg->Register.File](bld_base, reg, stype,
                                                           swizzle);
             fetch_constant(...)


static LLVMValueRef fetch_constant(
	struct lp_build_tgsi_context *bld_base,
	const struct tgsi_full_src_register *reg,
	enum tgsi_opcode_type type = type=TGSI_TYPE_UNSIGNED,
	unsigned swizzle_in = 0)

	unsigned swizzle = swizzle_in & 0xffff;
	idx = reg->Register.Index * 4 + swizzle;

    addr = LLVMConstInt(ctx->i32, idx * 4, 0);

	/* Fast path when user data SGPRs point to constant buffer 0 directly. */
	if (sel->info.const_buffers_declared == 1 &&
	    sel->info.shader_buffers_declared == 0) {

		LLVMValueRef desc = load_const_buffer_desc_fast_path(ctx);
	        LLVMValueRef ptr = LLVMGetParam(ctx->main_fn, ctx->param_const_and_shader_buffers);
	        ptr = LLVMBuildPtrToInt(ctx->ac.builder, ptr, ctx->ac.intptr, "");
	        LLVMValueRef desc0, desc1;

        	if (HAVE_32BIT_POINTERS) {
        		desc0 = ptr;
        		desc1 = LLVMConstInt(ctx->i32,
        				     S_008F04_BASE_ADDRESS_HI(ctx->screen->info.address32_hi), 0);
            }
	        LLVMValueRef desc_elems[] = { desc0, desc1,...};
            return  desc_elems;
            
        //* Load a dword from a constant buffer.
		LLVMValueRef result = buffer_load_const(ctx, desc, addr);
            ac_build_buffer_load(struct ac_llvm_context *ctx,
			    result[i] = ac_build_intrinsic(ctx, "llvm.SI.load.const.v4i32", -

		return bitcast(bld_base, type, result);
	}


LLVMValueRef si_load_sampler_desc(struct si_shader_context *ctx,
				  LLVMValueRef list, LLVMValueRef index,
				  enum ac_descriptor_type type = AC_DESC_IMAGE)
	switch (type) {
	    case AC_DESC_IMAGE:
	    	/* The image is at [0:7]. */
	    	index = LLVMBuildMul(builder, index, LLVMConstInt(ctx->i32, 2, 0), "");
	    	break;
    }
	return ac_build_load_to_sgpr(&ctx->ac, list, index);
```


*  流程一目了然， 首先调用lp_build_tgsi_llvm 开始构建llvm ir , 在内部调用lp_build_tgsi_inst_llvm 翻译main函数指令 ,对于此时测试用列， tex函数为texture,所以调用TGSI_TEX注册的处理函数build_tex_intrinsic.
* build_tex_intrinsic 通过tex_fetch_ptrs 获取参数gsampler的实际句柄地址
* 进入tex_fetch_ptrs,首先初始化listsamplers_and_images， 而对于bindless则更改为bindless_samplers_and_images地址，通过lp_build_emit_fetch_src 获取实际句柄对应的槽位地址
* lp_build_emit_fetch_src 对tgsi_file类型发射回调函数，对于常量则时fetch_constant.
* 对于只有一个const_buf槽位，无sbo时， 调用load_const_buffer_desc_fast_path 加载描述符地址，此时该buffer就相当于一个描述符，所以可直接作为va地址使用.
* 在load_const_buffer_desc_fast_path 内， 首先取参数const_and_shader_buffers 指针，从之前shader分析此时指针类型为f32\*, 所以使用LLVMBuildPtrToInt 转成intptr, 得到描述符写入地址，再用过buffer_load_const 调用内建buffer_load函数获取资源， 如此bindless 句柄再shader的使用分析完成

### 测试
使用piglit的basic-texture2D.shader_test 进行测试, 上面分析纹理及constbuf=1 对应与这个shader.

```
# In this test, just perform a texture lookup with a resident texture.
[require]
GL >= 3.3 
GLSL >= 3.30
GL_ARB_bindless_texture

[vertex shader passthrough]

[fragment shader]
#version 330
#extension GL_ARB_bindless_texture: require

layout (bindless_sampler) uniform sampler2D tex;
out vec4 finalColor;

void main()
{
    finalColor = texture2D(tex, vec2(0, 0));
}

[test]
texture rgbw 0 (16, 16) 
resident texture 0
uniform handle tex 0
draw rect -1 -1 2 2 
relative probe rgb (0.0, 0.0) (1.0, 0.0, 0.0)
```

从这个test可以看到常量uniform为1
设定ST_DEBUG=constants 可打印

```
param[0] sz=4 UNIFORM tex = {1.4e-45, 0, 0, 0}

```
表明常量为1.4e-45， 这是浮点数解释实际为无符号数

通过gallium_ddebug日志

```
c0027600 SET_SH_REG:
0000000e
001102c0         SPI_SHADER_USER_DATA_PS_2 <- 0x001102c0
00110240         SPI_SHADER_USER_DATA_PS_3 <- 0x00110240
c0027600 SET_SH_REG:



PS - Constant buffer slot 0 (CPU list):
        SQ_BUF_RSRC_WORD0 <- 0x001102c0
        SQ_BUF_RSRC_WORD1 <- BASE_ADDRESS_HI = 0
                             STRIDE = 0
                             CACHE_SWIZZLE = 0
                             SWIZZLE_ENABLE = 0
        SQ_BUF_RSRC_WORD2 <- 16 (0x00000010)
        SQ_BUF_RSRC_WORD3 <- DST_SEL_X = SQ_SEL_X
                             DST_SEL_Y = SQ_SEL_Y
                             DST_SEL_Z = SQ_SEL_Z
                             DST_SEL_W = SQ_SEL_W
                             NUM_FORMAT = BUF_NUM_FORMAT_FLOAT
                             DATA_FORMAT = BUF_DATA_FORMAT_32
                             ELEMENT_SIZE = 0
                             INDEX_STRIDE = 0
                             ADD_TID_ENABLE = 0
                             ATC = 0
                             HASH_ENABLE = 0
                             HEAP = 0
                             MTYPE = 0
                             TYPE = SQ_RSRC_BUF


```

可以看到const buf地址为0x001102c0, 其中PS_2恰好为constbuf地址


通过linux命令打印该地址内容可以

```bash

user@user:/home/shiji/piglit/bin$ od -j  0x00110400 -N 1  -t f4  bo_shader_runner_817_00000003.bin
4202000           1e-45
4202001
user@user:/home/shiji/piglit/bin$ od -j  0x00110400 -N 1  -t u4  bo_shader_runner_817_00000003.bin
4202000          1
4202001

```
可以看到恰好为uniform内容1


