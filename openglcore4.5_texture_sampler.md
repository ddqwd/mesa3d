# 纹理相关规范总结

`GenTextures`获取纹理对象名
`BindTexture`创建一个纹理对象或者将纹理对象绑定到一个新的目标
`BindTextures`符合操作， 创建一组纹理对象
`BindTextureUint`将纹理对象绑定到纹理单元，如果纹理为0,重置以这个纹理单元开始之后的所有纹理绑定
`CreateTextures`和`DeleteTextures`用来创建和删除纹理
纹理对象可同时绑定多个纹理单元，其中一个单元的操作会影响其他纹理单元的绑定状态

`TexImage3D(enum target, int level, int internalformat, sizei width, sizei height, sizei depth, int border ,enum format ,enum type, void*data) `用来明确三位纹理图像

`UNPACK_ROW_LENGTH `和`UNPACK_ALIGNMENT` 控制图像内存行与行空间的大小。
`UNPACK_SKIP_IMAGES`用来决定三维图像子体积的位置，表明跳过二维图像的多少。
如果纹理内部格式是整数，则分量被夹取在[-2^(n-1), 2^(n-1)-1]或[0,2^n-1]。
如果纹理内部格式是归一化固定点数， 被夹取在[-1,1]或[0,1]

可以使用采样对象用来保存纹理的参数,有`glGenSamplers`,`glCreateSamplers`,`BindSampler(s)`, `SamplerParameter{ifv}`

纹理图像被明确使用压缩格式的图像数据叫压缩纹理图像,通过`CompressedTexImage{1D,2D,3D}`,`CompressedTex(ure)SubImage{1D,2D,3D}`来定义这类图像

当立方体贴图纹理被采样时， 纹理坐标被视位一个从立方体中心发射的方向向量。q坐标被忽略。

每个纹素包含多个采样值的纹理叫做多重采样纹理，这类纹理没有多个图像级别, 使用TexImage\*Multisample来定义这列纹理

使用不可变存储格式纹理的数据，将之解析成不同格式的纹理叫纹理视图，使用`TextureView`产生这种纹理

缓冲对象附着到缓冲纹理，纹理图像数据从缓冲对象数据存储获取的这类纹理叫缓冲纹理，当缓冲对象数据改变时，附着到的缓冲纹理随之改变
缓冲纹理没有多个图像级别，只有单个级别
使用`Tex(ure)BufferRange`将缓冲对象的具体数据与缓冲纹理进行绑定,shader中使用samplerBuffer来指示这种纹理
缓冲纹理使用的目标必须是`TEXTURE_BUFFER`

纹理 参数通过`TexParameter*`来指定，可以用Sampler对象来替代

一旦一个纹理通过`Tex(ture)Storage{1D,,2D,3D, 2DMultisample, 3DMultisample}`命令明确所有等级的格式和维度变得不可变， 而图像的内容和参数仍然可以改变， 这中纹理被称为**不可变格式纹理**

`InvalidateTexSubImage`和`InvalidateTexImage`使某个子区域的纹理数据无效化可以提高性能,减少数据上传

`ClearTexSubImage`和`ClearTexImage`可以用一个常量值来填充纹理的子区域，达到清理的效果。

当纹理的基本内部格式是`DEPTH_COMPONET`或者`DEPTH_STENCIL`时，纹理值可以根据比较函数计算
`TEXTURE_COMARE_MODE`规定了比较操作数， `TEXTURE_COMPARE_FUNC` 规定了比较函数

如果纹理内部格式是`RGB9_E5`会发生共享指数颜色转换

shaders可以通过将纹理绑定到图像单元读取纹理数据
图像单元从0开始由实现决定范围， 最大值定义为`MAX_IMAGE_UINTS`, 在mesa18中为32*8.
和纹理图像单元不同，图像单元没有分离绑定点，每个图像单元一次只能有一个纹理绑定
Shader中的图像变量可以用**coherent**, **volatile**, **restrict**, **readonly**， **writeonly**限定符来修饰
对于shader读取操作的图像必须用`layout`声明输入格式
格式layout 图像格式分为浮点，有符号和无符号格式
shader中对图像操作的内建函数有imageSize, imageLoad, imageStore.此外还有原子相关图像操作
使用图像需要注意内存一致性问题
通过`glBindImageTexture(s)`可将图像绑定到绑定点， 这应该发生在shader编译之后。
如果**layered**为真，则纹理必须为数组纹理，CubeMap，3D纹理。


# mesa18实现分析

## 纹理基本用法

```c
_mesa_GenTextures(GLsizei n, GLuint *textures)
    create_textures_err
        create_textures
           st_NewTextureObject(-,name, target) -> gl_texture_object*
              _mesa_initialize_texture_object(-,st_texture_obj, name, target)
              [return] -> st_texture_object*

_mesa_ActiveTexture(GLenum texture)
   active_texture(texture, false);
      ctx->Texture.CurrentUnit = texUnit;

_mesa_BindTexture(GLenum target, GLuint texName)
    bind_texture(gl_context, target, texName, false)
        st_NewTextureObject
        bind_texture_object(gl_context, gl_context.Texture.CurrentUnit = 0, newTexObj)

_mesa_BindTextures(GLuint first, GLsizei count , const GLuint *textures)
    BindTextures
       bind_texture_object(gl_context, first+i, -) 
       unbind_textures_from_unit(gl_context, first+i)

_mesa_CreateTextures(GLenum target, GLsizei n, GLuint *textures)
    create_textures_err(gl_context, target, n , textures, -)
        create_textures
```

