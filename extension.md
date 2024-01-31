
本次分析的为opengl 类的扩展， 非xlib。

# extensions_table.h

扩展定义在extensions_table.h中
每一个扩展的启用都是通过定义EXT宏来控制。

# mesa_extension_table

mesa使用`_mesa_extension_table`来存储所有的扩展， 每个扩展通过`_mesa_extension`来存储。


```c
/**
 * \brief Table of supported OpenGL extensions for all API's.
 */
const struct mesa_extension _mesa_extension_table[] = {
#define EXT(name_str, driver_cap, gll_ver, glc_ver, gles_ver, gles2_ver, yyyy) \
        { .name = "GL_" #name_str, .offset = o(driver_cap), \
          .version = { \
            [API_OPENGL_COMPAT] = gll_ver, \  
            [API_OPENGL_CORE]   = glc_ver, \
            [API_OPENGLES]      = gles_ver, \ 
            [API_OPENGLES2]     = gles2_ver, \
           }, \
           .year = yyyy \
        },
#include "extensions_table.h"                 
#undef EXT
```

扩展启用表， mesa用`gl_context`的`Extension` 成员表示。 

在`gl_context` 创建时， 会使用**`st_init_extensions`** 对 opengl core , compability , opengl es , es 2 的Extension进行初始化

* 在该函数内部， 首先会初始化一部分扩展， 这些扩展对所有gallium 驱动都支持， 
* 对一部分扩展 通过`pipe_screen->get_param`接口获取驱动对该驱动的支持情况。
* 对一部分格式扩展通过`pipe_screent->is_format_supported `获取驱动支持
* 根据glsl的版本不同和opoengl 是否是兼容模式也会设置不同扩展，比如ARB_GPU_shader
* 对计算shader, 通过`get_compute_param`获取是否支持。
* 对一部分扩展根据winsys获取config信息来判断是否支持。
* 对于其他的扩展如果没有则驱动暂时不支持或者没有添加，  比如GL_EXT_shader_framebuffer_fetch，这个扩展在mesa-22已经加上。而在mesa-18 则在radeonsi上未作处理，在i965上已经启用 ，如果强制启用， 则可以设置 **MESA_EXTENSION_OVERRIDE**环境变量来启用。



