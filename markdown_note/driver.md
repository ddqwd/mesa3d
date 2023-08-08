  
## OpenGL接口调用到gallium3d  driver流程分析

当程序使用如glutInit之后，进入 __glXInitialize
__glXInitialize 会搜索所有可用driver，如果没有默认使用mesa的软光栅swrast_dri.so.然后调用CreateDisplay进行各个库虚函数如创建屏幕，创建context的函数指针加载，之后driswCreateScreen创建屏幕，driswCreateScreen内部会调用driOpenSwrast，进行swast_dri.so的加载，一旦_dri.so库被加载，_dri.so内部的 megadriver_stub_init就会立即调用，这是一个用于 megadriver 动态库的构造函数。
当驱动程序被 dlopen 加载时，该函数将被执行。它将搜索通过 dladdr 函数打开的 foo_dri.so 文件的名称。 在找到 foo 的名称后，它将调用 __driDriverGetExtensions_foo，
并使用返回值更新 __driDriverExtensions 以便与旧版本的 DRI 驱动程序加载器兼容。,他通过__attribute__((constructor))这种属性机制保证库加载时立即执行。

加载完成之后，调用createNewScreen2或createNewScreen创建实际的屏幕，对于软光栅调用gallivm下llvmpipe的llvmpipe_create_screen。
当程序调用glutCreateWindow，之后进行context的创建，dri_create_context-> st_api_create_context  ->st_create_context->实际驱动的create_context( llvmpipe_create_context,)
dri_create_context,会build设备驱动函数，初始化一个 struct gl_context 结构体（渲染上下文）。 这包括通过指针分配所有其他结构体和数组，这些结构体和数组与上下文相关联。
需要注意的是，驱动程序需要在这里传递其 dd_function_table，因为我们至少需要调用 driverFunctions->NewTextureObject 来创建默认纹理对象。
执行导入和导出的回调表的初始化，以及其他一次性初始化。如果没有提供共享上下文，则会分配一个，并增加其引用计数。设置 GL API 的调度表。初始化 TNL 模块。设置最大的 Z 缓冲深度。最后查询 \c MESA_DEBUG 和 \c MESA_VERBOSE 环境变量以获取调试标志。 
t_create_context通过调用st_init_driver_functions,_mesa_initialize_context,st_creat_context_priv, 进行_mesa_gl*function的初始化以及绑定到实际的gl*函数接口

create_context会初始化llvm_context的各个绘制函数指针，如draw,vs_state,LLVMContext的创建。

对于mesa中GL dispatch是通过mapi目录下的glapi库实现， 其中原理还需进一步分析 。

_mesa_* 函数接口的安装

#define CALL_by_offset(disp, cast, offset, parameters) \
    (*(cast (GET_by_offset(disp, offset)))) parameters
#define GET_by_offset(disp, offset) \
    (offset >= 0) ? (((_glapi_proc *)(disp))[offset]) : NULL
#define SET_by_offset(disp, offset, fn) \
    do { \
        if ( (offset) < 0 ) { \
            /* fprintf( stderr, "[%s:%u] SET_by_offset(%p, %d, %s)!\n", */ \
            /*         __func__, __LINE__, disp, offset, # fn); */ \
            /* abort(); */ \
        } \
        else { \
            ( (_glapi_proc *) (disp) )[offset] = (_glapi_proc) fn; \
        } \
    } while(0)




typedef void (GLAPIENTRYP _glptr_Clear)(GLbitfield);
#define CALL_Clear(disp, parameters) \
    (* GET_Clear(disp)) parameters
static inline _glptr_Clear GET_Clear(struct _glapi_table *disp) {
   return (_glptr_Clear) (GET_by_offset(disp, _gloffset_Clear));
}