* 在`_mesa_GenTextures`中，首先通过`_mesa_HashFindFreeKeyBlock` 获取texobj 起始first哈希值，然后递增通过st_NewTextureObject构造texobj
* 在`st_NewTextureObject `中，首先构造一个`st_texture_object `，为`st_texture_object.sampler_views` 分配计数为1的空间, sampler_view表示当前上下文的纹理的一个预览面,用于vs, fs.然后调用`_mesa_initialize_texture_object `将texobj基类gl_texture_object成员的值初始化为默认值，比如`st_texture_object._BufferObjectFormat`为GL_R8,这一点在
OpenGL core 4.5 8.22节specs
> Each initial texture image is null. It has zero width, height, and depth, internal
format RGBA, or R8 for buffer textures, component sizes set to zero and component
types set to NONE, the compressed flag set to FALSE, a zero compressed size, and
the bound buffer object name is zero.
* `_mesa_BindTexture` 通过`bind_texture_object`将纹理对象与当前纹理单元绑定,实际`_mesa_reference_texobj `将texObj 绑定到`gl_texture_unit.CurrentTex[targetIndex]`中
* 在`bind_texture_object`内，设置`ctx.NewStates`状态位`_NEW_TEXTURE_OBJECT` 


```c 
//Requires GL 4.5 or ARB_direct_state_access
_mesa_BindTextureUnit(GLuint unit, GLuint texture)
   bind_texture_unit(ctx, unit, texture, false);
   _mesa_lookup_texture(ctx, texture)-> gl_texture_object* = texObj
   bind_texture_object(ctx, unit, texObj)

```

* 除了通过`glActiveTexture`和`glBindTexture(s)`,还可以通过`glBindTextureUnit `直接进行纹理单元的绑定， 如果unit为0，则解绑所有该目标上的纹理单元

## 纹理图像数据明确

see with format.pdf 

```c
_mesa_TexImage2D( GLenum target, GLint level, GLint internalFormat,
                  GLsizei width, GLsizei height, GLint border,
                  GLenum format, GLenum type,
                  const GLvoid *pixels )
----------------------------------------------------------
  teximage_err(ctx, compressed=false, dims=2, ... ) 
    teximage(..., no_error =false)
      _mesa_get_current_tex_object(ctx, target)
      _mesa_get_tex_image(ctx, texObj, target, level) -> gl_texture_image* texImage
          st_NewTextureImage(ctx) -> gl_texture_image* 
              [return] malloc ->
          set_tex_image(texObj, target, level, texImage);

      ctx->Driver.FreeTextureImageBuffer(ctx, texImage);
          st_texture_release_all_sampler_views(st, stObj = texImage->TexObject);
              pipe_sampler_view_release(st->pipe, &views->views[i].view);
          
      _mesa_init_teximage_fields


      ctx->Driver.TexImage(ctx, dims, texImage, format,
        st_TexImage
            prep_teximage(ctx, texImage, format, type);
            st_AllocTextureImageBuffer(ctx, texImage)
                st_gl_texture_dims_to_pipe_dims
                texImage As stImage 
                stImage->pt = st_texture_create(st_context, ...)-> pipe_resource*
                    [return]si_resource_create(pipe_screen* screen,pipe_resource* templ)  -> pipe_resource*
            st_TexSubImage(ctx, dims, texImage, 0, 0, 0, texImage->Width, texImage->Height, texImage->Depth,
                format, type, pixels, unpack);
                u_default_texture_subdata(pipe, stImage->pt, dst_level, 0,&box, data, stride, layer_stride);
                    map=si_texture_transfer_map(pipe, stImage->pt, level, usage, box, &transfer) -> void *
  	                  offset = si_texture_get_offset(sctx->screen,tex,level,box,&trans->b.b.stride,&trans->b.b.layer_stride);
                        map=si_buffer_map_sync_with_rings(sctx, stImage->pt->buffer, usage) -> void*
                            [return] amdgpu_bo_map 
                        [return]  map + offset
                    util_copy_box(map, ...);

                    

si_resource_create(screen, templ)
  si_buffer_create  | for PIPE_BUFFER
  si_texture_create   | for non-buffer -> pipe_resource* 
    si_init_surface(si_screen* screen, radeon_surf* surf, templ,mode,...) -> int
        amdgpu_surface_init(radeon_winsys* (screen->ws), templ, ..., mode, surf) 
        [return] ac_compute_surface(ws->addrlib, &ws->info, &config, mode, surf)
    [return]si_texture_create_object(screen, templ, (struct pb_buffer*)NULL, &surface) -> si_texture*
              si_init_resource_fields(screen, tex->buffer,tex->size, tex->surface.surf_alignment)
              si_alloc_resource(screen, tex->buffer) ---- winsys
              si_screen_clear_buffer(screen, tex->cmask_buffer, tex->cmask_offset, tex->surface.cmask_size, 0xCCCCCCCC) 
              si_screen_clear_buffer(screen, tex->buffer, tex->htile_offset, tex->surface.htile_size, 0)
              si_screen_clear_buffer(screen, tex->buffer, tex->dcc_offset, tex->surface.dcc_size, 0xFFFFFFFF) 
              [return] -> tex



ac_compute_surface(addrlib, info, config, mode, surf)
    [return] surf_config_sanity(config, surf->flags)

    [return] gfx6_compute_surface(addlib, info, config, surf) ->
                 [return] gfx6_compute_level(addrlib, config, surf, ...) -> 
                          [return] AddrComputeSurfaceInfo(addrlib, AddrSurfInfoIn, AddrSurfInfoOut) ->
                 [return] gfx6_surface_settings(addrlib, info ,config, AddrSurfInfoOut,surf) -> 
                 [return] ac_compute_cmask(info,config, surf) -> 0
```
   

