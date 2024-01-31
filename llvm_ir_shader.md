***Written by sj.yu***



```
/* Valid shader configurations:
 *
 * API shaders       VS | TCS | TES | GS |pass| PS
 * are compiled as:     |     |     |    |thru|
 *                      |     |     |    |    |
 * Only VS & PS:     VS |     |     |    |    | PS
 * GFX6 - with GS:   ES |     |     | GS | VS | PS
 *      - with tess: LS | HS  | VS  |    |    | PS
 *      - with both: LS | HS  | ES  | GS | VS | PS
 * GFX9 - with GS:   -> |     |     | GS | VS | PS
 *      - with tess: -> | HS  | VS  |    |    | PS
 *      - with both: -> | HS  | ->  | GS | VS | PS
 *
 * -> = merged with the next stage
 */

```

分析amdgpu 芯片系列为 gfx6-8

#  rw_buffers

##  RW_BUFFERS 槽数及用途

```c /* Private read-write buffer slots. */                                                  
 enum {                                                                                  
     SI_ES_RING_ESGS,                                                                    
     SI_GS_RING_ESGS,                                                                    
                                                                                         
     SI_RING_GSVS,                                                                       
                                                                                         
     SI_VS_STREAMOUT_BUF0,                                                               
     SI_VS_STREAMOUT_BUF1,                                                               
     SI_VS_STREAMOUT_BUF2,                                                               
     SI_VS_STREAMOUT_BUF3,                                                               
                                                                                         
     SI_HS_CONST_DEFAULT_TESS_LEVELS,                                                    
     SI_VS_CONST_INSTANCE_DIVISORS,                                                      
     SI_VS_CONST_CLIP_PLANES,                                                            
     SI_PS_CONST_POLY_STIPPLE,                                                           
     SI_PS_CONST_SAMPLE_POSITIONS,                                                       
                                                                                         
     /* Image descriptor of color buffer 0 for KHR_blend_equation_advanced. */           
     SI_PS_IMAGE_COLORBUF0,                                                              
     SI_PS_IMAGE_COLORBUF0_HI,                                                           
     SI_PS_IMAGE_COLORBUF0_FMASK,                                                        
     SI_PS_IMAGE_COLORBUF0_FMASK_HI,                                                     
                                                                                         
     SI_NUM_RW_BUFFERS,                                                                  
 };                      
```


1. `SI_ES_RING_ESGS` 和 `SI_GS_RING_ESGS`：这些槽位用于引用 ES-GS 环和 GS-GS 环，这些环在图形管线的不同阶段用于数据传递。

2. `SI_RING_GSVS`：这个槽位用于引用 GS-VS 环，用于几何着色器（Geometry Shader）和顶点着色器（Vertex Shader）之间的数据传递。

3. `SI_VS_STREAMOUT_BUF0` 到 `SI_VS_STREAMOUT_BUF3`：这些槽位用于引用顶点着色器流输出缓冲区的不同索引。顶点着色器流输出允许将处理后的顶点数据输出到缓冲区，以供后续渲染阶段使用。

4. `SI_HS_CONST_DEFAULT_TESS_LEVELS`：这个槽位用于引用默认的细分级别常量，通常与细分控制着色器（Tessellation Control Shader）相关。

5. `SI_VS_CONST_INSTANCE_DIVISORS`：这个槽位用于引用实例分割常量，通常与顶点着色器中的实例化绘制相关。

6. `SI_VS_CONST_CLIP_PLANES`：这个槽位用于引用剪裁平面常量，通常与顶点剪裁相关。
7. `SI_PS_CONST_POLY_STIPPLE`：这个槽位用于引用多边形点划线段的常量。

8. `SI_PS_CONST_SAMPLE_POSITIONS`：这个槽位用于引用采样点位置的常量。

9. `SI_PS_IMAGE_COLORBUF0` 到 `SI_PS_IMAGE_COLORBUF0_FMASK_HI`：这些槽位用于引用颜色缓冲区的图像描述符，通常用于高级混合等功能。

10. `SI_NUM_RW_BUFFERS`：这个枚举值表示可用的私有读写缓冲槽位的数量。

## SI_ES_RING_ESGS, SI_GS_RING_ESGS                                                                    

测试和下面一样使用简单的gs即可检测。
接口 : si_update_gs_ring_buffer
            si_set_ring_buffer
创建esgs_ring -> winsys [申请buffer地址]
下发 R_030900_VGT_ESGS_RING_SIZE寄存器

## SI_RING_GSVS,                                                                       

接口逻辑同上 测试用简单的gs即可检测。

创建gsvs_ring -> winsys [申请buffer地址]
下发 R_030904_VGT_GSVS_RING_SIZE寄存器

si_set_ring_buffer 内部将buffere地址写入描述符。

其他逻辑同slot8


## SI_HS_CONST_DEFAULT_TESS_LEVELS (slot =7)

上传数据都在si_set_tess_state中。
内部流程首先时申请const_buffer，写入数据为default_outer_level，和 default_inter_level.
```
st_update_tess
    si_set_tess_state

```
其他逻辑同slot8
用一个简单的tes即可验证。

## 测试SI_VS_STREAMOUT_BUF*

测试关键是使用变换反馈。
```
begin_transform_feedback -> si_create_so_target -> winsys [得到buffer]

```
si_set_streamout_targets -> 写入描述符buffer

这个会和vs main参数的streamout 对应。

st_translate_stream_output_info2

##  SI_VS_CONST_CLIP_PLANES (slot = 9)

测试关键是使用

```
* glEnable ,
* glClipPlane (compat 模式启用）
```

st_update_clip
    si_set_clip_state
        si_set_rw_buffer


这里下发机制和slot8不同的是， 这里用的是user_buffer， 所以在si_set_rw_buffer内部首先是向winsys申请bo地址保存这个buffer然后下发。

## SI_PS_CONST_POLY_STIPPLE (slot = 10)

测试关键
```
glEnable(GL_POLYSTIPPLE)
```

当NEW_POLYSTIPPLE设置时启用

st_update_polygon_stipple
    si_set_polygon_stipple
        si_set_rw_buffers

机理同 slot=9

## 测试SI_VS_CONST_INSTANCE_DIVISORS (slot = 8)


测试关键时在glVertexAttribDivisor 是否大于1

SI_VS_CONST_INSTANCE_DIVISORS主要用来 保存实例化因子

这个只有在divisor大于1的情况下才会使用,用于快速实例化绘图。
rwbuffers在prolog函数内部处理， 通过获取rw的SI_VS_CONST_INSTANCE_DIVISORS的slot获取实际const buf，进而ac_build_fast_udiv_nuw读取divisor 完成。


![sfds](/home/shiji/Downloads/rw.svg)


## SI_PS_CONST_SAMPLE_POSITIONS(slot=11)

初始化就可测试,比较简单。

集中在 si_set_framebuffer_state

```
si_set_framebuffer_state 
    si_set_rw_buffer

```
写入的buffer为ctx的sample_pos_buffer, 多重采样位置。
这个buffer申请在si_create_buffer
其他同slot8

## SI_PS_IMAGE_COLORBUF0,SI_PS_IMAGE_COLORBUF0_HI                                                           

SI_PS_IMAGE_COLORBUF0_HI 为下发的高位数据， 由于rw每个slot只有4word.而image需要占用8word.

最后这四个slot都是在KHR_blend_equation_advanced扩展使用的前提下使用。默认开启。

同时使用了EXT_shader_framebuffer_fetch 和 EXT_shader_framebuffer_fetch_non_coherent扩展才会使用

`EXT_shader_framebuffer_fetch` 和 `EXT_shader_framebuffer_fetch_non_coherent` 是OpenGL扩展，用于扩展着色器对帧缓冲器读取的能力。

这个在mesa默认是不启用的, 如果启用设置如下环境变量

```
export MESA_EXTENSION_OVERRIDE="+GL_EXT_shader_framebuffer_fetch +GL_EXT_shader_framebuffer_fetch_non_coherent"

```
**同时shader里面必须出现inout声明, 以及启用刚才这两个扩展。**

下面对使用场景分析。

下面为caller

```
si_set_framebuffer_state
    si_update_ps_colorbuf0_slot
si_bind_ps_shader
    si_update_ps_colorbuf0_slot
si_update_all_texture_descriptors
    si_update_ps_colorbuf0_slot

```

*  关于 si_set_framebuffer_state 

1. cso调用

cso_set_framebuffer
cso调用和cso_set_fragment_shader出现情况一致。

st_update_framebuffer_state

framebuffer状态设置的时机有以下几处:

1. 处于计算管线
是在atom发射时设置， 这一点和update_fs_fp不同。
2. flush_frontbuffer时
这个由glFlush, glFinish触发,同时使用了renderbuffer, glSwap默认会flush。

所用测试为2种情况。


* si_bind_ps_shader

这个用途就是更性当前的ps shader, 会在shader发生改变如下.
 caller 如下:

```
cso_set_fragment_shader_handle 
cso_destroy_fragment_shader_handle 
cso_delete_fragment_shader_handle 
cso_restore_fragment_shader_handle 

```

除此之外blitter也会调用.

着重看cso_set

* cso_set_fragment_shader_handle

这个函数会在以下接口调用:
1. st_Clear

2. blitter

3. st_update_fp
 条件
    *` st_upate_fp` 会在多个fs时候出现。
* `draw_vbo`时

4. bitmap
```
glBitmap -> st_Bitmap -> draw_bitmap_quad -> setup_render_state
st_DrawAtlasBitmaps -> setup_render_state
```
5. readpixels
```
glReadPixels ->try_pbo_readpixels
```
6. texture
```
glTexImage1D/2D/3D / glCompressedTexSubImage1/2/3D() and                                                                         |   -compressed_tex_sub_image_no_error(un
glCompressedTextureSubImage1/2/3D(). / -->st_CompressedTexImage->st_CompressedTexSubImage ->try_pbo_upload_common
glTexSubImage --> try_pbo_upload->try_pbo_upload_common
```

