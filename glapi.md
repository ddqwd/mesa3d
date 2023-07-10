

```
 _mesa_initialize (extensions_override=0x0) at ../src/mesa/main/context.c:248
#1  0x00007ffff565354a in st_api_create_context (fscreen=fscreen@entry=0x5555555ff200,
    attribs=attribs@entry=0x7fffffffd391, error=error@entry=0x7fffffffd38c, shared_ctx=shared_ctx@entry=0x0)
    at ../src/mesa/state_tracker/st_manager.c:961
#2  0x00007ffff558422a in dri_create_context (screen=screen@entry=0x5555555ff200,
    api=api@entry=API_OPENGL_COMPAT, visual=visual@entry=0x55555560c6b0,
    ctx_config=ctx_config@entry=0x7fffffffd4b0, error=error@entry=0x7fffffffd5bc,
    sharedContextPrivate=sharedContextPrivate@entry=0x0, loaderPrivate=0x55555557d740)
    at ../src/gallium/frontends/dri/dri_context.c:177
#3  0x00007ffff558808c in driCreateContextAttribs (psp=0x5555555ff200, api=<optimized out>,
    config=0x55555560c6b0, shared=0x0, num_attribs=<optimized out>, attribs=<optimized out>,
    error=0x7fffffffd5bc, data=0x55555557d740) at ../src/gallium/frontends/dri/dri_util.c:627
#4  0x00007ffff7f79e19 in drisw_create_context_attribs (base=0x55555557c490, config_base=<optimized out>,
    shareList=<optimized out>, num_attribs=<optimized out>, attribs=<optimized out>, error=0x7fffffffd5bc)
    at ../src/glx/drisw_glx.c:639
#5  0x00007ffff7f790f9 in dri_common_create_context (base=<optimized out>, config_base=<optimized out>,
    shareList=<optimized out>, renderType=<optimized out>) at ../src/glx/dri_common.c:679
#6  0x00007ffff7f7c5ac in CreateContext (dpy=dpy@entry=0x55555556afb0, generic_id=119,
    config=config@entry=0x555555615640, shareList_user=shareList_user@entry=0x0, allowDirect=<optimized out>,
    allowDirect@entry=1, code=code@entry=24, renderType=32788) at ../src/glx/glxcmds.c:322
#7  0x00007ffff7f7c844 in glXCreateNewContext (dpy=0x55555556afb0, fbconfig=0x555555615640, renderType=32788,
    shareList=0x0, allowDirect=1) at ../src/glx/glxcmds.c:1443
#8  0x00007ffff7ec1480 in ?? () from /lib/x86_64-linux-gnu/libglut.so.3
#9  0x00007ffff7ec1deb in fgOpenWindow () from /lib/x86_64-linux-gnu/libglut.so.3
#10 0x00007ffff7ec05c8 in fgCreateWindow () from /lib/x86_64-linux-gnu/libglut.so.3
#11 0x00007ffff7ec209f in glutCreateWindow () from /lib/x86_64-linux-gnu/libglut.so.3
#12 0x000055555555574a in main (argc=1, argv=0x7fffffffdbb8) at ../src/redbook/list.c:118
```
glXCreateContext & glXCreateNewContext
CreateContext
dri_common_create_context
dri2_create_context_attribs
driCreateContextAttribs
dri_create_context
st_api_create_context
st_create_context
    _mesa_initialize_context 


_mesa_initialize_context & st_api_create_context
_mesa_initialize
one_time_init
    _mesa_init_remap_table 
        map_function_spec 
            _glapi_add_dispatch