* `_mesa_get_tex_image`内会获取当前texObj 目标为target的level的texImage, 如果获取成功返回，否则调用stack_tracker创建一个texImage
* `set_tex_image`内将texImage的TexObject指向texObj, 同时将`texObj.Image[face][level]`指向texImage
* 在u_default_texture_subdata内，首先通过si_texture_transfer_map 向winsys获取bo_map地址， 之后通过util_copy_box将传入的pixels写入这个map地址
* 在`st_texture_create`内部会创建一个pipe_resource 结构，这个结构保存纹理的相关信息,传入后续处理
* 在`si_texture_create`内， 调用之前，会通过`si_choose_tiling `获取`tiling mode`设置，选择相应的平铺计算模式， 如果设置`nodcc`则mode为`RADEON_SURF_MODE_LINEAR_ALIGNED` 
* 在`amdgpu_surface_init`内部 会生成`ac_surf_config`变量，然后用`templ`的相关信息填充`ac_surf_config.info`.
* 在`ac_compute_surface`内部,
    1. 首先产生addrlib相关的输入输出结构，有`AddrSurfInfoIn(out), AddrDccIn(out),AddrHtileIn(Out),AddrTileInfoIn(Out).` 
    2. 之后根据mode分类,设置`AddrSurfInfoIn.tileMode`， 在这里对应`RADEON_SURF_MODE_LINEAR_ALIGNED` 的为`ADDR_TM_LINEAR_ALIGNED`
    3. 对模板附件纹理，`dcc, fmask, cmask, TC-compatible HTIL` 进行设置填充信息，将addrlib相关的结构加入对应架构的addrlib中

* 在`si_texture_create_object`内部， 
    1. 首先构造si_texture结构tex, 填充tex.surface 为surf,  
    2. 如果存在多重纹理， 并且采用fmask设置，将`ac_compute_surface` 处理后的surface有关fmask, cmask设置信息填充 `tex.fmask_offset, tex.size, tex.cmask_offset. tex->cmask_buffer`
    3. 根据surface的dcc设置， 对`tex.dcc_offset，tex.size`进行填充
    4. 此时pb_buffer为NULL，调用`si_alloc_resource`向winsys申请bo, 否则将tex->buffer的buf,bo地址指向这个`pb_buffer`参数地址
    5. 根据`cmask, htile,dcc` 的设置，调用`si_screen_clear_buffer`进行设置, 同时对`tex->cmask_base_address_reg`设置成buffer的gpu地址加上cmask
    6. 设置`R600_DEBUG=tex`,此时可以打印纹理相关设置信息


## 纹理视图

在`TexStorage`的分析基础上，

```c
_mesa_TextureView(GLuint texture, GLenum target, GLuint origtexture,
                  GLenum internalformat,
                  GLuint minlevel, GLuint numlevels,
                  GLuint minlayer, GLuint numlayers)
----------------------------------------------------------------------
   origTexObj = _mesa_lookup_texture(ctx, origtexture);
   texObj = _mesa_lookup_texture(ctx, texture);
   _mesa_texture_view_compatible_format(ctx, origTexObj->Image[0][0]->InternalFormat, internalformat)
   texture_view(ctx, origTexObj, texObj, target, internalformat, minlevel,
                numlevels, minlayer, numlayers, false);
       initialize_texture_fields
   st_TextureView(ctx, texObj, origTexObj)
       pipe_resource_reference(&texObj->pt, origTexObj->pt);
       pipe_resource_reference(&texObj[0][0]->pt, texObj[0]->pt);

```
* 在_mesa_TextureView内部， 首先通过_mesa_lookup_texture查找origtexture对应的纹理对象，然后检查该纹理是否是不可变格式，源纹理和当前纹理格式是否兼容，如果兼容继续下一步
* 在texture_view内部， 为当前texobj创建了 st_image，同时设置相关格式参数
* 在st_TextureView， 将texture的资源图像数据pt设置成origtexture的纹理图像数据， 达到引用计数的效果


## 采样器对象

```c
_mesa_GenSamplers(GLsizei count, GLuint *samplers)
   create_samplers_err(ctx, count, samplers, "glGenSamplers");
      create_samplers(ctx, count, samplers, caller);
        st_NewSamplerObject
            _mesa_new_sampler_object(ctx, name) -> sampObj
                _mesa_new_sampler_object

_mesa_BindSampler(GLuint unit, GLuint sampler)
   bind_sampler(ctx, unit, sampler, false);
     _mesa_bind_sampler(ctx, unit, sampObj);
        _mesa_reference_sampler_object(ctx, &ctx->Texture.Unit[unit].Sampler, sampObj);

```

* 采样器对象较为简单，就是更新相应纹理单元的Sampler对象，在`_mesa_bind_sampler`内， 如果sampObj不等于当前纹理单元的Sampler，会置`_NEW_TEXTURE_OBJECT`状态位



## 压缩纹理

```c
_mesa_CompressedTexImage3D(GLenum target, GLint level,
                              GLenum internalFormat, GLsizei width,
                              GLsizei height, GLsizei depth, GLint border,
                              GLsizei imageSize, const GLvoid *data)
------------------------------------------------------------------
    teximage_err(ctx, compressed = 1,dims= 3,...)
        teximage(ctx, compressed,...)
```

* 可见压缩纹理普通的纹理明确都是调用teximage，只不过teximage的参数compressed参数此时为真,后续流程相同

## 缓冲纹理