* si_update_all_texture_descriptors


每一个状态原子函数的调用通过设置dirty脏位进行设计。

```
st_link_shader
    st_set_prog_affected_state_flags
        set_affected_state_flags
```


使用满足条件的case后 可
通过:

```
si_update_ps_colorbuf0_slot
    si_set_shader_image_desc
        si_make_texture_descriptor
        si_set_mutable_tex_desc_fields

```


开始写入buffer。

**写入描述符rw12的值为当前fbo surface的纹理的资源地址,**
**写入描述符rw14的值为资源地址加上fmask_offset**

* 关于默认framebuffer该纹理的来源:

vbo_use_buffer_objects —> BufferData



其他流程同bindless texture

附上 打印记录:

```

RW buffers slot 13 (GPU list):
        SQ_BUF_RSRC_WORD0 <- 0x003fe000
        SQ_BUF_RSRC_WORD1 <- BASE_ADDRESS_HI = 0
                             STRIDE = 0
                             CACHE_SWIZZLE = 0
                             SWIZZLE_ENABLE = 0
        SQ_BUF_RSRC_WORD2 <- 0
        SQ_BUF_RSRC_WORD3 <- DST_SEL_X = SQ_SEL_0
                             DST_SEL_Y = SQ_SEL_0
                             DST_SEL_Z = SQ_SEL_0
                             DST_SEL_W = SQ_SEL_0
                             NUM_FORMAT = BUF_NUM_FORMAT_UNORM
                             DATA_FORMAT = BUF_DATA_FORMAT_INVALID
                             ELEMENT_SIZE = 0
                             INDEX_STRIDE = 0
                             ADD_TID_ENABLE = 0
                             ATC = 0
                             HASH_ENABLE = 0
                             HEAP = 0
                             MTYPE = 0
                             TYPE = SQ_RSRC_BUF

RW buffers slot 14 (GPU list):
        SQ_BUF_RSRC_WORD0 <- 0
        SQ_BUF_RSRC_WORD1 <- BASE_ADDRESS_HI = 0
                             STRIDE = 0
                             CACHE_SWIZZLE = 0
                             SWIZZLE_ENABLE = 0
        SQ_BUF_RSRC_WORD2 <- 0
        SQ_BUF_RSRC_WORD3 <- DST_SEL_X = SQ_SEL_0
                             DST_SEL_Y = SQ_SEL_0
                             DST_SEL_Z = SQ_SEL_0
                             DST_SEL_W = SQ_SEL_0
                             NUM_FORMAT = BUF_NUM_FORMAT_UNORM
                             DATA_FORMAT = BUF_DATA_FORMAT_INVALID
                             ELEMENT_SIZE = 0
                             INDEX_STRIDE = 0
                             ADD_TID_ENABLE = 0
:
```

## SI_PS_IMAGE_COLORBUF0_FMASK,SI_PS_IMAGE_COLORBUF0_FMASK_HI                                                     

用法和color0.测试关键是使用多重采样。
如:
```
 // 创建一个多重采样纹理
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureID);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA, 800, 600, GL_TRUE);

    // 创建一个自定义帧缓冲区
    GLuint framebufferID;
    glGenFramebuffers(1, &framebufferID);
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);

    // 将多重采样纹理附着到帧缓冲区的颜色附着点
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, textureID, 0);

```
可以从日志看到输出。

```
RW buffers slot 14 (GPU list):
        SQ_BUF_RSRC_WORD0 <- 0x0100c01c
        SQ_BUF_RSRC_WORD1 <- BASE_ADDRESS_HI = 0
                             STRIDE = 4880 (0x1310)
                             CACHE_SWIZZLE = 0
                             SWIZZLE_ENABLE = 0
        SQ_BUF_RSRC_WORD2 <- 0x0095c31f
        SQ_BUF_RSRC_WORD3 <- DST_SEL_X = SQ_SEL_X
                             DST_SEL_Y = SQ_SEL_X
                             DST_SEL_Z = SQ_SEL_X
                             DST_SEL_W = SQ_SEL_X
                             NUM_FORMAT = BUF_NUM_FORMAT_UNORM
                             DATA_FORMAT = BUF_DATA_FORMAT_INVALID
                             ELEMENT_SIZE = 0
                             INDEX_STRIDE = 3
                             ADD_TID_ENABLE = 1
                             ATC = 0
                             HASH_ENABLE = 0
                             HEAP = 0
                             MTYPE = 2
                             TYPE = SQ_RSRC_BUF_RSVD_2

RW buffers slot 15 (GPU list):
        SQ_BUF_RSRC_WORD0 <- 0x007fe000
        SQ_BUF_RSRC_WORD1 <- BASE_ADDRESS_HI = 0
                             STRIDE = 0
                             CACHE_SWIZZLE = 0
                             SWIZZLE_ENABLE = 0
        SQ_BUF_RSRC_WORD2 <- 0
        SQ_BUF_RSRC_WORD3 <- DST_SEL_X = SQ_SEL_0
                             DST_SEL_Y = SQ_SEL_0
                             DST_SEL_Z = SQ_SEL_0
                             DST_SEL_W = SQ_SEL_0
                             NUM_FORMAT = BUF_NUM_FORMAT_UNORM
                             DATA_FORMAT = BUF_DATA_FORMAT_INVALID
                             ELEMENT_SIZE = 0
                             INDEX_STRIDE = 0
                             ADD_TID_ENABLE = 0

```


### param_bindless_samplers_and_images 

上传描述符寄存器指定为 SI_SGPR_BINDLESS_SAMPLERS_AND_IMAGES，VS_1

初始激活slot数为1024



![bindless_sampler_and_images](/home/shiji/Downloads/bindless_texture.svg)



## const_and_shader_buffers 

const_and_shader_buffers分为32 个slot， 其中0-15 用来保存shader buffers, 16-31用来保存const_buffers


###  Const Buffers

arb_uniform_buffer_object-rendering

vs经过tgsi_scan扫描处理后可知道vs使用的constbuf数量

const_and_shader_buffers主要保存uniform这种常量，其中当没有ubo使用的时候默认是使用cosnt_and_shader_buffers的第16





![constbuf](/home/shiji/Downloads/constbuf.svg)







###  Shader Buffer



![ssbo](/home/shiji/Downloads/ssbo.svg)



## samplers_and_images

![samplers_and_iamges](/home/shiji/Downloads/sample_and_image.svg)


# Graphics Shaders 

# Vertex Shader 

## blitter vs 

SI_VS_BLIT_SGPRS_POS_COLOR)

|参数索引|参数类型|参数含义|寄存器种类|  MESA下发地址寄存器|备注 |
|----|---|---|---| --- | ---|
| 0  |v4i32*| param_rw_buffers                  | SGPR| SPI_SHADER_USER_DATA_VS_0 |
| 1  |v8i32*| param_bindless_samplers_and_images| SGPR| SPI_SHADER_USER_DATA_VS_1 | 
| 2  |i32   |i16 x1,y1                          | SGPR| SPI_SHADER_USER_DATA_VS_2 |
| 3  |i32   | i16 x2, y2                        | SGPR| SPI_SHADER_USER_DATA_VS_3 |
| 4  |i32   | depth                             | SGPR| SPI_SHADER_USER_DATA_VS_4 | 
| 5  |i32   | color0                            | SGPR| SPI_SHADER_USER_DATA_VS_5 |
| 6  |i32   | color1                            | SGPR| SPI_SHADER_USER_DATA_VS_6 |
| 7  |i32   | color2                            | SGPR| SPI_SHADER_USER_DATA_VS_7 |
| 8  |i32   | color3                            | SGPR| SPI_SHADER_USER_DATA_VS_8 |
| 9  |i32   | vertex_id                         | VGPR| |
| 10 |i32   | instance_id                       | VGPR||
| 11 |i32   | param_vs_prim_id                  | VGPR||
| 12 |i32   | unused                            | VGPR ||

SI_VS_BLIT_SGPRS_POS_TEXCOORD


|参数索引|参数类型|参数含义|寄存器种类|  MESA下发地址寄存器|备注 |
|----|---|---|---| --- | ---|
| 0  |v4i32*| param_rw_buffers                  | SGPR| SPI_SHADER_USER_DATA_VS_0 |
| 1  |v8i32*| param_bindless_samplers_and_images| SGPR| SPI_SHADER_USER_DATA_VS_1 | 
| 2  |i32   |i16 x1,y1                          | SGPR| SPI_SHADER_USER_DATA_VS_2 |
| 3  |i32   | i16 x2, y2                        | SGPR| SPI_SHADER_USER_DATA_VS_3 |
| 4  |i32   | depth                             | SGPR| SPI_SHADER_USER_DATA_VS_4 | 
| 5  |f32   | texcoord.x1                       | SGPR| SPI_SHADER_USER_DATA_VS_5 |
| 6  |f32   | texcoord.y1                       | SGPR| SPI_SHADER_USER_DATA_VS_6 |
| 7  |f32   | texcoord.x2                       | SGPR| SPI_SHADER_USER_DATA_VS_7 |
| 8  |f32   | texcoord.y2                       | SGPR| SPI_SHADER_USER_DATA_VS_8 |
| 9  |i32   | texcoord.z                        | VGPR| |
| 10  |i32  | texcoord.w                        | VGPR| |
| 11 |i32   | instance_id                       | VGPR||
| 12 |i32   | param_vs_prim_id                  | VGPR||
| 13 |i32   | unused                            | VGPR ||



## Vertex Shader As VS

###  VS prolog 函数声明

si_build_vs_prolog_function