```c
_mesa_TexBufferRange(GLenum target, GLenum internalFormat, GLuint buffer,
                     GLintptr offset, GLsizeiptr size)
-----------------------------------------------------
    _mesa_lookup_bufferobj_err -> bufObj 
    _mesa_get_current_tex_object -> texObj
        _mesa_get_current_tex_unit(ctx) -> texObj
   texture_buffer_range(ctx, texObj, internalFormat, bufObj,offset, size, "glTexBufferRange");
      st_TexParameter
```

* 在_mesa_TexBufferRange 内,首先通过check_texture_buffer_target 检查target是否为TEXTURE_BUFFER，
* 之后查找当前buffer所属的bufferobj之后，校验相关buffer格式范围，之后通过_mesa_get_current_tex_object获取当前激活的纹理单元中目标为target的纹理对象texObj
* 在`texture_buffer_range`内部，会更新`texObj.BufferObjectFormat,_BufferObjectFormat, BufferOffset, BufferSize` 为传入参数， 并且将`texObj.BufferObject`指向这个bufObj,最后会设置驱动状态为`ST_NEW_SAMPLER_VIEWS`,

## 不可变纹理

```c
_mesa_TexStorage3D(GLenum target, GLsizei levels, GLenum internalformat,
                   GLsizei width, GLsizei height, GLsizei depth)
-----------------------------------------------------------------------
    texstorage_error(dims=3,...) 
        _mesa_get_current_tex_object(gl_context, target) -> gl_texture_object* texObj 
        texture_storage_error(ctx, dims, texObj,...,dsa= false) 
            texture_storage(ctx,dims, texObj, NULL, ...,dsa=false, 0 , false)
                initialize_texture_fields(ctx, texObj, ...)
                    _mesa_init_teximage_fields(ctx, texObj.Image[face[level])
                    _mesa_next_mipmap_level_size(target, 0, levelWidht, levelHeight,levelDepth, ...)
                st_AllocTextureStorage(ctx, texObj, ...)
                    st_texture_storage
                        st_gl_texture_dims_to_pipe_dims
                        st_texture_create(...) -> pipe_reosurce* (texObj->pt)
                        compressed_tex_fallback_allocate(ctx, texObj.Image[face][leve])
                _mesa_set_texture_view_state(ctx, texObj, target, levels)
                update_fbo_texture(ctx, texobj)
                    _mesa_update_fbo_texture(ctx, texObj, face, level)
```


* 在`texture_storage`内部，首先会进行纹理格式的检查，然后初始化纹理对象,分配纹理存储 ，设置不可变格式状态，更新fbo纹理
* 在`initialize_texture_field`s中，首先初始化tex.Image不同face,level, 然后调用`_mesa_next_mipmap_level_size`进行各个mipmap层及图像尺寸的计算
* `st_AllocTextureStorage` 内部, 首先检查texObj.Image[0][0]是否为多重采样，会对多重采样数进行格式更正,之后通过`st_texture_create`创建纹理资源和上面用法一致
* `_mesa_set_texture_view_state` 内部会将texObj.Immutable, texObj.ImmutableLevels, texObj.MinLevel, texObj.MaxLevel, texObj.MinLayer 设定为固定值,其中Immutable表明了这个纹理是不可变纹理格式

*` _mesa_BindTextures`会对fist开始的count 纹理单元连续绑定


## 纹理清理

```c
_mesa_ClearTexSubImage(GLuint texture, GLint level,
                       GLint xoffset, GLint yoffset, GLint zoffset,
                       GLsizei width, GLsizei height, GLsizei depth,
                       GLenum format, GLenum type, const void *data)
-----------------------------------------------------------------
    get_tex_obj_for_clear(ctx, "glClearTexSubImage", texture) -> texObj
        _mesa_lookup_texture_err
    get_tex_images_for_clear(ctx, "glClearTexSubImage",texObj, level, texImages) -> numImages
    st_ClearTexSubImage(ctx, texImage, ...)
        si_clear_texture(ctx,texImage.pt,..., )
            util_format_description(tex->format) -> util_format_description * desc

            sf = si_create_surface(ctx, texImage.pt, (pipe_surface)tmpl) -> pipe_surface*
                [return] si_create_surface_custom(ctx, texImage.pt,tmpl, ...) -> pipe_surface* 
                         // tmp 控制格式
	                     pipe_resource_reference(&surface->base.texture, texture);
                         [return] -> si_surface*

            desc->unpack_*_uint{sint, float}(pipe_color_union color, , 0, data, 0 , 1,,1);

		    si_clear_depth_stencil(pipe, sf, clear, depth, stencil, box->x, box->y, box->width, box->height, false);
                si_blitter_begin
	            util_blitter_clear_depth_stencil(sctx->blitter, dst, clear_flags, depth, stencil,
	            si_blitter_end(sctx);

			si_clear_render_target(pipe, sf, &color, box->x, box->y, box->width, box->height, false);
                si_blitter_begin(sctx, SI_CLEAR_SURFACE | SI_DISABLE_RENDER_COND)
                util_blitter_clear_render_target(sctx->blitter, dst=sf, color,dstx=box->x, dsty=box->y, width, height);
	            si_blitter_end(sctx);
```

* 在`si_clear_texture`内部,首先通过目标纹理图像的存储格式，获取格式描述符，为当前清楚的纹理区域创建一个pip_surface 纹理预览，之后根据纹理使用的类型使用格式描述符对数据像素解包，调用具体`si_clear_depth_stencil `或`si_clear_render_target `来进行实际清除。
* `si_clear_render_target`内，通过blitter来清除纹理数据。分为三步，第一步通过si_blitter_end保存当前shader，光栅化，fb的状态， 第二步通过`util_blitter_clear_render_target `开始清除fb颜色缓冲表面设置，并且恢复之前设置的状态，第三步通过`si_blitter_end `设置shader指针脏位,发射修改后的blitter vs设置,blitter在此处不作分析


## 多重采样纹理

```c
_mesa_TexStorage3DMultisample(GLenum target, GLsizei samples,
                              GLenum internalformat, GLsizei width,
                              GLsizei height, GLsizei depth,
                              GLboolean fixedsamplelocations)
--------------------------------------------------------------------
   texObj = _mesa_get_current_tex_object(ctx, target);
   texture_image_multisample(ctx, 3, texObj, NULL, ..., GL_TRUE, 0, "glTexStorage3DMultisample");
   _mesa_init_teximage_fields_ms(ctx, texImage,...) 
   ctx->Driver.AllocTextureStorage(ctx, texObj, 1, width, height, depth)


_mesa_TexImage3DMultisample(GLenum target, GLsizei samples,
                            GLenum internalformat, GLsizei width,
                            GLsizei height, GLsizei depth,
                            GLboolean fixedsamplelocations)
--------------------------------------------------------------------
   texObj = _mesa_get_current_tex_object(ctx, target);
   texture_image_multisample(ctx,3,texObj, NULL, ..., immutable=GL_FALSE,offset=0, "glTexImage3DMultisample");
   texImage = _mesa_get_tex_image(ctx, texObj, 0, 0);
   _mesa_init_teximage_fields_ms(ctx, texImage, ...)
   st_AllocTextureStorage(ctx, texObj, 1, ...)
      st_texture_storage
        si_is_format_supported(screen, format, ptarget, num_samplers, num_samples, PIPE_BIND_SAMPLER_VIEW);  

```

* 可见`_mesa_TexStorage3DMultisample`和`_mesa_TexImage3DMultisample`的区别在与对`texture_image_multisample`的调用参数上面`immutable`时为真
* 两者通过`_mesa_init_teximage_fields_ms`将`texImage.NumSamples`及`FixedSampleLocations` 更新为参数`samples, fixedSampleLocations`
* 在`st_texture_storage`中，会将NumSamples置为最小起始数2,依次递增，在[2, MAXSAMPLES]范围内获取格式支持的最小采样数，这一操作在`si_is_format_supported` 调用过程中完成,并且更新该`NumSamples`



## 状态数据更新

再进行上面的纹理设置之后，根据 设置`_NEW_TEXTURE_OBJECT` 状态位, 在draw命令issues后，会检查`ctx.NewState`的状态,调用_mesa_update_state进行状态更新

```c
_mesa_update_state(ctx)
    _mesa_update_state_locked(ctx)
        _mesa_update_texture_state(ctx);
            update_program_texture_state(ctx, prog, enabled_texture_units);
                update_single_program_texture_state(ctx, prog[i], prog[i].SamplerUnits[s], enabled_texture_units)
                    [return] update_single_program_texture(ctx, prog, unit) -> gl_texture_obj*
                    [return] texObj = _mesa_get_fallback_texture(ctx, target_index);
        update_program(ctx);
        update_program_constants(ctx)
        ctx->Driver.UpdateState(ctx);
            [st_invalidate_state]
                st_get_active_states(ctx) -> st->active_states
```

* 在_mesa_update_state_locked内部, 
    1. 设置环境变来功能MESA_VERBOSE=state查看状态更新设置
    2. 校验是否有纹理对象_NEW_TEXTURE_OBJECT或_NEW_PROGRAM产生，如果存在调用_mesa_update_texture_state更新shader中纹理的绑定
    3. 如果存在新程序对象则update_program
* 在_mesa_update_texture_state内，首先获取当前使用的程序对象，调用update_program_texture_state 更新程序对象中纹理状态并返回总计使用的纹理单元，最后根据返回的enabled_texture_units 值清除各个未使用纹理单元的纹理对象
* 在`update_program_texture_state` 内部， 遍历所有已经链接好的shader,通过`gl_program.SamperUsed`检查是否使用了纹理.该`SamplersUsed`是一个mask整数，每一位表示一个使用sampler的用glUniform*这类形式确定的纹理单元索引s
 如果使用, 对该shader program 进入`update_single_program_texture_state` 对该shader进行纹理对象绑定更新,首先获取纹理对象，之后设置启用该纹理单元位
* 在`update_single_program_texture`内首先获取unit位置的纹理单元，获取shader中unit位置的纹理类型索引，进而获得当前纹理单元目标纹理类型的纹理对象，之后校验纹理完整性,如果纹理完整，返回该对象，否则返回一个由mesa实现的重新构造的falllback完整纹理对象,参数为mesa自定义默认参数
*  `update_program_constants`作用是更新固定功能接口传入的常量参数，如gl_ModelViewMatrix
* 在shader中的采样器与纹理单元绑定更新之后， 调用`st_invalidate_state`开始设置驱动状态
* 对于出现新的程序对象时，`st_get_active_states` 获取各个shader的链接时状态，这个链接状态由各个shader的affected_states指定，这个字段表示链接时shader内部的各个状态，比如说使用了纹理，那么会将状态设置`ST_NEW_SAMPLERS`，这个表示新的采样状态
* 对于`_NEW_TEXTURE_OBJECT `状态， 会将当前程序对象激活的`ST_NEW_SAMPLER_VIEWS ,ST_NEW_SAMPLERS, ST_NEW_IMAGE_UNITS`,更新到`st_context.dirty., st_context.dirty`.对于初次渲染，在st_context创建这个dirty会初始化为全部状态启用，即`ST_ALL_STATES_MASK`.在每次渲染之后都会重置