vs_prolog生成的条件：
1. 存在输入input变量
2. 非blit vs

|参数索引|参数类型|参数含义|寄存器种类|  MESA下发地址寄存器|备注 |
|----|---|---|---| --- | ---|
| 0 |v4i32* | param_rw_buffers |  SGPR | SPI_SHADER_USER_DATA_VS_0 
| 1 |v8i32* | param_bindless_samplers_and_images|  SGPR |SPI_SHADER_USER_DATA_VS_1 
| 2 |f32* | const_and_shader_buffers|  SGPR |SPI_SHADER_USER_DATA_VS_2 |无UBO或SSBO
|  |v4i32* | const_and_shader_buffers|  SGPR |SPI_SHADER_USER_DATA_VS_2 |
| 3 |v8i32* | samplers_and_images  | SGPR |SPI_SHADER_USER_DATA_VS_3 |
| 4 |i32 | vs_state_bits| SGPR  |SPI_SHADER_USER_DATA_VS_4  |
| 5 |i32 | base_vertex| SGPR |SPI_SHADER_USER_DATA_VS_5 |
| 6 |i32 | start_instance| SGPR| SPI_SHADER_USER_DATA_VS_6 | 
| 7 |i32 | draw_id| SGPR| SPI_SHADER_USER_DATA_VS_7 
|  8|v4i32* | param_vertex_buffers | SGPR| SPI_SHADER_USER_DATA_VS_8 
| 9 |i32 | vertex_id| VGPR| |
| 10 |i32 | instance_id| VGPR ||
| 11 |i32 | param_vs_prim_id| VGPR ||
| 12 |i32 | unused | VGPR ||

返回值为structure类型，其中14位置为input
Structure <0,1,2,3,4,5，6,7,8,9,10,11,12,13,14..N>

### VS 带stream out main函数声明

函数声明创建在create_function

|参数位置|参数类型|参数含义|寄存器种类|  MESA下发地址寄存器|备注 |
|----|---|---|---| --- | ---|
| 0 |v4i32* | param_rw_buffers |  SGPR | SPI_SHADER_USER_DATA_VS_0 
| 1 |v8i32* | param_bindless_samplers_and_images|  SGPR |SPI_SHADER_USER_DATA_VS_1 
| 2 |f32* | const_and_shader_buffers|  SGPR |SPI_SHADER_USER_DATA_VS_2 |无UBO或SSBO
|  |v4i32* | const_and_shader_buffers|  SGPR |SPI_SHADER_USER_DATA_VS_2 |
| 3 |v8i32* | samplers_and_images  | SGPR |SPI_SHADER_USER_DATA_VS_3 |
| 4 |i32 | vs_state_bits| SGPR  |SPI_SHADER_USER_DATA_VS_4  |
| 5 |i32 | base_vertex| SGPR |SPI_SHADER_USER_DATA_VS_5 |
| 6 |i32 | start_instance| SGPR| SPI_SHADER_USER_DATA_VS_6 | 
| 7 |i32 | draw_id| SGPR| SPI_SHADER_USER_DATA_VS_7 
|   |v4i32* | param_vertex_buffers | SGPR| SPI_SHADER_USER_DATA_VS_8 
|   |i32 | param_streamout_config | SGPR| | 
|  |i32 | param_streamout_write_index | SGPR| | 
|  |i32 | param_streamout_offset[0] | SGPR| | 
|  |i32 | param_streamout_offset[1] | SGPR| | 
|  |i32 | param_streamout_offset[2] | SGPR| | 
|  |i32 | param_streamout_offset[3] | SGPR| | 
| |i32 | vertex_id| VGPR|内置变量 | gl_Vertexid
|  |i32 | instance_id| VGPR |内置变量|gl_Instanceid
|  |i32 | param_vs_prim_id| VGPR |内置变量|gl_PrimitiveID,用于特殊导出情形
|  |i32 | unused | VGPR ||
| |i32 | num_input[0]| VGPR| | Vertex load indices
|  |i32 | num_input[1]| VGPR| |
|  |...|...|...|...|...|
| |i32| num_input[n] | VGPR| |
| end |void | returns | void | |函数返回值

以下为带streamout一个输出且stride为0的main LLVM IR

```
; ModuleID = 'mesa-shader'
target datalayout = "e-p:64:64-p1:64:64-p2:32:32-p3:32:32-p4:64:64-p5:32:32-p6:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64-S32-A5"
target triple = "amdgcn--"

define amdgpu_vs void @main([0 x <4 x i32>] addrspace(6)* inreg noalias dereferenceable(18446744073709551615), [0 x <8 x i32>] addrspace(6)* inreg noalias dereferenceable(18446744073709551615), [0 x <4 x i32>] addrspace(6)* inreg noalias dereferenceable(18446744073709551615), [0 x <8 x i32>] addrspace(6)* inreg noalias dereferenceable(18446744073709551615), i32 inreg, i32 inreg, i32 inreg, i32 inreg, [0 x <4 x i32>] addrspace(6)* inreg noalias dereferenceable(18446744073709551615), i32 inreg, i32 inreg, i32 inreg, i32, i32, i32, i32, i32) #0 {
main_body:
}

attributes #0 = { "no-signed-zeros-fp-math"="true" }

```

### VS 无stream out main函数声明

函数声明创建在create_function

|参数位置|参数类型|参数含义|寄存器种类|  MESA下发地址寄存器|备注 |
|----|---|---|---| --- | ---|
| 0 |v4i32* | param_rw_buffers |  SGPR | SPI_SHADER_USER_DATA_VS_0 
| 1 |v8i32* | param_bindless_samplers_and_images|  SGPR |SPI_SHADER_USER_DATA_VS_1 
| 2 |f32* | const_and_shader_buffers|  SGPR |SPI_SHADER_USER_DATA_VS_2 |无UBO或SSBO
|  |v4i32* | const_and_shader_buffers|  SGPR |SPI_SHADER_USER_DATA_VS_2 |
| 3 |v8i32* | samplers_and_images  | SGPR |SPI_SHADER_USER_DATA_VS_3 |
| 4 |i32 | vs_state_bits| SGPR  |SPI_SHADER_USER_DATA_VS_4  |
| 5 |i32 | base_vertex| SGPR |SPI_SHADER_USER_DATA_VS_5 |
| 6 |i32 | start_instance| SGPR| SPI_SHADER_USER_DATA_VS_6 | 
| 7 |i32 | draw_id| SGPR| SPI_SHADER_USER_DATA_VS_7 
| 8 |v4i32* | param_vertex_buffers | SGPR| SPI_SHADER_USER_DATA_VS_8 
| 9 |i32 | vertex_id| VGPR|内置变量 | gl_Vertexid
| 10 |i32 | instance_id| VGPR |内置变量|gl_Instanceid
| 11 |i32 | param_vs_prim_id| VGPR |内置变量|gl_PrimitiveID,用于特殊导出情形
| 12 |i32 | unused | VGPR ||
| 13|i32 | num_input[0]| VGPR| | Vertex load indices
| 14 |i32 | num_input[1]| VGPR| |
| ... |...|...|...|...|...|
| n |i32| num_input[n] | VGPR| |
| end |void | returns | void | |函数返回值

### param_vertex_buffers

参考之前vbo文档

### vs_state_bits/ base_vertex / start_instance

跟随每次draw-> si_emit_draw_packts下发


### vertex_id 

当draw时， 表示元素的索引，依次递增。

根据不同的draw,会有不同的情况。

#### DrawArrays*类

vertex_id(i) = *first* + i

* indirect 类

1. 首先获取indirect buffer地址。
通过解析PKT3_SET_BASE pm4获取i
2. 如果不是multidraw则解析PKT3_DRAW_INDIRECT获取offset
3. 如果时multidraw检查PKT3_DRAW_INDIRECT_MULTI获取当前bufer
4. 之后从buffer获取first变量。

* 非indirect绘制

1. first等同base_vertex参数

2. 固件还可根据PKT3_DRAW_INDEX_AUTO,获取draw的count参数。

#### DrawElement* 类

在兼容模式下， 如果没有ebo, 则：
vertex_id(i) = baservertex + indices[i]

在core模式：
vertexc_id(i) = baservertex + *pbo(indices+i);

针对core模式：

* indirect类

1. 和drawArray不同的时，这里会下发一个PKT3_INDEX_BASE,寄存器表述index buffer地址。
2. 对于multidraw, 检查的是PKT3_DRAW_INDEX_INDIRECT_MULTI寄存器获取buffer相关信息。
3. 其他相同

* 非indirect类

1. 此时first = basevertex 参数


### instance_id

使用draw时， 表示当前图元号。由glDraw\*的参数instance指定。

范围由 VGT_NUM_INSTANCES确定
解析PKT3_NUM_INSTANCES 获取实例数量

### param_vs_prim_id

从0递增,
这个参数只有在以下情况才能用到：

1. 没有tcs,tes, gs,
2. fs 输入变量有 gl_PrimitiveID

在这种情况，会使用传入的primid， 并导出primid给fs使用

参见规范

> • The variable gl_PrimitiveID is filled with the number of primitives processed by the drawing command which generated the input vertices. The
first primitive generated by a drawing command is numbered zero, and the
primitive ID counter is incremented after every individual point, line, or triangle primitive is processed. Restarting a primitive topology using the primitive restart index has no effect on the primitive ID counter.

### num_input[n]

通过prolog返回值给定

测试这块，对于prolog, hs ,es 基本都在与shader这块

##  Vertex Shader  as LS

### LS prolog 函数声明

si_build_vs_prolog_function