如果使用纹理之后,会同时将ST_NEW_SAMPLERS 和 ST_NEW_SAMPLER_VIEWS 置位

首先处理ST_NEW_SAMPLERS

```c
// ST_NEWS_SAMPLERS_VIEW
st_update_vertex_textures(struct st_context *st)
    update_textures_local(st, PIPE_SHADER_VERTEX, ctx->VertexProgram._Current)
        struct pipe_sampler_view *local_views[PIPE_MAX_SAMPLERS] = {0};
        update_textures(st, shader_stage, prog, local_views);
            [loop num_samplers] struct pipe_sampler_view *sampler_view = NULL;
                                st_update_single_texture(st, &sampler_view, texUnit, glsl130, texel_fetch_samplers & 1);
                                     [return ] *sampler_view = st_get_buffer_sampler_view_from_stobj(st, stObj); | for tbo
                                     samp = _mesa_get_samplerobj(ctx, texUnit);
                                    *sampler_view = st_get_texture_sampler_view_from_stobj(st, stObj, samp, ...); | for  others
                                pipe_sampler_view_reference(&(local_views[unit]), sampler_view);
            cso_set_sampler_views(st->cso_context, shader_stage, num_textures, sampler_views=local_views);

// tbo 
st_get_buffer_sampler_view_from_stobj(st, stObj) -> pipe_sample_view*
   struct st_buffer_object *stBuf =
      st_buffer_object(stObj->base.BufferObject);
   [return] st_texture_get_current_sampler_view
   struct pipe_sampler_view templ;
   temp.xxx= ...;
   struct pipe_sampler_view *view =
      st->pipe->create_sampler_view(st->pipe, buf=stBuf->buffer, &templ);
            goto label si_create_sampler_view;

   view = st_texture_set_sampler_view(st, stObj, view, false, false);
   [return] view
// for non-tbo
st_get_texture_sampler_view_from_stobj(st, stObj, samp, glsl130_or_later, ignore_srgb_decode);
   [return] sv=st_texture_get_current_sampler_view(st, stObj) -> st_sample_view*
   struct pipe_sampler_view *view =
         st_create_texture_sampler_view_from_stobj(st, stObj, format, glsl130_or_later);
            struct pipe_sampler_view templ;
            temp.xxx= ...;
            [return] st->pipe->create_sampler_view(st->pipe, stObj->pt, &templ) -> pipe_sampler_view*
                    see label si_create_sampler_view;
   [return] view = st_texture_set_sampler_view(st, stObj, view, glsl130_or_later, srgb_skip_decode);
                struct st_sampler_views *views;
                views = stObj->sampler_views;
                views->view[i]->view= view;

[label] si_create_sampler_view;
si_create_sampler_view(ctx, buf, state=tmpl, ...)
   si_create_sampler_view_custom(sctx=ctx, (si_texture*)texture=buf,state,stObj->pt->width0, stObj->pt->height0) 
        struct si_sampler_view *view = CALLOC_STRUCT(si_sampler_view); 
        pipe_resource_reference(&view->base.texture, texture); 
        // for tbo
        si_make_buffer_descriptor(sctx->screen,
        	  r600_resource(texture),
        	  state->format,
        	  state->u.buf.offset,
        	  state->u.buf.size,
        	  view->state);
        
        // for texture
        si_make_texture_descriptor(sctx->screen, texture, true,
            state->target, pipe_format, state_swizzle,
            first_level, last_level,
            state->u.tex.first_layer, last_layer,
            width, height, depth,
            view->state, view->fmask_state);

        [return] &view->base;
   
// for tbo
si_make_buffer_descriptor(sctx->screen,...)
	state[4] = 0;
	state[5] = S_008F04_STRIDE(stride);
	state[6] = num_records;
	state[7] = S_008F0C_DST_SEL_X(si_map_swizzle(desc->swizzle[0])) |
	

// for texture
si_make_texture_descriptor(.,tex=texture..,state,fmask_state)
	state[0] = 0;
    state[1-7] = ...

    //texture->surface.fmask_size for fmask 
	va = tex->buffer.gpu_address + tex->fmask_offset;
    fmask_state[0] = (va >> 8) | tex->surface.fmask_tile_swizzle;
	fmask_state[1] = S_008F14_BASE_ADDRESS_HI(va >> 40) |
    fmask_state[2..7]= ...
		
cso_set_sampler_views(st->cso_context, shader_stage, num_textures, sampler_views);
    si_set_sampler_views(ctx, shader, start=0, count, views=sampler_views)
	    [loop count]si_set_sampler_view(sctx, shader, slot=start + i, views[i], false);
	                    struct si_samplers *samplers = &sctx->samplers[shader];
                        si_set_sampler_view_desc(sctx, sview,samplers->sampler_states[slot], desc);
	                        struct si_texture *tex = (struct si_texture *)sview->texture;
                            memcpy(desc, sview->state, 8*4);
                            
                            // for tbo
		                    si_set_buf_desc_address(&tex->buffer, sview->base.u.buf.offset, desc + 4);
	                            uint64_t va = buf->gpu_address + offset;
	                            state[0] = va;
	                            state[1] &= C_008F04_BASE_ADDRESS_HI;
	                            state[1] |= S_008F04_BASE_ADDRESS_HI(va >> 32);

                            // for non-tbo
                            si_set_mutable_tex_desc_fields(sctx->screen, tex, sview->base_level_info,..., desc)
	                            va = tex->buffer.gpu_address;
		                        va += base_level_info->offset;
                            	state[0] = va >> 8;
                            	state[1] &= C_008F14_BASE_ADDRESS_HI;
                            	state[1] |= S_008F14_BASE_ADDRESS_HI(va >> 40);
                                state[2..7]=...
		                memcpy(desc + 8, sview->fmask_state | null_texture_descriptor, 8*4);
			            si_set_sampler_state_desc(sstate, sview, is_buffer ? NULL : tex, desc + 12);

st_update_fragment_textures(struct st_context *st)
   update_textures(st, PIPE_SHADER_FRAGMENT, ctx->FragmentProgram._Current, st->state.frag_sampler_views);

```