|参数索引|参数类型|参数含义|寄存器种类|  MESA下发地址寄存器|备注 |
|----|---|---|---| --- | ---|
| 0 |v4i32* | param_rw_buffers |  SGPR | SPI_SHADER_USER_DATA_LS_0 
| 1 |v8i32* | param_bindless_samplers_and_images|  SGPR |SPI_SHADER_USER_DATA_LS_1 
| 2 |f32* | const_and_shader_buffers|  SGPR |SPI_SHADER_USER_DATA_LS_2 |无UBO或SSBO
|  |v4i32* | const_and_shader_buffers|  SGPR |SPI_SHADER_USER_DATA_LS_2 |
| 3 |v8i32* | samplers_and_images  | SGPR |SPI_SHADER_USER_DATA_LS_3 |
| 4 |i32 | vs_state_bits| SGPR  |SPI_SHADER_USER_DATA_LS_4  |
| 5 |i32 | base_vertex| SGPR |SPI_SHADER_USER_DATA_LS_5 | | 6 |i32 | start_instance| SGPR| SPI_SHADER_USER_DATA_LS_6 | 
| 7 |i32 | draw_id| SGPR| SPI_SHADER_USER_DATA_LS_7 
|  8|v4i32* | param_vertex_buffers | SGPR| SPI_SHADER_USER_DATA_LS_8 
| 9 |i32 | vertex_id| VGPR| |
| 10 |i32 | instance_id| VGPR ||
| 11 |i32 | param_vs_prim_id| VGPR ||
| 12 |i32 | unused | VGPR ||

这里和As VS不同的是这里用的LS寄存器， 通过侦测是否使用tes 来调用si_shader_change_notify 改变了vertex_shader的寄存器。

返回值为structure类型，其中14位置为input
Structure <0,1,2,3,4,5，6,7,8,9,10,11,12,13,14..N>


### LS main函数声明

函数声明创建在create_function

|参数位置|参数类型|参数含义|寄存器种类|  MESA下发地址寄存器|备注 |
|----|---|---|---| --- | ---|
| 0 |v4i32* | param_rw_buffers |  SGPR | SPI_SHADER_USER_DATA_LS_0 
| 1 |v8i32* | param_bindless_samplers_and_images|  SGPR |SPI_SHADER_USER_DATA_LS_1 
| 2 |f32* | const_and_shader_buffers|  SGPR |SPI_SHADER_USER_DATA_LS_2 |无UBO或SSBO
|  |v4i32* | const_and_shader_buffers|  SGPR |SPI_SHADER_USER_DATA_LS_2 |
| 3 |v8i32* | samplers_and_images  | SGPR |SPI_SHADER_USER_DATA_LS_3 |
| 4 |i32 | vs_state_bits| SGPR  |SPI_SHADER_USER_DATA_LS_4  |
| 5 |i32 | base_vertex| SGPR |SPI_SHADER_USER_DATA_LS_5 |
| 6 |i32 | start_instance| SGPR| SPI_SHADER_USER_DATA_LS_6 | 
| 7 |i32 | draw_id| SGPR| SPI_SHADER_USER_DATA_LS_7 
| 8 |v4i32* | param_vertex_buffers | SGPR| SPI_SHADER_USER_DATA_LS_8 
| 9 |i32 | vertex_id| VGPR|内置变量 | gl_Vertexid
| 10 |i32 | param_rel_auto_id | VGPR |内置变量|gl_Instanceid
| 11 |i32 | instance_id| VGPR |内置变量|gl_Primitiveid
| 12 |i32 | unused | VGPR ||
| 13|i32 | num_input[0]| VGPR| | Vertex load indices
| ... |...|...|...|...|...|
| n |i32| num_input[n] | VGPR| |
| end |void | returns | void | |函数返回值


## Vertex Shader As ES

### ES prolog 函数声明

si_build_vs_prolog_function

|参数位置|参数类型|参数含义|寄存器种类|  MESA下发地址寄存器|备注 |
|----|---|---|---| --- | ---|
| 0 |v4i32* | param_rw_buffers |  SGPR | SPI_SHADER_USER_DATA_ES_0 
| 1 |v8i32* | param_bindless_samplers_and_images|  SGPR |SPI_SHADER_USER_DATA_ES_1 
| 2 |f32* | const_and_shader_buffers|  SGPR |SPI_SHADER_USER_DATA_ES_2 |无UBO或SSBO
|  |v4i32* | const_and_shader_buffers|  SGPR |SPI_SHADER_USER_DATA_ES_2 |
| 3 |v8i32* | samplers_and_images  | SGPR |SPI_SHADER_USER_DATA_ES_3 |
| 4 |i32 | vs_state_bits| SGPR  |SPI_SHADER_USER_DATA_ES_4  |
| 5 |i32 | base_vertex| SGPR |SPI_SHADER_USER_DATA_ES_5 |
| 6 |i32 | start_instance| SGPR| SPI_SHADER_USER_DATA_ES_6 | 
| 7 |i32 | draw_id| SGPR| SPI_SHADER_USER_DATA_ES_7 
| 8 |v4i32* | param_vertex_buffers | SGPR| SPI_SHADER_USER_DATA_ES_8 
| 9 |i32 | param_es2gs_offset| SGPR|内置变量 | 
| 10 |i32 | vertex_id| VGPR| |
| 11 |i32 | instance_id| VGPR ||
| 13 |i32 | param_vs_prim_id| VGPR ||
| 14 |i32 | unused | VGPR ||


这里和As VS不同的是这里用的LS寄存器， 通过侦测是否使用tes 来调用si_shader_change_notify 改变了vertex_shader的寄存器。

返回值为structure类型，其中14位置为input
Structure <0,1,2,3,4,5，6,7,8,9,10,11,12,13,14,15..N>


### ES main函数声明

函数声明创建在create_function

|参数位置|参数类型|参数含义|寄存器种类|  MESA下发地址寄存器|备注 |
|----|---|---|---| --- | ---|
| 0 |v4i32* | param_rw_buffers |  SGPR | SPI_SHADER_USER_DATA_ES_0 
| 1 |v8i32* | param_bindless_samplers_and_images|  SGPR |SPI_SHADER_USER_DATA_ES_1 
| 2 |f32* | const_and_shader_buffers|  SGPR |SPI_SHADER_USER_DATA_ES_2 |无UBO或SSBO
|  |v4i32* | const_and_shader_buffers|  SGPR |SPI_SHADER_USER_DATA_ES_2 |
| 3 |v8i32* | samplers_and_images  | SGPR |SPI_SHADER_USER_DATA_ES_3 |
| 4 |i32 | vs_state_bits| SGPR  |SPI_SHADER_USER_DATA_ES_4  |
| 5 |i32 | base_vertex| SGPR |SPI_SHADER_USER_DATA_ES_5 |
| 6 |i32 | start_instance| SGPR| SPI_SHADER_USER_DATA_ES_6 | 
| 7 |i32 | draw_id| SGPR| SPI_SHADER_USER_DATA_ES_7 
| 8 |v4i32* | param_vertex_buffers | SGPR| SPI_SHADER_USER_DATA_ES_8 
| 9 |i32 | param_es2gs_offset| SGPR|内置变量 | 
| 10 |i32 | vertex_id| VGPR| |
| 11 |i32 | instance_id| VGPR ||
| 13 |i32 | param_vs_prim_id| VGPR ||
| 14 |i32 | unused | VGPR ||
| 15|i32 | num_input[0]| VGPR| | Vertex load indices
| ... |...|...|...|...|...|
| n |i32| num_input[n] | VGPR| |
| end |void | returns | void | |函数返回值



# Tessellation Control Shader  

tcs为可选着色


## TCS As HS

### HS main

|参数位置|参数类型|参数含义|寄存器种类|  MESA下发地址寄存器|备注 |
|----|---|---|---| --- | ---|
| 0 | v4i32* | param_rw_buffers                  | SGPR | SPI_SHADER_USER_DATA_HS_0 
| 1 | v8i32* | param_bindless_samplers_and_images| SGPR | SPI_SHADER_USER_DATA_HS_1 
| 2 | f32*   | const_and_shader_buffers          | SGPR | SPI_SHADER_USER_DATA_HS_2 |无UBO或SSBO
|   | v4i32* | const_and_shader_buffers          | SGPR | SPI_SHADER_USER_DATA_HS_2 |
| 3 | v8i32* | samplers_and_images               | SGPR | SPI_SHADER_USER_DATA_HS_3 |
| 4 | i32    | param_tcs_offchip_layout          | SGPR | SPI_SHADER_USER_DATA_HS_4 | TCS输出在离片外缓冲区中的布局                      |
|   |        |                                   |      |                            | # 6位                                              |
|   |        |                                   |      |                            | [0:5] = 每个线程组的补丁数，最大 = NUM_PATCHES（40）
|   |        |                                   |      |                            | # 6位
|   |        |                                   |      |                            | [6:11] = 每个补丁的输出顶点数，最大 = 32
|   |        |                                   |      |                            | # 20位
|   |        |                                   |      |                            | [12:31] = 缓冲区中每个补丁属性的偏移（以字节为单位）。
|   |        |                                   |      |                            | 最大 = NUM_PATCHES*32*32*16
| 5 | i32    | param_tcs_out_lds_offsets         | SGPR | SPI_SHADER_USER_DATA_HS_5  | TCS输出和TCS补丁输出在LDS中的偏移：
|	|		 |									 |		|						     | [0:15] = TCS输出补丁0的偏移 / 16，最大 = NUM_PATCHES * 32 * 32
|	|		 |									 |		|						     | [16:31] = 用于每个补丁的TCS输出补丁0的偏移 / 16
|	|		 |									 |		|						     |            最大 = (NUM_PATCHES + 1) * 32*32
|	|		 |									 |		|						     |
| 6 | i32    | param_tcs_out_lds_layout          | SGPR | SPI_SHADER_USER_DATA_HS_6  | TCS输出 / TES输入的布局：
|   |        |                                   |      |                            | [0:12] = 输出补丁之间的跨度，以DW（双字，32位字）为单位，num_outputs * num_vertices * 4
|   |        |                                   |      |                            |          最大值 = 32*32*4 + 32*4
|   |        |                                   |      |                            | [13:18] = gl_PatchVerticesIn，最大值 = 32
| 7 | i32    | param_vs_state_bits               | SGPR | SPI_SHADER_USER_DATA_HS_7  |VS（顶点着色器）状态和LS（局部着色器）输出 / TCS（细分控制着色器）输入的布局信息：
|   |        |  |                                |      |                            |  [0] = 顶点颜色是否被裁剪
|   |        |  |                                |      |                            |  [1] = 是否使用索引
|   |        |  |                                |      |                            |  [8:20] = 输出补丁之间的跨度，以DW（双字，32位字）为单位 = num_inputs * num_vertices * 4
|   |        |  |                                |      |                            |           最大值 = 32*32*4 + 32*4
|   |        |  |                                |      |                            |  [24:31] = 顶点之间的跨度，以DW为单位 = num_inputs * 4
|   |        |  |                                |      |                            |            最大值 = 32*4
| 8 | i32    | param_tcs_offchip_offset          | SGPR | SPI_SHADER_USER_DATA_HS_8  | 外部内存(主要指L2 L3 缓存) 偏移
| 9 | i32    | param_tcs_factor_offset           | SGPR | SPI_SHADER_USER_DATA_HS_9  |  向tes传递的数据偏移
| 10 |i32    | tcs_patch_id                      | VGPR |                            |  细分面id
| 11 |i32    | tcs_rel_ids                       | VGPR |                            |  相对细分面id 


### TCS的内建变量

* in int gl_PatchVerticesIn;

根据glsl4.5 规范

> The variable gl_PatchVerticesIn is available only in the tessellation control and evaluation languages. It
is an integer specifying the number of vertices in the input patch being processed by the shader. A single
tessellation control or evaluation shader can read patches of differing sizes, so the value of
gl_PatchVerticesIn may differ between patches.

由glPatchParameter 指定

处理可根据si_shader.c:si_load_system_value:中对TGSI_SEMANTIC_VERTICESIN 中的处理, 具体可由param_tcs_out_lds_layout的处理

* in int gl_PrimitiveID

根据OpenGL4.5 规范
> The variable gl_PrimitiveID is filled with the number of primitives processed by the drawing command which generated the input vertices. The
first primitive generated by a drawing command is numbered zero, and the
primitive ID counter is incremented after every individual point, line, or triangle primitive is processed. Restarting a primitive topology using the primitive restart index has no effect on the primitive ID counter.

具体值参考si_shader.c:si_load_system_value:中对	TGSI_SEMANTIC_PRIMID的处理， 为tcs_patch_id

* gl_InvocationID

> The variable gl_InvocationID holds an invocation number for the current tessellation control shader invocation. Tessellation control shaders are
invoked once per output patch vertex, and invocations are numbered beginning with zero

具体值参考si_shader.c:si_load_system_value:中对	TGSI_SEMANTIC_INVOCATIONID的处理， 


函数返回<10个SGPRi32，11个VGPRf32> ,用作epilog参数

```c++
define amdgpu_hs <{ i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, float, float, float, float, float, float, float, float, float, float, float }> @main([0 x <4 x i32>] addrspace(6)* inreg noalias dereferenceable(18446744073709551615), [0 x <8 x i32>] addrspace(6)* inreg noalias dereferenceable(18446744073709551615), [0 x <4 x i32>] addrspace(6)* inreg noalias dereferenceable(18446744073709551615), [0 x <8 x i32>] addrspace(6)* inreg noalias dereferenceable(18446744073709551615), i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32, i32) #0 {
main_body:
}
attributes #0 = { "amdgpu-max-work-group-size"="0x80" "no-signed-zeros-fp-math"="true" }

```

### epilog 

si_build_tcs_epilog_function 

|参数位置|参数类型|参数含义|寄存器 | mesa 下发寄存器 | 备注 |
|---|---|---|---| ---| --- | 
| 0 |intptr(i32/i64) | param_rw_buffers| SGPR | SPI_SHADER_USER_DATA_HS_0 | 
| 1 |intptr | param_bindless_sample    | SGPR | SPI_SHADER_USER_DATA_HS_1 | 
| 2 |intptr | const_and_shader_buffers | SGPR | SPI_SHADER_USER_DATA_HS_2 | 
| 3 |intptr | samplers_and_images      | SGPR | SPI_SHADER_USER_DATA_HS_3 |
| 4 |i32    | param_tcs_offchip_layout | SGPR | SPI_SHADER_USER_DATA_HS_4 |
| 5 |i32    | gap                      | SGPR | SPI_SHADER_USER_DATA_HS_5 |
| 6 |i32    | param_tcs_out_lds_layout | SGPR | SPI_SHADER_USER_DATA_HS_6 |
| 7 |i32    | ??                       | SGPR | SPI_SHADER_USER_DATA_HS_7 |
| 8 |i32    | param_tcs_offchip_offset | SGPR | SPI_SHADER_USER_DATA_HS_8 | 
| 9 |i32    | param_tcs_factor_offset  | SGPR | SPI_SHADER_USER_DATA_HS_9 | 
| 10 |i32   | gap                      | VGPR |                           | 
| 11 |i32   | gap                      | VGPR | 						  | 
| 12 |i32   | tess_factors_idx         | VGPR |                           |相对细分面id         |
| 13 |i32   | InvocationID             | VGPR |                           |在这个面内的顶点id   |
| 14 |i32   | lds_offset               | VGPR |                           |本地共享内存偏移     |
| 15 |i32   | factors[0]               | VGPR |                           |gl_TessLevelOuter[0] |
| 16 |i32   | factors[1]               | VGPR |                           |gl_TessLevelOuter[1] |
| 17 |i32   | factors[2]               | VGPR |                           |gl_TessLevelOuter[2] |
| 18 |i32   | factors[3]               | VGPR |                           |gl_TessLevelOuter[3] |
| 19 |i32   | factors[4]               | VGPR |                           |gl_TessLevelInner[0] |
| 20 |i32   | factors[5]               | VGPR |                           |gl_TessLevelInner[1] |

```
define amdgpu_hs void @tcs_epilog(i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32 inreg, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) #0 {
main_body:
}

```


# Tessellation Evaluation Shader


## TES as VS  

### main

create_function

|参数位置|参数类型|参数含义|寄存器 | mesa 下发寄存器 | 备注
|----|---|---|---| --- | --- |
| 0  | v4i32* | param_rw_buffers         | SGPR | SPI_SHADER_USER_DATA_VS_0 |                |
| 1  | v8i32* | param_bindless_sample    | SGPR | SPI_SHADER_USER_DATA_VS_1 |               |
| 2  | f32*   | const_and_shader_buffers | SGPR | SPI_SHADER_USER_DATA_VS_2 |               |
|    | v4i32* | const_and_shader_buffer  | SGPR | SPI_SHADER_USER_DATA_VS_2 |               |
| 3  | v8i32* | samplers_and_images      | SGPR | SPI_SHADER_USER_DATA_VS_3 |                |
| 4  | i32    | parm_vs_state_bits       | SGPR | SPI_SHADER_USER_DATA_VS_4 |                         |
| 5  | i32    | param_tcs_offchip_layout | SGPR | SPI_SHADER_USER_DATA_VS_5 | 同tcs|
| 6  | i32    | param_tes_offchip_addr   | SGPR | SPI_SHADER_USER_DATA_VS_6 | 外部内存地址  |
| 7  | i32    | ??                       | SGPR | SPI_SHADER_USER_DATA_VS_7 | 未知  |
| 8  | i32    | param_tcs_offchip_offset | SGPR | SPI_SHADER_USER_DATA_VS_8 | 同tcs   |
| 9  | f32    | param_tes_u              | VGPR |                           | gl_TessCoord.u |
| 10 | f32    | param_tes_v              | VGPR |                           | gl_TessCoord.v |  
| 11 | i32    | param_tes_rel_patch_id   | VGPR |                           | 同tcs          |  
| 12 | i32    | tes_patch_id             | VGPR |                           | 同tes          | 


#### 内建变量

## TES as ES 

### main

|参数位置|参数类型|参数含义|寄存器| 备注| 
|----|---|---|---| --- | 
| 0  | v4i32* | param_rw_buffers         | SGPR | SPI_SHADER_USER_DATA_ES_0 | 同上 |   
| 1  | v8i32* | param_bindless_sample    | SGPR | SPI_SHADER_USER_DATA_ES_1 | 同上 |     
| 2  | f32*   | const_and_shader_buffers | SGPR | SPI_SHADER_USER_DATA_ES_2 | 同上 | 
|    | v4i32* | const_and_shader_buffer  | SGPR | SPI_SHADER_USER_DATA_ES_2 | 同上 | 
| 3  | v8i32\* | samplers_and_images     | SGPR | SPI_SHADER_USER_DATA_ES_3 | 同上 | 
| 4  | i32    | parm_vs_state_bits       | SGPR | SPI_SHADER_USER_DATA_ES_4 | 同上 | 
| 5  | i32    | param_tcs_offchip_layout | SGRP | SPI_SHADER_USER_DATA_ES_5 | 同上 |   
| 6  | i32    | param_tes_offchip_addr   |      | SPI_SHADER_USER_DATA_ES_6 | 同上 |  
| 7  | i32    | param_tcs_offchip_offset |      | SPI_SHADER_USER_DATA_ES_7 | 同上 |  
| 8  | i32    | ?? 						 | SGPR | SPI_SHADER_USER_DATA_ES_8 | 同上 | 
| 9  | i32    | param_es2gs_offset       |      | SPI_SHADER_USER_DATA_ES_9 | esgs_ring在RW buffer中SI_ES_RING_ESGS buffer的偏移量 | 
| 10 | f32    | param_tes_u              | VGPR |                           | 同as Vs
| 11 | f32    | param_tes_v              | VGPR |                           | 同as Vs    
| 12 | i32    | param_tes_rel_patch_id   | VGPR |                           | 相对细分面id 
| 13 | i32    | tes_patch_id             | VGPR |                           |