* 在`update_textures`内部，对所有使用的shader纹理遍历处理，调用`st_update_single_texture` 获取该纹理单元的采样预览
* 在`st_update_single_texture`内， 对于tbo调用`st_get_texture_sampler_view_from_stobj `获取预览， 对于非tbo，通过`st_get_texture_sampler_view_from_stobj` 获取
* 对于tbo， 在`st_get_buffer_sampler_view_from_stobj` 内，首先查找当前纹理对象是否存在采样预览，存在直接返回， 否则调用驱动create_sampler_view构建
* 对于非tbo纹理，流程与tbo类似，先通过`st_create_texture_sampler_view_from_stobj` 再调用驱动`create_sampler_view` 构建， 两者区别在与传参时tbo使用的buffer数据，而纹理使用的实际纹理上传数据
* 在st_texture_set_sampler_views内，遍历views->views数组，找到属于当前si_context的views，如果找到或者没找到但存在一个空闲的views，则将该views.view置为参数views, 否则当view->count> views->max，时重新分配一个更大的views数组,否则获取当前count的views， 并递增count.返回参数view
* 在`si_create_sampler_view_custom`内,首先构建派生类`si_samplers_view`, 继承参数state， 之后初始化基类state参数,将基类纹理base.texture指向texture,最后对tbo使用si_make_buffer_descriptor 构建buffer描述符R_008F00_SQ_BUF_RSRC_WORD0， 对非tbo纹理使用si_make_texture_descriptor为原samper_view和 fmask构建R_008F10_SQ_IMG_RSRC_WORD0 描述符
* 在`si_make_buffer_descriptor` 和 `si_make_texture_descriptor`内， 对非地址外的位置写入描述符数据,保留地址为空
* 在`cso_set_sampler_views`内，上传描述符资源
    1. 在si_set_sampler_view_desc内， 首先通过memcpy将sview->state描述符的数据赋值到desc，
    2. 对于tbo调用si_set_buf_desc_address 将实际的buffer数据虚拟地址加上偏移写入desc +4 = state[4]
    3. 对于非tbo，调用si_set_mutable_tex_desc_fields 将纹理描述符地址写入
    4. 设定地址之后， 对于tbo来说无fmask设置，如果当前存在采样参数配置，则将采样状态设置写入desc[12:15]
    5. 对于非tbo ，fmask类采样则将fmask描述符写入desc[8:15],如果非fmask则将当前存在的采样状态设置写入desc[12:15],等待后续更新
*  简单tbo测试可通过gallium_ddebug 的sampler slot定位buffer地址后 ，再读取bin文件如下打印,图中数据为texture buffer实际数据

```bash
       user@user:/home/shiji/piglit/bin$ xxd -s 0x00000001000C0000 -l 131  bo_arb_texture_buffer_range-ranges_21596_00000002.bin
       1000c0000: 0101 0303 0505 0707 0909 0b0b 0d0d 0f0f  ................
       1000c0010: 1111 1313 1515 1717 1919 1b1b 1d1d 1f1f  ................
       1000c0020: 2121 2323 2525 2727 2929 2b2b 2d2d 2f2f  !!##%%''))++--//
```

再处理ST_NEW_SAMPLER_VIEWS
根据ST_NEW_SAMPLERS定义

```c
 /* Combi    ST_NEW_GS_SAMPLERS | ST_NEW_FS_SAMPLERS | ST_NEW_CS_SAMPLERS)
 #define ST_NEW_SAMPLERS         (ST_NEW_VS_SAMPLERS | \
                                  ST_NEW_TCS_SAMPLERS | \
                                  ST_NEW_TES_SAMPLERS | \
                                  ST_NEW_GS_SAMPLERS | \
                                  ST_NEW_FS_SAMPLERS | \

```
会调用各个shader的sampers处理函数