### 内建变量

* in int gl_PatchVerticesIn;
* in int gl_PrimitiveID;
* patch in float gl_TessLevelOuter[4];
* patch in float gl_TessLevelInner[2];

来源tcs


* in vec3 gl_TessCoord;

根据glsl 4.5 specs: 
 > The variable gl_TessCoord is available only in the tessellation evaluation language. It specifies a threecomponent (u,v,w) vector identifying the position of the vertex being processed by the shader relative to the primitive being tessellated. Its values will obey the properties 
	gl_TessCoord.x == 1.0 – (1.0 – gl_TessCoord.x) // two operations performed
	gl_TessCoord.y == 1.0 – (1.0 – gl_TessCoord.y) // two operations performed
	gl_TessCoord.z == 1.0 – (1.0 – gl_TessCoord.z) // two operations performed

实际上就是重心坐标
见si_shader.c:si_load_system_value:对TGSI_SEMANTIC_TESSCOORD 的处理。

# Geometry Shader

## GS prolog

产生条件为: **只有图元为TRAIANGLE_STRIP_ADJACENCY 且没有tes**

|参数位置|参数类型|参数含义|寄存器种类|  MESA下发地址寄存器|备注 |
|----|---|---|---| --- | ---|
| 0  | v4i32* | param_rw_buffers                  | SGPR | SPI_SHADER_USER_DATA_GS_0 ||
| 1  | v8i32* | param_bindless_samplers_and_images| SGPR | SPI_SHADER_USER_DATA_GS_1 ||
| 2  | f32*   | const_and_shader_buffers          | SGPR | SPI_SHADER_USER_DATA_GS_2 | 无UBO或SSBO|
|    | v4i32* | const_and_shader_buffers          | SGPR | SPI_SHADER_USER_DATA_GS_2 ||
| 3  | v8i32* | samplers_and_images               | SGPR | SPI_SHADER_USER_DATA_GS_3 ||
| 4  | i32    | param_gs2vs_offset                | SGPR | 由ES程序决定| 输出数据在 SI_RING_GSVS buffer的偏移 |
| 5  | i32    | param_gs_wave_id                  | SGPR | | Wave buffer slot number (0-9) |
| 6  | i32    | gs_vtx_offset0                    | VGPR |                           | 第0个顶点偏移 |
| 7  | i32    | gs_vtx_offset1                    | VGPR |                           |  -  |
| 8  | i32    | gs_prim_id                        | VGPR |                           | gl_PrimitiveIDIn|
| 9  | i32    | gs_vtx_offset2                    | VGPR |                           |  - |
| 10 | i32    | gs_vtx_offset3                    | VGPR |                           |  -|
| 11 | i32    | gs_vtx_offset4                    | VGPR |                           |  -|
| 12 | i32    | gs_vtx_offset5                    | VGPR |                           |  - |
| 13 | i32    | gs_invocation_id                  | VGPR |                           | 实例化gs的id|

#### 参数解释

* gs_vtx_offset* 

由于gs的输入图元限制，最多只可能有6个顶点的图元。
每个参数表示对应图元的顶点在esgs_ring的偏移。
再加上gs2_vs_offset即可获得ring buffer中的顶点数据。


#### built-in variables

* in int gl_PrimitiveIDIn;
根据OpengGL 4.5 第418 page规范
> It is filled with the number of primitives processed by the drawing command which
generated the input vertices. The first primitive generated by a drawing command
is numbered zero, and the primitive ID counter is incremented after every individual point, line, or triangle primitive is processed. For triangles drawn in point or
line mode, the primitive ID counter is incremented only once, even though multiple
points or lines may eventually be drawn. Restarting a primitive topology using the
primitive restart index has no effect on the primitive ID counter

可见同vs primid 

* in int gl_InvocationID;

表示实例化gs的运行id

* out int gl_PrimitiveID;

参考  si_llvm_export_vs 对内建变量的处理。

表示输出图元的id， 从0递增, 用来提到draw产生的prim id.

* out int gl_Layer;

用于层化渲染，在多层纹理使用，如二维纹理数组， 三维纹理。

* out int gl_ViewportIndex;

用于明确视口的id , 用于多个视口draw。

## GS main

函数声明创建在create_function

|参数位置|参数类型|参数含义|寄存器种类|  MESA下发地址寄存器|备注 |
|----|---|---|---| --- | ---|
| 0  | v4i32* | param_rw_buffers                  | SGPR | SPI_SHADER_USER_DATA_GS_0 |
| 1  | v8i32* | param_bindless_samplers_and_images| SGPR | SPI_SHADER_USER_DATA_GS_1 | 
| 2  | f32*   | const_and_shader_buffers          | SGPR | SPI_SHADER_USER_DATA_GS_2 |无UBO或SSBO
|    | v4i32* | const_and_shader_buffers          | SGPR | SPI_SHADER_USER_DATA_GS_2 |
| 3  | v8i32* | samplers_and_images               | SGPR | SPI_SHADER_USER_DATA_GS_3 |
| 4  | i32    | param_gs2vs_offset                | SGPR |  | 输出数据在 SI_RING_GSVS buffer的偏移 
| 5  | i32    | param_gs_wave_id                  | SGPR |  | 当前wave id
| 6  | i32    | gs_vtx_offset0                    | VGPR |                           | 第0个顶点偏移 
| 7  | i32    | gs_vtx_offset1                    | VGPR |                           |  -  
| 8  | i32    | gs_prim_id                        | VGPR |                           | gl_PrimitiveIDIn
| 9  | i32    | gs_vtx_offset2                    | VGPR |                           |  - 
| 10 | i32    | gs_vtx_offset3                    | VGPR |                           |  -
| 11 | i32    | gs_vtx_offset4                    | VGPR |                           |  -
| 12 | i32    | gs_vtx_offset5                    | VGPR |                           |  - 
| 13 | i32    | gs_invocation_id                  | VGPR |                           | 实例化gs的id



## GS copy shader(vs)

同 Vertex As Vs main 无num_input参数


# Fragment Shader

## PS prolog 

只有si_need_ps_prolog条件成立才有prolog
si_need_ps_prolog的条件如下:

1. 前后颜色读取

这个主要发生在fs中使用了gl_Color

2. 多重采样，逐样本采样

* force_persp_sample_interp 强制每个样本插值
* force_linear_sample_interp
* force_linear_center_interp 


3. 插值优化

* bc_optimize_for_persp 
* bc_optimize_for_linear 

通过 R_0286D8_SPI_PS_IN_CONTROL 寄存器控制 

4. 启用图案掩码

* poly_stipple 

5. 像素着色器迭代次数大于1, 同时启用样本掩码

* samplemask_log_ps_iter


|参数索引|参数类型|参数含义|寄存器种类|  MESA下发地址寄存器|备注 |
|----|---|---|---| --- | ---|
| 0 | v4i32* | param_rw_buffers | SGPR | SPI_SHADER_USER_DATA_PS_0 ||
| 1 | v8i32* | param_bindless_samplers_and_images| SGPR | SPI_SHADER_USER_DATA_PS_1 ||
| 2 | f32*| const_and_shader_buffers|  SGPR |SPI_SHADER_USER_DATA_PS_2 |无UBO或SSBO|
|   | v4i32* | const_and_shader_buffers|  SGPR |SPI_SHADER_USER_DATA_PS_2 ||
| 3 | v8i32* | samplers_and_images  | SGPR |SPI_SHADER_USER_DATA_PS_3 ||
|||||||
| 4 | f32 | SI_PARAM_ALPHA_REF | SGPR  |SPI_SHADER_USER_DATA_PS_4 |alpha测试值|
| 5 | i32 | SI_PARAM_PRIM_MASK | SGPR | | 重心坐标优化标志，图元掩码，属性lds偏移|
|||||||
| 6 | v2i32 | SI_PARAM_PERSP_SAMPLE | VGPR |  |多重透视采样插值重心坐标|
| 7 | v2i32 | SI_PARAM_PERSP_CENTER | VGPR |  |透视单一采样插值重心坐标|
| 8 | v2i32 | SI_PARAM_PERSP_CENTROID | VGPR |  | 透视单一重心采样插值重心坐标|
| 9 | v3i32 | SI_PARAM_PERSP_PULL_MODEL | VGPR |  |pull-mode 插值重心坐标|
| 10 | v2i32 | SI_PARAM_LINEAR_SAMPLE | VGPR |  | 线性多重采样插值重心坐标 |  
| 11 | v2i32 | SI_PARAM_LINEAR_CENTER | VGPR |  |线性单一采样插值重心坐标|
| 12 | v2i32 | SI_PARAM_LINEAR_CENTROID | VGPR |  |线性单一重心采样插值重心坐标 |
| 13 | f32 | SI_PARAM_LINE_STIPPLE_TEX | VGPR |  | 线 纹理 产生|
|\||||||
| 14 |f32 | SI_PARAM_POS_X_FLOAT frag_pos[0]| VGPR | |片段坐标x|
| 15 |f32 | SI_PARAM_POS_Y_FLOAT frag_pos[1]| VGPR | |片段坐标y|
| 16 |f32 | SI_PARAM_POS_Z_FLOAT frag_pos[2]| VGPR | |片段坐标z|
| 17 |f32 | SI_PARAM_POS_W_FLOAT frag_pos[3]| VGPR | |片段w|
| 18 |f32 | SI_PARAM_FRONT_FACE | VGPR |  | 面部剔除布尔值，用于gl_Color判断front,back|
|||||||
| 19 |i32 | SI_PARAM_ANCILLARY | VGPR |  |辅助vgpr,eyeid, eta,gl_SampleID|
| 20 |f32 | SI_PARAM_SAMPLE_COVERAGE | VGPR |  | 采样覆盖度|
| 21 |i32 | SI_PARAM_POS_FIXED_PT| VGPR |  |固定坐标x,y|


关于最后一个PRIM_MASK的SPGR没有通过寄存器配置设置。

* ps参数插值

> 对于像素波（pixel waves），顶点属性数据被预加载到局部数据共享内存（LDS）中，而重心坐标（I、J）被预加载到向量通用寄存器（VGPRs）中，然后才开始波动（wave）的处理。参数插值可以通过使用LDS_PARAM_LOAD从LDS中加载属性数据到VGPRs，然后使用V_INTERP指令来进行像素级的插值。

> LDS参数加载（LDS_PARAM_LOAD）用于从LDS中读取顶点参数数据并将其存储在VGPRs中，以便用于参数插值。这些指令的操作方式类似于内存指令，但它们使用EXPcnt来跟踪未完成的读取操作，并在数据到达VGPRs后递减EXPcnt。
> 
> 像素着色器（pixel shaders）可能在其参数数据写入LDS之前就已启动。一旦数据在LDS中可用，波的STATUS寄存器中的"LDS_READY"位将设置为1。如果在"LDS_READY"设置为1之前要发出LDS_DIRECT_LOAD或LDS_PARAM_LOAD指令，像素着色器波将被阻塞。
> 
> 最常见的插值形式涉及通过重心坐标"I"和"J"对顶点参数进行加权。常见的计算公式如下：
>   Result = P0 + I * P10 + J * P20
>   其中，"P10"是（P1 - P0），而"P20"是（P2 - P0）。
> 
> 参数插值涉及两种类型的指令：
> • LDS_PARAM_LOAD：从LDS中读取打包的参数数据到VGPR（每个四通道组打包数据）
> • V_INTERP_*：VALU FMA指令，用于在四通道组内的各个通道之间解压参数数据。

* SI_PRIM_MASK

数据格式如下可表示三种类型， 优化，图元，lds偏移。 bc_optimize 表示是否开启优化， prim_mask控制图元种类。
lds偏移为需要插值属性值在lds的偏移地址， 如此插值函数可获得实际数据。

```
then {bc_optimize, prim_mask[14:0] , lds_offset[15:0]} N/A

```

*  SI_PARAM_PERSP_PULL_MODEL 
*  
pull-mode 插值

"Pull-model interpolation" 是一种用于在计算机图形和计算机图形硬件中处理插值的方法。这种插值方法通常用于处理图像、图形对象的平滑渐变、渲染和插值等方面。与 "push-model interpolation" 相对，它强调数据点从图像中"拉"取所需的插值值。存在坐标(x,y,z);

1. **数据拉取：** 在 pull-model 插值中，数据点或像素从图像中"拉"取所需的插值值。这意味着每个像素或数据点都根据其自身位置从图像或数据集中获取所需的颜色、纹理坐标或其他属性。

2. **平滑渐变：** pull-model 插值通常用于创建平滑的渐变和过渡效果。例如，在图像处理中，这种插值方法用于创建平滑的色彩过渡，以避免锯齿状或块状效果。

3. **硬件支持：** pull-model 插值通常需要硬件支持，因为它需要在像素级别进行插值计算。现代的图形处理单元（GPU）通常支持 pull-model 插值，以便实现高质量的图形渲染和图像处理。

4. **灵活性：** pull-model 插值提供了更大的灵活性，因为每个像素可以根据其自身的位置和要求来获取插值值。这使得图形设计师能够创建更精细和自定义的渲染效果。

5. **临近点插值：** pull-model 插值的一个常见应用是临近点插值（Nearest-Neighbor Interpolation），其中每个像素从最近的数据点或纹素中获取值。这用于放大和缩小图像，以确保图像质量。

总之，pull-model interpolation 是一种插值方法，它在图形和图像处理中用于创建平滑渐变和渲染效果。它通过从图像中"拉"取所需的插值值来处理每个像素或数据点，从而提供更高的灵活性和定制性。这种插值方法通常需要图形硬件支持，以在像素级别执行插值计算。



* SI_PARAM_LINE_STIPPLE_TEX 

线纹理装配用于glLineStipple。在最新版本已经废弃使用。

* SI_PARAM_ANCILLARY

1. RTA_Index[28:16]：射线追踪加速结构索引 - 在射线追踪中使用的加速结构的索引。

2. Sample_Num[11:8]：采样号码

3. Eye_id[7]：眼睛标识 - 标识图形渲染中的视点或眼睛（通常用于立体视觉渲染）。

4. VRSrateY[5:4]：变速刷新率（垂直方向） - 用于可变刷新率技术中指定垂直方向的刷新率。

5. VRSrateX[3:2]：变速刷新率（水平方向） - 用于可变刷新率技术中指定水平方向的刷新率。

6. Prim Typ[1:0]：图元类型 - 用于指示图形渲染中使用的图元类型，例如点、线或三角形。

其中Sample_Num对应gl_SampleID, 并非采样数量

* SI_PARAM_POS_FIXED_PT 

固定位置坐标Position {Y[16], X[16]} 

当启用PolygonStipple，fbfetch,双边颜色时使用。


fs的VGPR各个参数由 SPI_PS_INPUT_ENA SPI_PS_INPUT_ADDR  控制， 
这两个寄存器SPI_PS_INPUT_ADDR由编译器后端控制生成。

在si_emit_shader_ps 内部派发这两个寄存器， 但是只有当这个值与默认值不同时才会下发pm4包。
可以通过gallium_dump的log SHADER_KEY观察

如
```

SHADER KEY
  part.ps.prolog.color_two_side = 0
  part.ps.prolog.flatshade_colors = 0
  part.ps.prolog.poly_stipple = 0
  part.ps.prolog.force_persp_sample_interp = 0
  part.ps.prolog.force_linear_sample_interp = 0
  part.ps.prolog.force_persp_center_interp = 1
  part.ps.prolog.force_linear_center_interp = 0
  part.ps.prolog.bc_optimize_for_persp = 0
  part.ps.prolog.bc_optimize_for_linear = 0
  part.ps.epilog.spi_shader_col_format = 0x4
  part.ps.epilog.color_is_int8 = 0x0
  part.ps.epilog.color_is_int10 = 0x0
  part.ps.epilog.last_cbuf = 0
  part.ps.epilog.alpha_func = 4
  part.ps.epilog.alpha_to_one = 0
  part.ps.epilog.poly_line_smoothing = 0
  part.ps.epilog.clamp_color = 0


```