首先处理ST_NEW_SAMPLER_VIEWS  状态
```c
// ST_NEW_SAMPLERS
st_update_vertex_samplers(struct st_context *st)
   update_shader_samplers(st, PIPE_SHADER_VERTEX, ctx->VertexProgram._Current, NULL, NULL);
      const struct pipe_sampler_state *states[PIPE_MAX_SAMPLERS];
      st_convert_sampler_from_unit(st, sampler, tex_unit);
         st_convert_sampler(st, texobj, msamp=Sampler, ctx->Texture.Unit[texUnit].LodBias,sampler);
      states[unit] = sampler;
     
      states[unit]=NULL | for tbo
      cso_set_samplers(st->cso_context, shader_stage, num_samplers, states);
         [loop] cso_single_sampler(ctx, shader_stage, i<num_samplers, states[i]);
                   *cso = malloc(cso_sampler)
                   memcpy(&cso->state, templ, sizeof(*templ));
                   cso->data = ctx->pipe->create_sampler_state(ctx->pipe, &cso->state);
                      [si_create_sampler_state] -> void *
	                     struct si_sampler_state *rstate = CALLOC_STRUCT(si_sampler_state);
                         [return] rstate
                   iter = cso_insert_state(ctx->cache, hash_key, CSO_SAMPLER, cso);
                   ctx->samplers[shader_stage].cso_samplers[i] = cso;
                   ctx->samplers[shader_stage].samplers[i] = cso->data;
         cso_single_sampler_done(ctx, shader_stage);
               struct sampler_info *info = &ctx->samplers[shader_stage];
               ctx->pipe->bind_sampler_states(ctx->pipe, shader_stage,0, num_samplers,info->samplers);
                  [si_bind_sampler_states(sctx, shader, 0, count, **states]
                    [loop count] struct si_samplers *samplers = &sctx->samplers[shader];
                                 samplers->sampler_states[slot] = sstates[i];
		                         struct si_sampler_view *sview =(struct si_sampler_view *)samplers->views[slot];
			                     tex = (struct si_texture *)sview->base.texture;
		                         unsigned desc_slot = si_get_sampler_slot(slot);
                                 si_set_sampler_state_desc(sstates[i], sview, tex,desc->list + desc_slot * 16 + 12);
```
* 在`update_shader_samplers`内部，
    1. 对于每个shader使用到的非tbo的纹理，通过`st_convert_sampler_fromt_unit`获取对应纹理对象的采样参数,这个状态值由纹理对象的采样对象决定,获取采样状态值后存入states数组
    2. 对于tbo, 不保存采样状态,所以`cso_single_sampler,cso_single_sampler_done`不作处理
    3. 通过cso_set_samplers 下发采样状态
* 在`cso_set_samplers`,首先通过`cso_single_sampler`设定状态，再通过cso_single_sampler_done下发状态
* 在`cso_single_sampler`构造了一个cso_sampler结构， 把采样参数数据存入state成员,通过调用`si_create_sampler_state`将驱动设置的采样参数寄存器配置相关信息si_sampler_state 存入data成员
* `cso_single_sampler_done` 将之前设定的各个纹理的采样参数配置传入驱动处理
* `si_bind_sampler_states`，对num_samplers依次处理，通过si_get_sampler_slot获取`samper_and_image buffer`中的slot位，samples位于槽位 [8..39]，共计32个对应SI_NUM_SAMPLERS ,以递增顺序排列,之后通过slot将获取
采样预览`si_sampler_view`,从而获取对应纹理资源tex, 这里对tbo, fmask 过滤处理;最后通过si_set_sampler_state_desc 把采样数据更新描述符[12:15]位置


## 图像单元 

```c
_mesa_BindImageTexture(GLuint unit, GLuint texture, GLint level,
                       GLboolean layered, GLint layer, GLenum access,
                       GLenum format)
---------------------------------------------------------------------
   valiate_bind_image_texture 
   _mesa_lookup_texture
   bind_image_texture(ctx, texObj, unit, level, layered, layer, access, format)
       set_image_binding(u, texObj, level, layered, layer, access, format);

```
* 该函数首先参数是否正确，之后查找纹理获取纹理对象，在`bind_image_texture`内部， 获取位于unit点的`gl_context.ImageUnits[unit]` 类型为`gl_image_unit`的值 ， 然后设置驱动状态为`ST_NEW_IMAGE_UNITS`
* `set_image_binding` 内部更新gl_image_uint成员，并且将gl_image_uint.TexObj 指向texObj

`ST_NEW_IMAGE_UNITS `实则为`ST_NEW_VS(TES,GS,TCS,FS,CS)_IMAGES`的设置位组合。

和之前分析一样，由于这次是直接设置驱动状态，依次调用相关映射函数 `st_bind_vs(tcs, tes, gs, fs cs)_images`

```c
st_bind_vs_images(struct st_context *st)
    st_bind_images(st, prog = st->ctx->_Shader->CurrentProgram[MESA_SHADER_VERTEX], PIPE_SHADER_VERTEX)
        st_convert_image_from_unit(st, img, prog->sh.ImageUnits[i],prog->sh.ImageAccess[i])
        cso_set_shader_images(st->cso_context, shader_type, 0,prog->info.num_images, images)
            ctx->pipe->set_shader_images(ctx->pipe, shader_stage, start, count, images);
                si_set_shader_images(ctx->pipe, shader_stage, start_slot=start, count, pipe_image_view* views=images);
                    [loop count] si_set_shader_image(ctx, shader, slot, view=&views[i], false);
	                                struct si_descriptors *descs = si_sampler_and_image_descriptors(ctx, shader);
	                                uint32_t *desc = descs->list + desc_slot * 8;
		                            util_copy_image_view(&images->views[slot], view);
	                                si_set_shader_image_desc(ctx, view, skip_decompress=false, desc, NULL);
                                        // for tbo
                                        si_make_buffer_descriptor
		                                si_set_buf_desc_address(res, view->u.buf.offset, desc + 4);

                                        // non-tbo
                                        si_make_texture_descriptor(screen, tex,
		                                si_set_mutable_tex_desc_fields(screen, tex,

```
* 操作流程即是下发图像数据， 这块`si_set_shader_images`同shader的`sample_and_image buffer`分析
* si_set_shader_images 的处理与sampler基本一致， 合并了流程， 不同点在于描述符占字节数为8个word,同时如果存在fmask，则将fmask写在每个SQ_IMG_RSRC_WORD0 后四个



```

st_TexSubImage(struct gl_context *ctx, GLuint dims,
               struct gl_texture_image *texImage,
               GLint xoffset, GLint yoffset, GLint zoffset,
               GLint width, GLint height, GLint depth,
               GLenum format, GLenum type, const void *pixels,
               const struct gl_pixelstore_attrib *unpack)

   struct st_texture_image *stImage = st_texture_image(texImage);

```