固件可通过读取参数列表获取响应地址。
其中具体的VGPR地址计算规则参考` 3.5.4.1. Pixel Shader VGPR Input Control`
[rdna3-reference](https://www.amd.com/content/dam/amd/en/documents/radeon-tech-docs/instruction-set-architectures/rdna3-shader-instruction-set-architecture-feb-2023_0.pdf)



根据glsl 4.5 规范 有如下内建变量输入输出值:

* in vec4 gl_FragCoord;

SI_PARAM_POS_X_FLOAT frag_pos[0]
SI_PARAM_POS_Y_FLOAT frag_pos[1]
SI_PARAM_POS_Z_FLOAT frag_pos[2]
SI_PARAM_POS_W_FLOAT frag_pos[3]

* in bool gl_FrontFacing;

front_face

* in float gl_ClipDistance[];

    1. 寄存器相关配置：
        si_emit_clip_regs
        si_shader_io_get_unique_index
        属于shader_int 输入变量，根据插值表确定插值参数获取，  

* in float gl_CullDistance[];

同gl_ClipDistance

* in vec2 gl_PointCoord;

根据规范

> The values in gl_PointCoord are two-dimensional coordinates indicating where within a point primitive
the current fragment is located, when point sprites are enabled. They range from 0.0 to 1.0 across the
point. If the current primitive is not a point, or if point sprites are not enabled, then the values read from
gl_PointCoord are undefined.

同gl_ClipDistance用法


* in int gl_PrimitiveID:

根据规范core, 473 page:

>  If a geometry shader is active, the built-in variable gl_PrimitiveID contains the ID value emitted by the geometry shader for the provoking vertex. If no
geometry shader is active, gl_PrimitiveID contains the number of primitives
processed by the rasterizer since the last drawing command was called. The first
primitive generated by a drawing command is numbered zero, and the primitive ID
counter is incremented after every individual point, line, or polygon primitive is
processed. For polygons drawn in point or line mode, the primitive ID counter is
incremented only once, even though multiple points or lines may be drawn

属于consant常量插值。

根据mesa-18的注释:

>fs.constant返回中间顶点的参数，因此对于平面着色来说并不是真正有用的。
>它旨在用于自定义插值（但内置函数无法从其他两个顶点中获取）。
>
>幸运的是，这并不重要，因为我们依赖于FLAT_SHADE状态来处理正确的事情。
>我们之所以使用fs.constant，是因为fs.interp不能用于整数，因为它们可能等于NaN。
>
>当interp为false时，我们将使用fs.constant，或者对于更新的LLVM版本，我们将使用amdgcn.interp.mov。

使用内建函数 llvm.amdgcn.interp.mov之间获取。


* in int gl_SampleID;

属于系统内建变量
通过ancillary移位获取[11:8]

* in vec2 gl_SamplePosition;

属于系统内建输入变量

具体代码参考si_shader.c:si_load_system_value对 TGSI_SEMANTIC_SAMPLEPOS 的处理。
概括来说就是用了片元坐标的x，y值然后进行插值使用。


* in int gl_SampleMaskIn[];

具体代码参考si_shader.c:si_load_system_value对 TGSI_SEMANTIC_SAMPLEMASK 的处理。
获取参数为SI_PARAM_SAMPLE_COVERAGE


* in int gl_Layer;

同 gl_PrimitiveID ，属于输入变量。需要插值。

根据glsl4.5 specs 

> The input variable gl_Layer in the fragment language will have the same value that was written to the
output variable gl_Layer in the geometry language. If the geometry stage does not dynamically assign a
value to gl_Layer, the value of gl_Layer in the fragment stage will be undefined. If the geometry stage
makes no static assignment to gl_Layer, the input value in the fragment stage will be zero. Otherwise, the
fragment stage will read the same value written by the geometry stage, even if that value is out of range.
If a fragment shader contains a static access to gl_Layer, it will count against the implementation defined
limit for the maximum number of inputs to the fragment stage.

对于无gs 为0。

* in int gl_ViewportIndex;

根据opengl core 13.6.1 一节， viewportindex可以被gs设置， 如果没有gs或者gs没有赋值，则为0


* in bool gl_HelperInvocation;

根据glsl4.5 specs 

> The value gl_HelperInvocation is true if the fragment shader invocation is considered a helper
invocation and is false otherwise. A helper invocation is a fragment-shader invocation that is created
solely for the purposes of evaluating derivatives for use in non-helper fragment-shader invocations. Such
derivatives are computed implicitly in the built-in function texture() (see section 8.9 “Texture
Functions”), and explicitly in the derivative functions in section 8.13.1 “Derivative Functions”, for
example dFdx() and dFdy().

该值通过调用内建函数llvm.amdgcn.ps.live 获取，。、
参加代码si_shader.c 对TGSI_SEMANTIC_HELPER_INVOCATION 处理。


* out float gl_FragDepth;


自定义深度值


* out int gl_SampleMask[];

采样mask， 控制覆盖度。


returns:

返回值为main的入参,

## PS main

|参数索引|参数类型|参数含义|寄存器种类|  MESA下发地址寄存器|备注 |
|----|---|---|---| --- | ---|
| 0 | v4i32* | param_rw_buffers | SGPR | SPI_SHADER_USER_DATA_PS_0 ||
| 1 | v8i32* | param_bindless_samplers_and_images| SGPR | SPI_SHADER_USER_DATA_PS_1 ||
| 2 | f32* | const_and_shader_buffers|  SGPR |SPI_SHADER_USER_DATA_PS_2 |无UBO或SSBO|
|   | v4i32* | const_and_shader_buffers|  SGPR |SPI_SHADER_USER_DATA_PS_2 ||
| 3 | v8i32* | samplers_and_images  | SGPR |SPI_SHADER_USER_DATA_PS_3 ||
|||||||
| 4 | f32 | SI_PARAM_ALPHA_REF | SGPR  | SPI_SHADER_USER_DATA_PS_4||
| 5 | i32 | SI_PARAM_PRIM_MASK | SGPR | ||
|||||||
| 6 | v2i32 | SI_PARAM_PERSP_SAMPLE | VGPR |  ||
| 7 | v2i32 | SI_PARAM_PERSP_CENTER | VGPR |  ||
| 8 | v2i32 | SI_PARAM_PERSP_CENTROID | VGPR |  ||
| 9 | v3i32 | SI_PARAM_PERSP_PULL_MODEL | VGPR |  ||
| 10 | v2i32 | SI_PARAM_LINEAR_SAMPLE | VGPR |  ||
| 11 | v2i32 | SI_PARAM_LINEAR_CENTER | VGPR |  ||
| 12 | v2i32 | SI_PARAM_LINEAR_CENTROID | VGPR |  ||
| 13 | f32 | SI_PARAM_LINE_STIPPLE_TEX | VGPR |  ||
|||||||
| 14 |f32 | SI_PARAM_POS_X_FLOAT frag_pos[0]| VGPR | ||
| 15 |f32 | SI_PARAM_POS_Y_FLOAT frag_pos[1]| VGPR | ||
| 16 |f32 | SI_PARAM_POS_Z_FLOAT frag_pos[2]| VGPR | ||
| 17 |f32 | SI_PARAM_POS_W_FLOAT frag_pos[3]| VGPR | ||
| 18 |f32 | SI_PARAM_FRONT_FACE | VGPR | PA_SU_SC_MODE_CNTL | \n|
|||||||
| 19 |i32 | SI_PARAM_ANCILLARY | VGPR |  ||
| 20 |f32 | sample_coverage | VGPR |  ||
| 21 |i32 | SI_PARAM_POS_FIXED_PT| VGPR |  ||
| 22 |f32 | color_elements0 | VGPR |  |存在prolog|
| ... | f32 | ... | VGPR |  ||
| 22+num_color_elements |f32 | color_elements[num_color_elements]| VGPR |  ||

这里的num_color_elements 表示fs中使用的gl_Color颜色分量数，具体可由swizzle mask 检测。 如果使用的是x则为1, 这个是再linkshader中的阶段确定的。

returns: 

返回值 为epilog使用:

num_returns数量为num_return_sgprs 5个i32 加上colors_written, write_z, writes_stencil,writes_samplemask ,


但最低不小于(SI_SGPR_ALPHA_REF + PS_EPILOG_SAMPLEMASK_MIN_LOC +  2) =  20个。


## PS epilog 

|参数位置|参数类型|参数含义|寄存器 | 备注
|---|---|---|---| --- |
| 0 |intptr(i32/i64) | param_rw_buffers | SPI_SHADER_USER_DATA_PS_0 | |
| 1 |intptr | param_bindless_sample| SPI_SHADER_USER_DATA_PS_1 | | 
| 2 |intptr | const_and_shader_buffers| SPI_SHADER_USER_DATA_PS_2 | 
| 3 |intptr  | samplers_and_images | SPI_SHADER_USER_DATA_PS_3 |
|||||||
| 4 | f32 | SI_PARAM_ALPHA_REF | SGPR  |   | |
|||||||
| 5 | f32 | colos_writen[0] | VGPR  |   ||
| 6 | f32 | colos_writen[1] | VGPR  |   ||
| 7 | f32 | colos_writen[3] | VGPR  |   ||
| 8 | f32 | colos_writen[4] | VGPR  |   ||
| 9 | f32 | z-buffer | VGPR  |   |gl_FragDepth| 
| 10 | f32 | stencil | VGPR  |   |gl_FragStencilRefARB |
| 11 | f32 | samplemask | VGPR  |   |gl_SampeMask[]|
| ... | f32 | ... | VGPR  |   ||
| 19 | f32 | PS_EPILOG_SAMPLEMASK_MIN_LOC + 5 | VGPR  |   ||


* stencil 

启用 GL_ARB_shader_stencil_export 扩展时使用。

内建变量 gl_FragStencilRefARB.

returns: void 


# Compute Shaders 

## Compute Shader

函数声明创建在create_function

|参数位置|参数类型|参数含义|寄存器种类|  MESA下发地址寄存器|备注 |
|---|---|---|---| --- | ---|
| 0 | v4i32* | param_rw_buffers                  | SGPR | COMPUTE_USER_DATA_0        | 
| 1 | v8i32* | param_bindless_samplers_and_images| SGPR | COMPUTE_USER_DATA_1        |
| 2 | f32*   | const_and_shader_buffers          | SGPR | COMPUTE_USER_DATA_2        | 无UBO或SSBO
|   | v4i32* | const_and_shader_buffers          | SGPR | COMPUTE_USER_DATA_2        |
| 3 | v8i32* | samplers_and_images               | SGPR | COMPUTE_USER_DATA_3        |
| ? | v3i32  | num_work_groups                   | SGPR | COMPUTE_USER_DATA_4        | 工作组的数量
| ? | v3i32  | param_block_size                  | SGPR | COMPUTE_USER_DATA_4/5      |
| ? | i32    | cs_user_data                      | SGPR | COMPUTE_USER_DATA_4/5/6    |
| ? | i32    | workgroup_ids[0]                  | SGPR | COMPUTE_USER_DATA_4/5/6/7  |
| ? | i32    | workgroup_ids[1]                  | SGPR | COMPUTE_USER_DATA_5/6/7/8  |
| ? | i32    | workgroup_ids[2]                  | SGPR | COMPUTE_USER_DATA_6/7/8/9  |
| ? | v3i32  | local_invocation_ids              | VGPR | |
| end |void | returns | void | |函数返回值


### 内建变量

1. **gl_NumWorkGroups**：
   - 这个变量包含传递给调度函数的工作组数目。

2. **gl_WorkGroupID**：
   - 这是当前着色器调用的工作组。XYZ 组件的每一个都将在半开区间 [0, gl_NumWorkGroups.XYZ) 内。

3. **gl_LocalInvocationID**：
   - 这是工作组内的当前着色器调用。XYZ 组件的每一个都将在半开区间 [0, gl_WorkGroupSize.XYZ) 内。

4. **gl_GlobalInvocationID**：
   - 这个值唯一标识了计算着色器调用中的特定着色器调用，它是通过以下数学计算的简化形式：
     gl_WorkGroupID * gl_WorkGroupSize + gl_LocalInvocationID;

5. **gl_LocalInvocationIndex**：
   - 这是 gl_LocalInvocationID 的一维版本，它标识了工作组内的当前着色器调用的索引。它的简化形式如下：

     gl_LocalInvocationIndex =
     gl_LocalInvocationID.z * gl_WorkGroupSize.x * gl_WorkGroupSize.y +
     gl_LocalInvocationID.y * gl_WorkGroupSize.x +
     gl_LocalInvocationID.x;







