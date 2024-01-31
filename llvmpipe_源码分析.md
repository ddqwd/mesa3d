

#  编译llvmpipe

git@github.com:Mesa3D/mesa.git

```
#!bin/bash

meson setup  build --reconfigure \
	-Dprefix="$PWD"/build/install \
	-Dbuildtype=debug \
    -Dplatforms=x11 \
    -Dgallium-rusticl="true" \
    -Dgallium-opencl="standalone"\
    -Dgallium-drivers="swrast" \
    -Dvulkan-drivers="swrast" \
    -Dvulkan-layers="device-select" \
    -Dcpp_args='-fcommon -ludev' \
    -Dc_args='-fcommon -ludev'  \
    -Dc_link_args='-ludev' \
    -Dcpp_link_args='-ludev' \

cd build
ninja -j2

ninja install

```

## llvmpipe_dri.so 生成关系

```
## src/gallium/meson.build
if with_gallium_softpipe
  subdir('drivers/softpipe')
  if with_llvm
    subdir('drivers/llvmpipe')
  endif
else
  driver_swrast = declare_dependency()
endif

```

driver_swrast(softpipe) => libsoftpipe

llvmpipe => dep_llvm

driver_swrast(llvmpipe)  => [ driver_swrast(softpipe), libllvmpipe]

libpipe_swrast.so => driver_swrast(llvmpipe)



# mesa实现分析

分析流程分为驱动初始化，资源下发，shader编译， 光栅化，出图显示流程五步
采用x11后端glx

mesa的模块按照流程可分为glx, state_tracker ,dri frontend, llvmpipe, sw_winsys


## 驱动初始化

```

drisw_bind_context
    driFetchDrawable

```
## glx 


### glXQueryExtensionsString
```c
_GLX_PUBLIC const char *
glXQueryExtensionsString(Display * dpy, int screen)
{
   struct glx_screen *psc;
   struct glx_display *priv;
   int is_direct_capable = GL_FALSE;

   if (GetGLXPrivScreenConfig(dpy, screen, &priv, &psc) != Success) {
         *ppriv = __glXInitialize(dpy);

}

```

### glXChooseFBConfig

```c
_GLX_PUBLIC GLXFBConfig *
glXChooseFBConfig(Display * dpy, int screen,
                  const int *attribList, int *nitems)
{
   config_list = (struct glx_config **)
      glXGetFBConfigs(dpy, screen, &list_size);
            struct glx_display *priv = __glXInitialize(dpy);
        
}
```

#### __glXInitialize
```c
/*
** Initialize the client side extension code.
*/
 _X_HIDDEN struct glx_display *
__glXInitialize(Display * dpy)
{
   XExtCodes *codes;
   struct glx_display *dpyPriv, *d;
   int i, majorVersion = 0;

   dpyPriv = calloc(1, sizeof *dpyPriv);
   if (!dpyPriv)
      return NULL;

   codes = XInitExtension(dpy, __glXExtensionName);

   /* This GLX implementation requires GLX 1.3 */
   if (!QueryVersion(dpy, dpyPriv->codes.major_opcode,
            xcb_glx_query_version_reply_t *reply = xcb_glx_query_version_reply(c,
      XESetWireToEvent(dpy, dpyPriv->codes.first_event + i, __glXWireToEvent);
      XESetEventToWire(dpy, dpyPriv->codes.first_event + i, __glXEventToWire);
   }
   XESetCloseDisplay(dpy, dpyPriv->codes.extension, __glXCloseDisplay);
   XESetErrorString (dpy, dpyPriv->codes.extension, __glXErrorString);

   dpyPriv->glXDrawHash = __glxHashCreate();

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
   Bool glx_direct = !debug_get_bool_option("LIBGL_ALWAYS_INDIRECT", false);
   Bool glx_accel = !debug_get_bool_option("LIBGL_ALWAYS_SOFTWARE", false);
   Bool zink;
   const char *env = getenv("MESA_LOADER_DRIVER_OVERRIDE");
   zink = env && !strcmp(env, "zink");

   dpyPriv->drawHash = __glxHashCreate();

   dpyPriv->zombieGLXDrawable = _mesa_pointer_set_create(NULL);

   /* Set the logger before the *CreateDisplay functions. */
   loader_set_logger(glx_message);

   /*
    ** Initialize the direct rendering per display data and functions.
    ** Note: This _must_ be done before calling any other DRI routines
    ** (e.g., those called in AllocAndFetchScreenConfigs).
    */
#if defined(GLX_USE_DRM)
   if (glx_direct && glx_accel && !zink) {
#if defined(HAVE_DRI3)
      if (!debug_get_bool_option("LIBGL_DRI3_DISABLE", false))
         dpyPriv->dri3Display = dri3_create_display(dpy);
#endif /* HAVE_DRI3 */
      if (!debug_get_bool_option("LIBGL_DRI2_DISABLE", false))
         dpyPriv->dri2Display = dri2CreateDisplay(dpy);
   }
#endif /* GLX_USE_DRM */
   if (glx_direct)
      dpyPriv->driswDisplay = driswCreateDisplay(dpy, zink);

#endif /* GLX_DIRECT_RENDERING && !GLX_USE_APPLEGL */

   if (!AllocAndFetchScreenConfigs(dpy, dpyPriv)) {
      free(dpyPriv);
      return NULL;
   }

    // 发送clientinfgo to xserver
   glxSendClientInfo(dpyPriv, -1);
        gl_extension_string = __glXGetClientGLExtensionString(screen);
        c = XGetXCBConnection(glx_dpy->dpy);
        xcb_glx_set_client_info_2arb(c,

}
```
* __glXInitialize ->  envvar:

LIBGL_ALWAYS_INDIRECT
LIBGL_ALWAYS_SOFTWARE
LIBGL_DRI2_DISABLE
LIBGL_DRI3_DISABLE
MESA_LOADER_DRIVER_OVERRIDE
zink
glx_message -> LIBGL_DEBUG


#### AllocAndFetchScreenConfigs

```c
/*
** Allocate the memory for the per screen configs for each screen.
** If that works then fetch the per screen configs data.
*/
static Bool
AllocAndFetchScreenConfigs(Display * dpy, struct glx_display * priv)
{
   struct glx_screen *psc;
   GLint i, screens;

   /*
    ** First allocate memory for the array of per screen configs.
    */
   screens = ScreenCount(dpy);
   priv->screens = calloc(screens, sizeof *priv->screens);
   if (!priv->screens)
      return GL_FALSE;

   for (i = 0; i < screens; i++, psc++) {
      psc = NULL;
#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
#if defined(GLX_USE_DRM)
#if defined(HAVE_DRI3)
      if (priv->dri3Display)
         psc = priv->dri3Display->createScreen(i, priv);
            [-> dri3_create_screen]
#endif /* HAVE_DRI3 */
      if (psc == NULL && priv->dri2Display)
	 psc = priv->dri2Display->createScreen(i, priv);
#endif /* GLX_USE_DRM */


      if (psc == NULL && priv->driswDisplay)
	 psc = priv->driswDisplay->createScreen(i, priv);
                [-> driswCreateScreen ]
#endif /* GLX_DIRECT_RENDERING && !GLX_USE_APPLEGL */

}

```
* ->  driswCreateScreen

#### driswCreateScreen

```cpp
static struct glx_screen *
driswCreateScreen(int screen, struct glx_display *priv)
{
   const struct drisw_display *pdpyp = (struct drisw_display *)priv->driswDisplay;
   if (pdpyp->zink && !debug_get_bool_option("LIBGL_KOPPER_DISABLE", false)) {
      return driswCreateScreenDriver(screen, priv, "zink");
   }

   return driswCreateScreenDriver(screen, priv, "swrast");
         __GLXDRIscreen *psp;
         const __DRIconfig **driver_configs;
         const __DRIextension **extensions;
         struct drisw_screen *psc;
         struct glx_config *configs = NULL, *visuals = NULL;
         const __DRIextension **loader_extensions_local;
         const struct drisw_display *pdpyp = (struct drisw_display *)priv->driswDisplay;

          // from xserver fetch config data
         if (!glx_screen_init(&psc->base, screen, priv)) {} 
            if (!getVisualConfigs(psc, priv, screen))
            if (!getFBConfigs(psc, priv, screen))


         /**
         * 尝试使用 \c dlopen 加载指定的驱动程序。
         *
         * 该函数将在驱动程序名称上添加 "_dri.so" 后缀，并按照 \c LIBGL_DRIVERS_PATH 环境变量指定的目录顺序搜索驱动程序。
         *
         * \param driverName - 驱动程序的名称，如 "i965"、"radeon"、"nouveau" 等。
         * \param out_driver_handle - 用于返回 dlopen() 处理结果的地址。
         *
         * \returns
         * 驱动程序的 __DRIextension 入口表，如果未找到驱动程序文件则返回 \c NULL。
         */
         extensions = driOpenDriver(driver, &psc->driver);
               void *glhandle;
            
               /* Attempt to make sure libGL symbols will be visible to the driver */
               glhandle = dlopen(GL_LIB_NAME, RTLD_NOW | RTLD_GLOBAL);
            
               static const char *search_path_vars[] = {
                  "LIBGL_DRIVERS_PATH",
                  "LIBGL_DRIVERS_DIR", /* deprecated */
                  NULL
               };
            
               const __DRIextension **extensions =
                  loader_open_driver(driverName, out_driver_handle, search_path_vars);
            
               if (glhandle)
                  dlclose(glhandle);
            
               return extensions;


         psc->name = driver;

         if (pdpyp->zink)
            loader_extensions_local = kopper_extensions_noshm;
         else if (!check_xshm(psc->base.dpy))
            loader_extensions_local = loader_extensions_noshm;
         else
            loader_extensions_local = loader_extensions_shm;

         static const struct dri_extension_match exts[] = {
             { __DRI_CORE, 1, offsetof(struct drisw_screen, core), false },
             { __DRI_SWRAST, 4, offsetof(struct drisw_screen, swrast), false },
             { __DRI_KOPPER, 1, offsetof(struct drisw_screen, kopper), true },
             { __DRI_COPY_SUB_BUFFER, 1, offsetof(struct drisw_screen, copySubBuffer), true },
             { __DRI_MESA, 1, offsetof(struct drisw_screen, mesa), false },
         };
         if (!loader_bind_extensions(psc, exts, ARRAY_SIZE(exts), extensions))
            goto handle_error;

         psc->driScreen =
            psc->swrast->createNewScreen2(screen, loader_extensions_local,
                                          extensions,
                                          &driver_configs, psc);
                [-> dri driSWRastCreateNewScreen2]

         extensions = psc->core->getExtensions(psc->driScreen);

         // add glx entensions 
         driswBindExtensions(psc, extensions);
              __glXEnableDirectExtension(&psc->base, "GLX_ARB_create_context");
              __glXEnableDirectExtension(&psc->base, "GLX_ARB_create_context_profile");
              __glXEnableDirectExtension(&psc->base, "GLX_ARB_create_context_no_error");
              __glXEnableDirectExtension(&psc->base, "GLX_EXT_no_config_context");

   

         configs = driConvertConfigs(psc->core, psc->base.configs, driver_configs);
         visuals = driConvertConfigs(psc->core, psc->base.visuals, driver_configs);

         psc->base.configs = configs;
         psc->base.visuals = visuals;

         psc->driver_configs = driver_configs;

         psc->base.vtable = &drisw_screen_vtable;
         psc->base.context_vtable = &drisw_context_vtable;
         psp = &psc->vtable;
         psc->base.driScreen = psp;
         psp->destroyScreen = driswDestroyScreen;
         psp->createDrawable = driswCreateDrawable;
         psp->swapBuffers = driswSwapBuffers;
         psp->bindTexImage = drisw_bind_tex_image;
         psp->releaseTexImage = drisw_release_tex_image;

         return &psc->base;
}

```
* zink -> LIBGL_KOPPER_DISABLE
* 进行以下操作
1. glx_screen_init
2. driOpenDriver

LIBGL_DRIVERS_PATH
LIBGL_DRIVERS_DIR

-> loader_open_driver

3. loader_bind_extensions
4. createNewScreen2


#### driOpenDriver -> loader_open_driver

```c
/**
 * 使用驱动程序名称打开 DRI 驱动程序，并返回 __DRIextension 入口点。
 *
 * \param driverName - 驱动程序的名称，如 "i965"、"radeon"、"nouveau" 等。
 * \param out_driver - 存储 dlopen() 返回值的地址。
 * \param search_path_vars - 用于覆盖 DEFAULT_DRIVER_DIR 搜索路径的 NULL 结尾的环境变量列表。
 *
 * \returns
 * 指向 __DRIextensionRec 结构体指针的指针，表示驱动程序的扩展入口表。
 */
const struct __DRIextensionRec **
loader_open_driver(const char *driver_name,
                   void **out_driver_handle,
                   const char **search_path_vars)
{
   char *get_extensions_name;
   const struct __DRIextensionRec **extensions = NULL;
   const struct __DRIextensionRec **(*get_extensions)(void);

   // 通过调用 loader_open_driver_lib 函数加载驱动程序swrast_dri.so
   // 
   void *driver = loader_open_driver_lib(driver_name, "_dri", search_path_vars,
                                         DEFAULT_DRIVER_DIR, true);


   //#define __DRI_DRIVER_GET_EXTENSIONS "__driDriverGetExtensions"
   // 获取用于查询扩展的函数名称 __driDriverGetExtensions_swarst
   get_extensions_name = loader_get_extensions_name(driver_name);
       if (asprintf(&name, "%s_%s", __DRI_DRIVER_GET_EXTENSIONS, driver_name) < 0)

   if (get_extensions_name) {
      // 使用 dlsym 获取扩展函数，并调用以获取扩展入口表
      get_extensions = dlsym(driver, get_extensions_name);
      if (get_extensions) 
         extensions = get_extensions();
             [-> dri __driDriverGetExtensions_swarst]
   }

   // 如果未成功获取扩展入口表，则尝试使用默认的扩展入口表名称
   if (!extensions)
      extensions = dlsym(driver, __DRI_DRIVER_EXTENSIONS);

   *out_driver_handle = driver;
   return extensions;
}

```

* env DEFAULT_DRIVER_DIR 

```
# meson.build
dri_drivers_path = get_option('dri-drivers-path')
if dri_drivers_path == ''
  dri_drivers_path = join_paths(get_option('prefix'), get_option('libdir'), 'dri')
endif

dri_search_path = get_option('dri-search-path')
if dri_search_path == ''
  dri_search_path = dri_drivers_path
endif

# loader/meson.build
  '-DDEFAULT_DRIVER_DIR="@0@"'.format(dri_search_path),
```
### dri3_create_display(Display * dpy)

```c
/** dri3_create_display
 *
 * Allocate, initialize and return a __DRIdisplayPrivate object.
 * This is called from __glXInitialize() when we are given a new
 * display pointer. This is public to that function, but hidden from
 * outside of libGL.
 */
_X_HIDDEN __GLXDRIdisplay *
dri3_create_display(Display * dpy)
{
   struct dri3_display                  *pdp;
   xcb_connection_t                     *c = XGetXCBConnection(dpy);
   xcb_dri3_query_version_cookie_t      dri3_cookie;
   xcb_dri3_query_version_reply_t       *dri3_reply;
   xcb_present_query_version_cookie_t   present_cookie;
   xcb_present_query_version_reply_t    *present_reply;
   xcb_generic_error_t                  *error;
   const xcb_query_extension_reply_t    *extension;


    // fetch data by xcb  api from xserver
   xcb_prefetch_extension_data(c, &xcb_present_id);
   extension = xcb_get_extension_data(c, &xcb_dri3_id);
   extension = xcb_get_extension_data(c, &xcb_present_id);
   dri3_cookie = xcb_dri3_query_version(c,
   present_cookie = xcb_present_query_version(c,
   dri3_reply = xcb_dri3_query_version_reply(c, dri3_cookie, &error);
   present_reply = xcb_present_query_version_reply(c, present_cookie, &error);



    // 设置显示接口
   pdp->base.destroyDisplay = dri3_destroy_display;
   pdp->base.createScreen = dri3_create_screen;
   pdp->loader_extensions = loader_extensions;

   return &pdp->base;
}

```

### driswCreateDisplay(Display * dpy, bool zink)

```c

/*
 * Allocate, initialize and return a __DRIdisplayPrivate object.
 * This is called from __glXInitialize() when we are given a new
 * display pointer.
 */
_X_HIDDEN __GLXDRIdisplay *
driswCreateDisplay(Display * dpy, bool zink)
{
   struct drisw_display *pdpyp;

    // 设置
   pdpyp->base.destroyDisplay = driswDestroyDisplay;
   pdpyp->base.createScreen = driswCreateScreen;
   pdpyp->zink = zink;

   return &pdpyp->base;
}



```


### glXCreateNewContext

```c
_GLX_PUBLIC GLXContext
glXCreateNewContext(Display * dpy, GLXFBConfig fbconfig,
                    int renderType, GLXContext shareList, Bool allowDirect)
{
   struct glx_config *config = (struct glx_config *) fbconfig;
   struct glx_config **config_list;
   int list_size;
   unsigned i;

   config_list = (struct glx_config **)
      glXGetFBConfigs(dpy, config->screen, &list_size);

   return CreateContext(dpy, config->fbconfigID, config, shareList,
			allowDirect, X_GLXCreateNewContext, renderType);
}

static GLXContext
CreateContext(Display *dpy, int generic_id, struct glx_config *config,
              GLXContext shareList_user, Bool allowDirect,
	      unsigned code, int renderType)
{
   struct glx_context *gc;
   struct glx_screen *psc;
   struct glx_context *shareList = (struct glx_context *) shareList_user;

   psc = GetGLXScreenConfigs(dpy, config->screen);

   gc = NULL;

   if (allowDirect && psc->vtable->create_context)
      gc = psc->vtable->create_context(psc, config, shareList, renderType);
            [-> dri_common_create_context]
   if (!gc)
      gc = indirect_create_context(psc, config, shareList, renderType);

   return (GLXContext) gc;
}





```

###  dri_common_create_context

```c
struct glx_context *
dri_common_create_context(struct glx_screen *base,
                          struct glx_config *config_base,
                          struct glx_context *shareList,
                          int renderType)
{
   unsigned int error;
   uint32_t attribs[2] = { GLX_RENDER_TYPE, renderType };

   return base->vtable->create_context_attribs(base, config_base, shareList,
                                               1, attribs, &error);
        [->  drisw_screen_vtable.drisw_create_context_attribs]
}

```

### drisw_create_context_attribs

```c

static struct glx_context *
drisw_create_context_attribs(struct glx_screen *base,
                             struct glx_config *config_base,
                             struct glx_context *shareList,
                             unsigned num_attribs,
                             const uint32_t *attribs,
                             unsigned *error)
{
   struct glx_context *pcp, *pcp_shared;
   __GLXDRIconfigPrivate *config = (__GLXDRIconfigPrivate *) config_base;
   struct drisw_screen *psc = (struct drisw_screen *) base;
   __DRIcontext *shared = NULL;

   struct dri_ctx_attribs dca;
   uint32_t ctx_attribs[2 * 5];
   unsigned num_ctx_attribs = 0;

   if (!psc->base.driScreen)
      return NULL;

   *error = dri_convert_glx_attribs(num_attribs, attribs, &dca);
   if (*error != __DRI_CTX_ERROR_SUCCESS)
      return NULL;

   /* Check the renderType value */
   if (!validate_renderType_against_config(config_base, dca.render_type)) {
      *error = BadValue;
      return NULL;
   }


   pcp = calloc(1, sizeof *pcp);
   if (pcp == NULL)
      return NULL;

   if (!glx_context_init(pcp, &psc->base, config_base)) {
      free(pcp);
      return NULL;
   }

   pcp->renderType = dca.render_type;

   pcp->driContext =
      psc->swrast->createContextAttribs(psc->driScreen,
                                        dca.api,
                                        config ? config->driConfig : NULL,
                                        shared,
                                        num_ctx_attribs / 2,
                                        ctx_attribs,
                                        error,
                                        pcp);
        [-> dri driCreateContextAttribs]

   if (pcp->driContext == NULL) {
      free(pcp);
      return NULL;
   }

   pcp->vtable = base->context_vtable;

   return pcp;
}


```



### glXMakeCurrent/glXMakeContextCurrent

```c
/**
 * Make a particular context current.
 *
 * \note This is in this file so that it can access dummyContext.
 */
static Bool
MakeContextCurrent(Display * dpy, GLXDrawable draw,
                   GLXDrawable read, GLXContext gc_user,
                   int opcode)
{
   struct glx_context *gc = (struct glx_context *) gc_user;
   struct glx_context *oldGC = __glXGetCurrentContext();
   int ret = GL_TRUE;


   if (gc) {
      /* Attempt to bind the context.  We do this before mucking with
       * gc and __glXSetCurrentContext to properly handle our state in
       * case of an error.
       *
       * If an error occurs, set the Null context since we've already
       * blown away our old context.  The caller is responsible for
       * figuring out how to handle setting a valid context.
       */
      if (gc->vtable->bind(gc, draw, read) != Success) {
            [drisw_bind_context]
         ret = GL_FALSE;
      } else {
         gc->currentDpy = dpy;
         gc->currentDrawable = draw;
         gc->currentReadable = read;
         __glXSetCurrentContext(gc);
      }
   }

   __glXUnlock();

   if (!ret)
      __glXSendError(dpy, GLXBadContext, None, opcode, False);

   return ret;
}

```

#### drisw_bind_context


```c
static int
drisw_bind_context(struct glx_context *context, GLXDrawable draw, GLXDrawable read)
{
   struct drisw_screen *psc = (struct drisw_screen *) context->psc;
   struct drisw_drawable *pdraw, *pread;

   pdraw = (struct drisw_drawable *) driFetchDrawable(context, draw);
   pread = (struct drisw_drawable *) driFetchDrawable(context, read);

   driReleaseDrawables(context);

   if (!psc->core->bindContext(context->driContext,
                               pdraw ? pdraw->driDrawable : NULL,
                               pread ? pread->driDrawable : NULL))
            [-> dri driBindContext -> dri_make_current]
      return GLXBadContext;
   if (psc->f) {
      if (pdraw)
         psc->f->invalidate(pdraw->driDrawable);
      if (pread && (!pdraw || pread->driDrawable != pdraw->driDrawable))
         psc->f->invalidate(pread->driDrawable);
   }

   return Success;
}





```

#### driFetchDrawable

```
driFetchDrawable
    __glXInitialize
    driswCreateDrawable
```

### struct 

#### glx_context

#### __GLXDRIdrawableRec/__GLXDRIdrawable

construct  : 
    _GLXDRIdrawableRec.createDrawable  
    (*createDrawable) [ driswCreateDrawable]
    xDrawable
    drawable
    psc

#### drisw_drawable

base : __GLXDRIdrawableRec
construct :
    driswCreateDrawable
    config
    gc => XCreateGC
    xdepth
    driDrawable -> drisw_screen->swrast->createNewDrawable



#### glx_display

construct : __glXInitialize 

#### __GLXDRIscreenRec /__GLXDRIscreen
    
#### glx_screen

parent : __GLXDRIscreenRec 
gl_context.psc


#### glx_config

parent : gl_context.config



#### driswCreateDrawable(struct glx_screen *base, 


```c
static __GLXDRIdrawable *
driswCreateDrawable(struct glx_screen *base, XID xDrawable,
                    GLXDrawable drawable, int type,
                    struct glx_config *modes)
{
   struct drisw_drawable *pdp;
   __GLXDRIconfigPrivate *config = (__GLXDRIconfigPrivate *) modes;
   struct drisw_screen *psc = (struct drisw_screen *) base;
   const __DRIswrastExtension *swrast = psc->swrast;
   const __DRIkopperExtension *kopper = psc->kopper;
   Display *dpy = psc->base.dpy;

   pdp = calloc(1, sizeof(*pdp));

   pdp->base.xDrawable = xDrawable;
   pdp->base.drawable = drawable;
   pdp->base.psc = &psc->base;
   pdp->config = modes;
   pdp->gc = XCreateGC(dpy, xDrawable, 0, NULL);
   pdp->xDepth = 0;

   /* Use the visual depth, if this fbconfig corresponds to a visual */
   if (pdp->config->visualID != 0) {
      template.visualid = pdp->config->visualID;
      template.screen = pdp->config->screen;
      pdp->xDepth = visinfo->depth;
   }

   /* Otherwise, or if XGetVisualInfo failed, ask the server */
   if (pdp->xDepth == 0) {
      Window root;
      int x, y;
      unsigned uw, uh, bw, depth;

      XGetGeometry(dpy, xDrawable, &root, &x, &y, &uw, &uh, &bw, &depth);
      pdp->xDepth = depth;
   }

   /* Create a new drawable */
   pdp->driDrawable =
         swrast->createNewDrawable(psc->driScreen, config->driConfig, pdp);

   pdp->base.destroyDrawable = driswDestroyDrawable;

   return &pdp->base;
}
```
## pipe_loader

### loader_open_driver_lib

```c
/**
 * Opens a driver or backend using its name, returning the library handle.
 *
 * \param driverName - a name like "i965", "radeon", "nouveau", etc.
 * \param lib_suffix - a suffix to append to the driver name to generate the
 * full library name.
 * \param search_path_vars - NULL-terminated list of env vars that can be used
 * \param default_search_path - a colon-separted list of directories used if
 * search_path_vars is NULL or none of the vars are set in the environment.
 * \param warn_on_fail - Log a warning if the driver is not found.
 */
void *
loader_open_driver_lib(const char *driver_name,
                       const char *lib_suffix,
                       const char **search_path_vars,
                       const char *default_search_path,
                       bool warn_on_fail)
{
   char path[PATH_MAX];
   const char *search_paths, *next, *end;

   search_paths = NULL;
   if (geteuid() == getuid() && search_path_vars) {
         search_paths = getenv(search_path_vars[i]);
   }
   if (search_paths == NULL)
      search_paths = default_search_path;

   for (const char *p = search_paths; p < end; p = next + 1) {

        // 搜索tls目录
      snprintf(path, sizeof(path), "%.*s/tls/%s%s.so", len,
               p, driver_name, lib_suffix);
      driver = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
      if (driver == NULL) {
        // 搜索非tls
         snprintf(path, sizeof(path), "%.*s/%s%s.so", len,
                  p, driver_name, lib_suffix);
         driver = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
      }
         break;
   }

   return driver;
}
```
### 

# state_tracker
## api 
```c
void
st_init_draw_functions(struct pipe_screen *screen,
                       struct dd_function_table *functions)
{
   functions->DrawGallium = st_draw_gallium;
   functions->DrawGalliumMultiMode = st_draw_gallium_multimode;

   if (screen->get_param(screen, PIPE_CAP_DRAW_VERTEX_STATE)) {
      functions->DrawGalliumVertexState = st_draw_gallium_vertex_state;
      functions->CreateGalliumVertexState = st_create_gallium_vertex_state;
   }
}


```
## st_draw_gallium

```c
static void
st_draw_gallium(struct gl_context *ctx,
                struct pipe_draw_info *info,
                unsigned drawid_offset,
                const struct pipe_draw_start_count_bias *draws,
                unsigned num_draws)
{
   struct st_context *st = st_context(ctx);

   prepare_draw(st, ctx, ST_PIPELINE_RENDER_STATE_MASK);

   if (!prepare_indexed_draw(st, ctx, info, draws, num_draws))
      return;

   cso_draw_vbo(st->cso_context, info, drawid_offset, NULL, draws, num_draws);
}



```
  
  
## st_api_query_versions

选择覆盖或者是内部计算

```c
/**
 * Query supported OpenGL versions. (if applicable)
 * The format is (major*10+minor).
 */
void
st_api_query_versions(struct pipe_frontend_screen *fscreen,
                      struct st_config_options *options,
                      int *gl_core_version,
                      int *gl_compat_version,
                      int *gl_es1_version,
                      int *gl_es2_version)
{
   *gl_core_version = get_version(fscreen->screen, options, API_OPENGL_CORE);
   *gl_compat_version = get_version(fscreen->screen, options, API_OPENGL_COMPAT);
   *gl_es1_version = get_version(fscreen->screen, options, API_OPENGLES);
   *gl_es2_version = get_version(fscreen->screen, options, API_OPENGLES2);
}

static unsigned
get_version(struct pipe_screen *screen,
            struct st_config_options *options, gl_api api)
{
   struct gl_constants consts = {0};
   struct gl_extensions extensions = {0};
   GLuint version;

   if (_mesa_override_gl_version_contextless(&consts, &api, &version)) {
      return version;
   }

    /**
     * Initialize fields of gl_constants (aka ctx->Const.*).
     * Use defaults from config.h.  The device drivers will often override
     * some of these values (such as number of texture units).
     */
   _mesa_init_constants(&consts, api);
        ...
        consts->GLSLVersion = api == API_OPENGL_CORE ? 130 : 120;
        consts->GLSLVersionCompat = consts->GLSLVersion;


   _mesa_init_extensions(&extensions);
        ...
        extensions->ARB_ES2_compatibility = GL_TRUE;
        extensions->ARB_draw_elements_base_vertex = GL_TRUE;

   st_init_limits(screen, c = &consts, &extensions, api);
        ...
        c->MaxTextureSize = screen->get_param(screen, PIPE_CAP_MAX_TEXTURE_2D_SIZE);
        c->MaxTextureSize = MIN2(c->MaxTextureSize, 1 << (MAX_TEXTURE_LEVELS - 1));


    /**
     * Use pipe_screen::get_param() to query PIPE_CAP_ values to determine
     * which GL extensions are supported.
     * Quite a few extensions are always supported because they are standard
     * features or can be built on top of other gallium features.
     * Some fine tuning may still be needed.
     */
     // see extension.pdf
   st_init_extensions(screen, &consts, &extensions, options, api);

   version = _mesa_get_version(&extensions, &consts, api);
          switch (api) {
           case API_OPENGL_COMPAT:
              FALLTHROUGH;
           case API_OPENGL_CORE:
              return compute_version(extensions, consts, api);
 
   return version;
}
```

## st_api_create_context

```c
/**
 * Create a rendering context.
 */
struct st_context *
st_api_create_context(struct pipe_frontend_screen *fscreen,
                      const struct st_context_attribs *attribs,
                      enum st_context_error *error,
                      struct st_context *shared_ctx)
{
   struct st_context *st;
   struct pipe_context *pipe;
   struct gl_config mode, *mode_ptr = &mode;
   bool no_error = false;

    // see glapi extension
   _mesa_initialize(attribs->options.mesa_extension_override);

   /* Create a hash table for the framebuffer interface objects
    * if it has not been created for this st manager.
    */
   if (fscreen->st_screen == NULL) {
      struct st_screen *screen;

      screen = CALLOC_STRUCT(st_screen);
      simple_mtx_init(&screen->st_mutex, mtx_plain);
      screen->drawable_ht = _mesa_hash_table_create(NULL,
                                                 drawable_hash,
                                                 drawable_equal);
      fscreen->st_screen = screen;
   }

   if (attribs->flags & ST_CONTEXT_FLAG_NO_ERROR)
      no_error = true;

   pipe = fscreen->screen->context_create(fscreen->screen, NULL,
                                          PIPE_CONTEXT_PREFER_THREADED |
                                          attribs->context_flags);
            [-> llvmpipe llvmpipe_create_context]

   st_visual_to_context_mode(&attribs->visual, &mode);

   st = st_create_context(attribs->profile, pipe, mode_ptr, shared_ctx,
                          &attribs->options, no_error,
                          !!fscreen->validate_egl_image);
   if (!st) {
      *error = ST_CONTEXT_ERROR_NO_MEMORY;
      pipe->destroy(pipe);
      return NULL;
   }

   if (attribs->flags & ST_CONTEXT_FLAG_DEBUG) {
      if (!_mesa_set_debug_state_int(st->ctx, GL_DEBUG_OUTPUT, GL_TRUE)) {
         *error = ST_CONTEXT_ERROR_NO_MEMORY;
         return NULL;
      }

      st->ctx->Const.ContextFlags |= GL_CONTEXT_FLAG_DEBUG_BIT;
   }

   if (st->ctx->Const.ContextFlags & GL_CONTEXT_FLAG_DEBUG_BIT) {
      _mesa_update_debug_callback(st->ctx);
   }

   if (attribs->flags & ST_CONTEXT_FLAG_FORWARD_COMPATIBLE)
      st->ctx->Const.ContextFlags |= GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT;

   if (attribs->context_flags & PIPE_CONTEXT_ROBUST_BUFFER_ACCESS) {
      st->ctx->Const.ContextFlags |= GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT_ARB;
      st->ctx->Const.RobustAccess = GL_TRUE;
   }

   if (attribs->context_flags & PIPE_CONTEXT_LOSE_CONTEXT_ON_RESET) {
      st->ctx->Const.ResetStrategy = GL_LOSE_CONTEXT_ON_RESET_ARB;
      st_install_device_reset_callback(st);
   }

   if (attribs->flags & ST_CONTEXT_FLAG_RELEASE_NONE)
       st->ctx->Const.ContextReleaseBehavior = GL_NONE;

   /* need to perform version check */
   if (attribs->major > 1 || attribs->minor > 0) {
      /* Is the actual version less than the requested version?
       */
      if (st->ctx->Version < attribs->major * 10U + attribs->minor) {
         *error = ST_CONTEXT_ERROR_BAD_VERSION;
         st_destroy_context(st);
         return NULL;
      }
   }

   st->can_scissor_clear = !!st->screen->get_param(st->screen, PIPE_CAP_CLEAR_SCISSORED);

   st->ctx->invalidate_on_gl_viewport =
      fscreen->get_param(fscreen, ST_MANAGER_BROKEN_INVALIDATE);

   st->frontend_screen = fscreen;

   if (st->ctx->IntelBlackholeRender &&
       st->screen->get_param(st->screen, PIPE_CAP_FRONTEND_NOOP))
      st->pipe->set_frontend_noop(st->pipe, st->ctx->IntelBlackholeRender);

   *error = ST_CONTEXT_SUCCESS;
   return st;
}
```

 * 主要涉及默认fb 的创建设置
## st_create_context

```c
struct st_context *
st_create_context(gl_api api, struct pipe_context *pipe,
                  const struct gl_config *visual,
                  struct st_context *share,
                  const struct st_config_options *options,
                  bool no_error, bool has_egl_image_validate)
{
   struct gl_context *ctx;
   struct gl_context *shareCtx = share ? share->ctx : NULL;
   struct dd_function_table funcs;
   struct st_context *st;

   memset(&funcs, 0, sizeof(funcs));
   st_init_driver_functions(pipe->screen, &funcs, has_egl_image_validate);

   /* gl_context must be 16-byte aligned due to the alignment on GLmatrix. */
   ctx = align_malloc(sizeof(struct gl_context), 16);

   ctx->pipe = pipe;
   ctx->screen = pipe->screen;

   if (!_mesa_initialize_context(ctx, api, no_error, visual, shareCtx, &funcs)) {
      align_free(ctx);
      return NULL;
   }

   st_debug_init();


   st = st_create_context_priv(ctx, pipe, options);
   if (!st) {
      _mesa_free_context_data(ctx, true);
      align_free(ctx);
   }

   return st;
}

```

## st_api_make_current

```c
/**
 * Bind the context to the calling thread with draw and read as drawables.
 *
 * The framebuffers might be NULL, meaning the context is surfaceless.
 */
bool
st_api_make_current(struct st_context *st,
                    struct pipe_frontend_drawable *stdrawi,
                    struct pipe_frontend_drawable *streadi)
{
   struct gl_framebuffer *stdraw, *stread;
   bool ret;

   if (st) {
      /* reuse or create the draw fb */
      stdraw = st_framebuffer_reuse_or_create(st, stdrawi);
      if (streadi != stdrawi) {
         /* do the same for the read fb */
         stread = st_framebuffer_reuse_or_create(st, streadi);
      }
      else {
         stread = NULL;
         /* reuse the draw fb for the read fb */
         if (stdraw)
            _mesa_reference_framebuffer(&stread, stdraw);
      }

      /* If framebuffers were asked for, we'd better have allocated them */
      if ((stdrawi && !stdraw) || (streadi && !stread))
         return false;

      if (stdraw && stread) {
             st_framebuffer_validate(stdraw, st);
             if (stread != stdraw)
                st_framebuffer_validate(stread, st);

             ret = _mesa_make_current(st->ctx, stdraw, stread);

             st->draw_stamp = stdraw->stamp - 1;
             st->read_stamp = stread->stamp - 1;
             st_context_validate(st, stdraw, stread);
          }
          else {
             struct gl_framebuffer *incomplete = _mesa_get_incomplete_framebuffer();
             ret = _mesa_make_current(st->ctx, incomplete, incomplete);
          }

          _mesa_reference_framebuffer(&stdraw, NULL);
          _mesa_reference_framebuffer(&stread, NULL);

          /* Purge the context's winsys_buffers list in case any
           * of the referenced drawables no longer exist.
           */
          st_framebuffers_purge(st);
       }
       else {
          GET_CURRENT_CONTEXT(ctx);

          if (ctx) {
             /* Before releasing the context, release its associated
              * winsys buffers first. Then purge the context's winsys buffers list
              * to free the resources of any winsys buffers that no longer have
              * an existing drawable.
              */
             ret = _mesa_make_current(ctx, NULL, NULL);
             st_framebuffers_purge(ctx->st);
          }

          ret = _mesa_make_current(NULL, NULL, NULL);
       }

       return ret;
    }


    ```

    * 该接口 功能就是设置当前ctx的fb  中的draw read buffer 为当前表面

    * 和默认fb有较大关系

    # dri

    ## __driDriverGetExtensions_swarst

    __driDriverGetExtensions_swarst => galliumsw_driver_extensions
    drisw_screen.core => driCoreExtension 
    drisw_screen.swrast  => driSWRastExtension
    drisw_screen.copySubBuffer => driSWCopySubBufferExtension
    drisw_screen.mesa  => mesaCoreExtension

    ```c

    // dri_util.c
    const __DRIswrastExtension driSWRastExtension = {
        .base = { __DRI_SWRAST, 4 },

        .createNewScreen            = driSWRastCreateNewScreen,
        .createNewDrawable          = driCreateNewDrawable,
        .createNewContextForAPI     = driCreateNewContextForAPI,
        .createContextAttribs       = driCreateContextAttribs,
        .createNewScreen2           = driSWRastCreateNewScreen2,
    };


    /** Core interface */
    const __DRIcoreExtension driCoreExtension = {
        .base = { __DRI_CORE, 2 },

        .createNewScreen            = NULL,
        .destroyScreen              = driDestroyScreen,
        .getExtensions              = driGetExtensions,
        .getConfigAttrib            = driGetConfigAttrib,
        .indexConfigAttrib          = driIndexConfigAttrib,
        .createNewDrawable          = NULL,
        .destroyDrawable            = driDestroyDrawable,
        .swapBuffers                = driSwapBuffers, /* swrast */
        .createNewContext           = driCreateNewContext, /* swrast */
        .copyContext                = driCopyContext,
        .destroyContext             = driDestroyContext,
        .bindContext                = driBindContext,
        .unbindContext              = driUnbindContext
    };



    static const struct __DRImesaCoreExtensionRec mesaCoreExtension = {
       .base = { __DRI_MESA, 1 },
       .version_string = MESA_INTERFACE_VERSION_STRING,
       .createNewScreen = driCreateNewScreen2,
       .createContext = driCreateContextAttribs,
       .initScreen = drisw_init_screen,
    };



    //drisw.c
    /* for swrast only */
    const __DRIcopySubBufferExtension driSWCopySubBufferExtension = {
       .base = { __DRI_COPY_SUB_BUFFER, 1 },

       .copySubBuffer               = driswCopySubBuffer,
    };



    // dri_screen.c
    const __DRIconfigOptionsExtension gallium_config_options = {
       .base = { __DRI_CONFIG_OPTIONS, 2 },
       .getXml = pipe_loader_get_driinfo_xml
    };




    /* This is the table of extensions that the loader will dlsym() for. */
    const __DRIextension *galliumsw_driver_extensions[] = {
        &driCoreExtension.base,
        &driSWRastExtension.base,
        &driCopySubBufferExtension.base,
        &gallium_config_options.base,
        NULL
    };


    #define DEFINE_LOADER_DRM_ENTRYPOINT(drivername)                          \
    const __DRIextension **__driDriverGetExtensions_##drivername(void);       \
    PUBLIC const __DRIextension **__driDriverGetExtensions_##drivername(void) \
    {                                                                         \
       return galliumdrm_driver_extensions;                                   \
    }

    #if defined(GALLIUM_SOFTPIPE)

    const __DRIextension **__driDriverGetExtensions_swrast(void);

    PUBLIC const __DRIextension **__driDriverGetExtensions_swrast(void)
    {
       return galliumsw_driver_extensions;
    }
    ```

    ### driSWRastCreateNewScreen/driSWRastCreateNewScreen2

    ```c
    /** swrast driver createNewScreen entrypoint. */
    static __DRIscreen *
    driSWRastCreateNewScreen(int scrn, const __DRIextension **extensions = loader_extensions_shm,
                             const __DRIconfig ***driver_configs, void *data = psc)
    {
       return driCreateNewScreen2(scrn, -1, extensions,
                                  galliumsw_driver_extensions,
                                  driver_configs, data);
    }

    __DRIscreen *
    driCreateNewScreen2(int scrn, int fd = -1 ,
                        const __DRIextension **loader_extensions,
                        const __DRIextension **driver_extensions = galliumsw_driver_extensions ,
                        const __DRIconfig ***driver_configs, void *data)
    {
        static const __DRIextension *emptyExtensionList[] = { NULL };
        struct dri_screen *screen;
        const __DRImesaCoreExtension *mesa = NULL;

        screen = CALLOC_STRUCT(dri_screen);

        assert(driver_extensions);
        for (int i = 0; driver_extensions[i]; i++) {
           if (strcmp(driver_extensions[i]->name, __DRI_MESA) == 0) {
                // bind to  mesaCoreExtension
              mesa = (__DRImesaCoreExtension *)driver_extensions[i];
              
           }
        }

        setupLoaderExtensions(screen, loader_extensions);

        screen->loaderPrivate = data;

        /* This will be filled in by mesa->initScreen(). */
        screen->extensions = emptyExtensionList;
        screen->fd = fd;
        screen->myNum = scrn;

        /* Option parsing before ->InitScreen(), as some options apply there. */
        driParseOptionInfo(&screen->optionInfo,
                           __dri2ConfigOptions, ARRAY_SIZE(__dri2ConfigOptions));
        driParseConfigFiles(&screen->optionCache, &screen->optionInfo, screen->myNum,
                            "dri2", NULL, NULL, NULL, 0, NULL, 0);
                            -> MESA_DRICONF_EXECUTABLE_OVERRIDE 
             initOptionCache(cache, info);
               struct OptConfData userData = {0};
            
               if (!execname)
                  execname = os_get_option("MESA_DRICONF_EXECUTABLE_OVERRIDE");
               if (!execname)
                  execname = util_get_process_name();
            
               userData.cache = cache;
               userData.xxx = ...;
            
            // WITH_XMLCONFIG=1
            #if WITH_XMLCONFIG 
               char *home;
            
                // see xml_config.c 
               parseConfigDir(&userData, datadir);
               parseOneConfigFile(&userData, SYSCONFDIR "/drirc");
            
               if ((home = getenv("HOME"))) {
                  char filename[PATH_MAX];
            
                  snprintf(filename, PATH_MAX, "%s/.drirc", home);
                  parseOneConfigFile(&userData, filename);
               }
            #else
               parseStaticConfig(&userData);
            #endif /* WITH_XMLCONFIG */


        // 调用mesacore 
        *driver_configs = mesa->initScreen(screen);
        [ -> dri  drisw_init_screen]

        struct gl_constants consts = { 0 };
        gl_api api;
        unsigned version;

        api = API_OPENGLES2;
        if (_mesa_override_gl_version_contextless(&consts, &api, &version))
           screen->max_gl_es2_version = version;

        api = API_OPENGL_COMPAT;
        if (_mesa_override_gl_version_contextless(&consts, &api, &version)) {
           screen->max_gl_core_version = version;
           if (api == API_OPENGL_COMPAT)
              screen->max_gl_compat_version = version;
        }

        screen->api_mask = 0;
        if (screen->max_gl_compat_version > 0)
           screen->api_mask |= (1 << __DRI_API_OPENGL);
        if (...)
           screen->api_mask |= (1 << ...);

        return opaque_dri_screen(screen);
            return (__DRIscreen *)screen;
    }
    ```
    * 这里的覆盖只是加入api_mask 

    * WITH_XMLCONFIG

    ```

    # meson.build

    # We don't have expat on Android or Windows, which is a needed dep for xmlconfig
    opt_xmlconfig = get_option('xmlconfig') \
      .require(not (with_platform_android or with_platform_windows),
               error_message : 'xmlconfig not available on Android or Windows')

    if host_machine.system() == 'darwin'
      dep_expat = meson.get_compiler('c').find_library('expat', required : opt_xmlconfig)
    else
      dep_expat = dependency('expat', fallback : ['expat', 'expat_dep'],
                             required : opt_xmlconfig)
    endif
    use_xmlconfig = dep_expat.found()
     

    #src/util/meson.build

    # Only install the drirc file if we build with support for parsing drirc files
    if use_xmlconfig
       install_data(files_drirc, install_dir : join_paths(get_option('datadir'), 'drirc.d'))
    endif

    xmlconfig_deps = []
    if use_xmlconfig
      xmlconfig_deps += dep_expat
    endif
    xmlconfig_deps += dep_regex


    c_xmlconfig_arg = '-DWITH_XMLCONFIG=@0@'.format(use_xmlconfig.to_int())

    ```
    ##### setupLoaderExtensions

    ```c
    static void
    setupLoaderExtensions(struct dri_screen *screen,
                          const __DRIextension **extensions)
    {
       static const struct dri_extension_match matches[] = {
           {__DRI_DRI2_LOADER, 1, offsetof(struct dri_screen, dri2.loader), true},
           {__DRI_IMAGE_LOOKUP, 1, offsetof(struct dri_screen, dri2.image), true},
           {__DRI_USE_INVALIDATE, 1, offsetof(struct dri_screen, dri2.useInvalidate), true},
           {__DRI_BACKGROUND_CALLABLE, 1, offsetof(struct dri_screen, dri2.backgroundCallable), true},
           {__DRI_SWRAST_LOADER, 1, offsetof(struct dri_screen, swrast_loader), true},
           {__DRI_IMAGE_LOADER, 1, offsetof(struct dri_screen, image.loader), true},
           {__DRI_MUTABLE_RENDER_BUFFER_LOADER, 1, offsetof(struct dri_screen, mutableRenderBuffer.loader), true},
           {__DRI_KOPPER_LOADER, 1, offsetof(struct dri_screen, kopper_loader), true},
       };
       loader_bind_extensions(screen, matches, ARRAY_SIZE(matches), extensions);
    }


    ```
    dri_screen.dri2.loader => __DRI_DRI2_LOADER => dri2LoaderExtension

    ```c
    static const __DRIdri2LoaderExtension dri2LoaderExtension = {
       .base = { __DRI_DRI2_LOADER, 3 },

       .getBuffers              = dri2GetBuffers,
       .flushFrontBuffer        = dri2FlushFrontBuffer,
       .getBuffersWithFormat    = dri2GetBuffersWithFormat,
    };

    ```
    ####  _mesa_override_gl_version_contextless

    ```c
    /**
     * Override the context's version and/or API type if the environment variables
     * MESA_GL_VERSION_OVERRIDE or MESA_GLES_VERSION_OVERRIDE are set.
     *
     * Example uses of MESA_GL_VERSION_OVERRIDE:
     *
     * 2.1: select a compatibility (non-Core) profile with GL version 2.1.
     * 3.0: select a compatibility (non-Core) profile with GL version 3.0.
     * 3.0FC: select a Core+Forward Compatible profile with GL version 3.0.
     * 3.1: select GL version 3.1 with GL_ARB_compatibility enabled per the driver default.
     * 3.1FC: select GL version 3.1 with forward compatibility and GL_ARB_compatibility disabled.
     * 3.1COMPAT: select GL version 3.1 with GL_ARB_compatibility enabled.
     * X.Y: override GL version to X.Y without changing the profile.
     * X.YFC: select a Core+Forward Compatible profile with GL version X.Y.
     * X.YCOMPAT: select a Compatibility profile with GL version X.Y.
     *
     * Example uses of MESA_GLES_VERSION_OVERRIDE:
     *
     * 2.0: select GLES version 2.0.
     * 3.0: select GLES version 3.0.
     * 3.1: select GLES version 3.1.
     */
    bool
    _mesa_override_gl_version_contextless(struct gl_constants *consts,
                                          gl_api *apiOut, GLuint *versionOut)
    {
       int version;
       bool fwd_context, compat_context;

        // 根据glcore和compat选择不同的环境变量
       get_gl_override(*apiOut, &version, &fwd_context, &compat_context);
               const char *env_var = (api == API_OPENGL_CORE || api == API_OPENGL_COMPAT)
                    ? "MESA_GL_VERSION_OVERRIDE" : "MESA_GLES_VERSION_OVERRIDE";
                ...
               *version = override[api].version;


       *versionOut = version;
       *apiOut = API_OPENGL_CORE;
       *apiOut = API_OPENGL_COMPAT;
       return false;
    }

    ```

    ### drisw_init_screen

    ```c
    static const __DRIconfig **
    drisw_init_screen(struct dri_screen *screen)
    {
       const __DRIswrastLoaderExtension *loader = screen->swrast_loader;
       const __DRIconfig **configs;
       struct pipe_screen *pscreen = NULL;
       const struct drisw_loader_funcs *lf = &drisw_lf;


       bool success = false;
       // meson.build
       // if dep_libdrm.found()
       //   pre_args += '-DHAVE_LIBDRM'
       //   if with_dri_platform == 'drm' and with_dri
       //     with_gallium_drisw_kms = true
       //   endif
       // endif
    #ifdef HAVE_DRISW_KMS
        // -1 is swrast
       if (screen->fd != -1)
          success = pipe_loader_sw_probe_kms(&screen->dev, screen->fd);
    #endif

        // the same to vulkan 
        // see vulkan 
       if (!success)
          success = pipe_loader_sw_probe_dri(&screen->dev, lf);
       if (success) {
          // see vulkan 
          pscreen = pipe_loader_create_screen(screen->dev);
                return pipe_loader_create_screen_vk(dev, false);

          // 根据dridc文件解析选择
          dri_init_options(screen);
       }

       configs = dri_init_screen_helper(screen, pscreen);

       ...
       return configs;
    }

    ```

    ### dri_init_screen_helper

    ```c
    const __DRIconfig **
    dri_init_screen_helper(struct dri_screen *screen,
                           struct pipe_screen *pscreen)
    {
       screen->base.screen = pscreen;
       screen->base.get_egl_image = dri_get_egl_image;
       screen->base.get_param = dri_get_param;
       screen->base.set_background_context = dri_set_background_context;

       if (pscreen->get_param(pscreen, PIPE_CAP_NPOT_TEXTURES))
            [-> llvmpipe ]
          screen->target = PIPE_TEXTURE_2D;
       else
          screen->target = PIPE_TEXTURE_RECT;

       dri_postprocessing_init(screen);

       st_api_query_versions(&screen->base,
                             &screen->options,
                             &screen->max_gl_core_version,
                             &screen->max_gl_compat_version,
                             &screen->max_gl_es1_version,
                             &screen->max_gl_es2_version);
            [-> state_tracker st_api_query_versions]


       return dri_fill_in_modes(screen);
    }
    ```

    ### driCreateContextAttribs

    ```c
    __DRIcontext *
    driCreateContextAttribs(__DRIscreen *psp, int api,
                            const __DRIconfig *config,
                            __DRIcontext *shared,
                            unsigned num_attribs,
                            const uint32_t *attribs,
                            unsigned *error,
                            void *data)
    {
        struct dri_screen *screen = dri_screen(psp);
        const struct gl_config *modes = (config != NULL) ? &config->modes : NULL;
        gl_api mesa_api;
        struct __DriverContextConfig ctx_config;

        ...

        struct dri_context *ctx = dri_create_context(screen, mesa_api,
                                                     modes, &ctx_config, error,
                                                     dri_context(shared),
                                                     data);
        return opaque_dri_context(ctx);
    }

    struct dri_context *
    dri_create_context(struct dri_screen *screen,
                       gl_api api, const struct gl_config *visual,
                       const struct __DriverContextConfig *ctx_config,
                       unsigned *error,
                       struct dri_context *sharedContextPrivate,
                       void *loaderPrivate)
    {
       struct dri_context *ctx = NULL;
       struct st_context *st_share = NULL;
       struct st_context_attribs attribs;
       enum st_context_error ctx_err = 0;

       /* KHR_no_error is likely to crash, overflow memory, etc if an application
        * has errors so don't enable it for setuid processes.
        */
       if (debug_get_bool_option("MESA_NO_ERROR", false) ||
           driQueryOptionb(&screen->dev->option_cache, "mesa_no_error"))
       }

       attribs.options = screen->options;
       dri_fill_st_visual(&attribs.visual, screen, visual);
       ctx->st = st_api_create_context(&screen->base, &attribs, &ctx_err,
                       st_share);
     
       ctx->st->frontend_context = (void *) ctx;

       if (ctx->st->cso_context) {
          ctx->pp = pp_init(ctx->st->pipe, screen->pp_enabled, ctx->st->cso_context,
                            ctx->st, st_context_invalidate_state);
          ctx->hud = hud_create(ctx->st->cso_context,
                                share_ctx ? share_ctx->hud : NULL,
                                ctx->st, st_context_invalidate_state);
       }

       /* order of precedence (least to most):
        * - driver setting
        * - app setting
        * - user setting
        */
       bool enable_glthread = driQueryOptionb(&screen->dev->option_cache, "mesa_glthread_driver");

       /* always disable glthread by default if fewer than 5 "big" CPUs are active */
       unsigned nr_big_cpus = util_get_cpu_caps()->nr_big_cpus;
       if (util_get_cpu_caps()->nr_cpus < 4 || (nr_big_cpus && nr_big_cpus < 5))
          enable_glthread = false;

       int app_enable_glthread = driQueryOptioni(&screen->dev->option_cache, "mesa_glthread_app_profile");
       if (app_enable_glthread != -1) {
          /* if set (not -1), apply the app setting */
          enable_glthread = app_enable_glthread == 1;
       }
       if (getenv("mesa_glthread")) {
          /* only apply the env var if set */
          bool user_enable_glthread = debug_get_bool_option("mesa_glthread", false);
          if (user_enable_glthread != enable_glthread) {
             /* print warning to mimic old behavior */
             fprintf(stderr, "ATTENTION: default value of option mesa_glthread overridden by environment.");
          }
          enable_glthread = user_enable_glthread;
       }
       /* Do this last. */
       if (enable_glthread) {
          bool safe = true;

          /* This is only needed by X11/DRI2, which can be unsafe. */
          if (backgroundCallable &&
              backgroundCallable->base.version >= 2 &&
              backgroundCallable->isThreadSafe &&
              !backgroundCallable->isThreadSafe(loaderPrivate))
             safe = false;

          if (safe)
             _mesa_glthread_init(ctx->st->ctx);
       }

       *error = __DRI_CTX_ERROR_SUCCESS;
       return ctx;

     fail:
       if (ctx && ctx->st)
          st_destroy_context(ctx->st);

       free(ctx);
       return NULL;
    }




    ```

    ##  driCreateNewDrawable


        struct dri_drawable *drawable =
           screen->create_drawable(screen, &config->modes, GL_FALSE, data);
           [drisw_create_drawable]

    ### drisw_create_drawable

    ```c
    static struct dri_drawable *
    drisw_create_drawable(struct dri_screen *screen, const struct gl_config * visual,
                          boolean isPixmap, void *loaderPrivate)
    {
       struct dri_drawable *drawable = dri_create_drawable(screen, visual, isPixmap,

       drawable->allocate_textures = drisw_allocate_textures;
       drawable->update_drawable_info = drisw_update_drawable_info;
       drawable->flush_frontbuffer = drisw_flush_frontbuffer;
       drawable->update_tex_buffer = drisw_update_tex_buffer;
       drawable->swap_buffers = drisw_swap_buffers;

       return drawable;
    }

    ````

    #### dri_create_drawable

    ```c
    /**
     * This is called when we need to set up GL rendering to a new X window.
     */
    struct dri_drawable *
    dri_create_drawable(struct dri_screen *screen, const struct gl_config *visual,
                        bool isPixmap, void *loaderPrivate)
    {
       struct dri_drawable *drawable = NULL;

       drawable = CALLOC_STRUCT(dri_drawable);

       drawable->loaderPrivate = loaderPrivate;
       drawable->refcount = 1;
       drawable->lastStamp = 0;
       drawable->w = 0;
       drawable->h = 0;

       dri_fill_st_visual(&drawable->stvis, screen, visual);

       /* setup the pipe_frontend_drawable */
       drawable->base.visual = &drawable->stvis;
       drawable->base.flush_front = dri_st_framebuffer_flush_front;
       drawable->base.validate = dri_st_framebuffer_validate;
       drawable->base.flush_swapbuffers = dri_st_framebuffer_flush_swapbuffers;

       drawable->screen = screen;

       p_atomic_set(&drawable->base.stamp, 1);
       drawable->base.ID = p_atomic_inc_return(&drifb_ID);
       drawable->base.fscreen = &screen->base;

       return drawable;
    }
    ```




    ## dri_make_current

    ```c
    GLboolean
    dri_make_current(struct dri_context *ctx,
             struct dri_drawable *draw,
             struct dri_drawable *read)
    {
       /* dri_unbind_context() is always called before this, so drawables are
        * always NULL here.
        */
       assert(!ctx->draw);
       assert(!ctx->read);

       _mesa_glthread_finish(ctx->st->ctx);


       /* Bind drawables to the context */
       ctx->draw = draw;
       ctx->read = read;

       dri_get_drawable(draw);
       draw->texture_stamp = draw->lastStamp - 1;

       if (draw != read) {
          dri_get_drawable(read);
          read->texture_stamp = read->lastStamp - 1;
       }

       st_api_make_current(ctx->st, &draw->base, &read->base);

       /* This is ok to call here. If they are already init, it's a no-op. */
       if (ctx->pp && draw->textures[ST_ATTACHMENT_BACK_LEFT])
          pp_init_fbos(ctx->pp, draw->textures[ST_ATTACHMENT_BACK_LEFT]->width0,
                       draw->textures[ST_ATTACHMENT_BACK_LEFT]->height0);

       return GL_TRUE;
    }

    ```

    # llvmpipe


    ## draw_create_context

    ```c

    /**
     * Create new draw module context with gallivm state for LLVM JIT.
     */
    static struct draw_context *
    draw_create_context(struct pipe_context *pipe, void *context,
                        boolean try_llvm)
    {
       struct draw_context *draw = CALLOC_STRUCT(draw_context);

    #ifdef DRAW_LLVM_AVAILABLE
       if (try_llvm && draw_get_option_use_llvm()) {
          draw->llvm = draw_llvm_create(draw, (LLVMContextRef)context);
       }
    #endif

       if (!draw_init(draw)) {} 

       draw->ia = draw_prim_assembler_create(draw);

       return draw;
    }

    ```

    ### draw_init

    ```c
    boolean
    draw_init(struct draw_context *draw)
    {
       /*
        * Note that several functions compute the clipmask of the predefined
        * formats with hardcoded formulas instead of using these. So modifications
        * here must be reflected there too.
        */

       ASSIGN_4V(draw->plane[0], -1,  0,  0, 1);
       ASSIGN_4V(draw->plane[1],  1,  0,  0, 1);
       ASSIGN_4V(draw->plane[2],  0, -1,  0, 1);
       ASSIGN_4V(draw->plane[3],  0,  1,  0, 1);
       ASSIGN_4V(draw->plane[4],  0,  0,  1, 1); /* yes these are correct */
       ASSIGN_4V(draw->plane[5],  0,  0, -1, 1); /* mesa's a bit wonky */
       draw->clip_xy = TRUE;
       draw->clip_z = TRUE;

       draw->pt.user.planes = (float (*) [DRAW_TOTAL_CLIP_PLANES][4]) &(draw->plane[0]);
       draw->pt.user.eltMax = ~0;

       if (!draw_pipeline_init(draw))
               draw->pipeline.wide_line  = draw_wide_line_stage(draw);
               draw->pipeline.wide_point = draw_wide_point_stage(draw);
               ...
          return FALSE;

       if (!draw_pt_init(draw))
               draw->pt.test_fse = debug_get_option_draw_fse();
               draw->pt.no_fse = debug_get_option_draw_no_fse();
               ...
          return FALSE;

       if (!draw_vs_init(draw))
               draw->vs.emit_cache = translate_cache_create();
               ..
          return FALSE;

        // 对于非tgsi直接返回
       if (!draw_gs_init(draw))
          return FALSE;

       return TRUE;
    }




    ```

    ####  draw_pt_arrays

    ```c

    /* Overall we split things into:
     *     - frontend -- prepare fetch_elts, draw_elts - eg vsplit
     *     - middle   -- fetch, shade, cliptest, viewport
     *     - pipeline -- the prim pipeline: clipping, wide lines, etc
     *     - backend  -- the vbuf_render provided by the driver.
     */
    static boolean
    draw_pt_arrays(struct draw_context *draw,
                   enum pipe_prim_type prim,
                   bool index_bias_varies,
                   const struct pipe_draw_start_count_bias *draw_info,
                   unsigned num_draws)
    {
       enum pipe_prim_type out_prim = prim;

       if (draw->gs.geometry_shader)
          out_prim = draw->gs.geometry_shader->output_primitive;
       else if (draw->tes.tess_eval_shader)
          out_prim = get_tes_output_prim(draw->tes.tess_eval_shader);

       unsigned opt = PT_SHADE;
       if (!draw->render) {
          opt |= PT_PIPELINE;
       }

       if (draw_need_pipeline(draw, draw->rasterizer, out_prim)) {
          opt |= PT_PIPELINE;
       }

       if ((draw->clip_xy ||
             draw->clip_z ||
             draw->clip_user) && !draw->pt.test_fse) {
          opt |= PT_CLIPTEST;
       }

       struct draw_pt_middle_end *middle;
       if (draw->pt.middle.llvm) {
          middle = draw->pt.middle.llvm;
       } else {
          if (opt == PT_SHADE && !draw->pt.no_fse)
             middle = draw->pt.middle.fetch_shade_emit;
          else
             middle = draw->pt.middle.general;
       }

       struct draw_pt_front_end *frontend = draw->pt.frontend;
       if (frontend) {
          if (draw->pt.prim != prim || draw->pt.opt != opt) {
             /* In certain conditions switching primitives requires us to flush
              * and validate the different stages. One example is when smooth
              * lines are active but first drawn with triangles and then with
              * lines.
              */
             draw_do_flush(draw, DRAW_FLUSH_STATE_CHANGE);
             frontend = NULL;
          } else if (draw->pt.eltSize != draw->pt.user.eltSize) {
             /* Flush draw state if eltSize changed.
              * This could be improved so only the frontend is flushed since it
              * converts all indices to ushorts and the fetch part of the middle
              * always prepares both linear and indexed.
              */
             frontend->flush(frontend, DRAW_FLUSH_STATE_CHANGE);
             frontend = NULL;
          }
       }

       if (!frontend) {
          frontend = draw->pt.front.vsplit;

          // vsplit_prepare
          frontend->prepare(frontend, prim, middle, opt);

          draw->pt.frontend = frontend;
          draw->pt.eltSize = draw->pt.user.eltSize;
          draw->pt.prim = prim;
          draw->pt.opt = opt;
       }

       if (draw->pt.rebind_parameters) {
          /* update constants, viewport dims, clip planes, etc */
          middle->bind_parameters(middle);
          draw->pt.rebind_parameters = FALSE;
       }

       for (unsigned i = 0; i < num_draws; i++) {
          /* Sanitize primitive length:
           */
          unsigned first, incr;

          if (prim == PIPE_PRIM_PATCHES) {
             first = draw->pt.vertices_per_patch;
             incr = draw->pt.vertices_per_patch;
          } else {
             draw_pt_split_prim(prim, &first, &incr);
          }

          unsigned count = draw_pt_trim_count(draw_info[i].count, first, incr);
          if (draw->pt.user.eltSize) {
             if (index_bias_varies) {
                draw->pt.user.eltBias = draw_info[i].index_bias;
             } else {
                draw->pt.user.eltBias = draw_info[0].index_bias;
             }
          } else {
             draw->pt.user.eltBias = 0;
          }

          draw->start_index = draw_info[i].start;

          if (count >= first)
             frontend->run(frontend, draw_info[i].start, count);

          if (num_draws > 1 && draw->pt.user.increment_draw_id)
             draw->pt.user.drawid++;
       }

       return TRUE;
    }



    ```
    ## struct 


    ### draw_assembler

    ### draw_context

    constuct : draw_create_context(pipe_context* pipe, void *context, boolean try_llvm)
        .llvm= draw_llvm_create
        .pipe = pipe

    draw_init(struct draw_context *draw)
        draw.pipeline => draw_pipeline_init
        draw.pt => draw_pt_init

    ### draw_llvm

    constuct : draw_llvm_create
        .draw = draw_contex
        .context = LLVMContextCreate
        .context_owned= true
        .nr_variants = 0
        .nr_gs_variants = 0
        .nr_tes_variants = 0
        .nr_tcs_variants = 0


    ## 上下文创建

    llvmpipe_create_context

    ```c
    struct pipe_context *
    llvmpipe_create_context(struct pipe_screen *screen, void *priv,
                            unsigned flags)
    {
       struct llvmpipe_context *llvmpipe;
       struct llvmpipe_screen *lp_screen = llvmpipe_screen(screen);

   if (!llvmpipe_screen_late_init(lp_screen))
      return NULL;

   llvmpipe = align_malloc(sizeof(struct llvmpipe_context), 16);

   list_inithead(&llvmpipe->fs_variants_list.list);

   list_inithead(&llvmpipe->setup_variants_list.list);

   list_inithead(&llvmpipe->cs_variants_list.list);

   llvmpipe->pipe.screen = screen;
   llvmpipe->pipe.priv = priv;

   /* Init the pipe context methods */
   llvmpipe->pipe.destroy = llvmpipe_destroy;
   llvmpipe->pipe.set_framebuffer_state = llvmpipe_set_framebuffer_state;
   llvmpipe->pipe.clear = llvmpipe_clear;
   llvmpipe->pipe.flush = do_flush;
   llvmpipe->pipe.texture_barrier = llvmpipe_texture_barrier;

   llvmpipe->pipe.render_condition = llvmpipe_render_condition;
   llvmpipe->pipe.render_condition_mem = llvmpipe_render_condition_mem;

   llvmpipe->pipe.fence_server_sync = llvmpipe_fence_server_sync;
   llvmpipe->pipe.get_device_reset_status = llvmpipe_get_device_reset_status;
   llvmpipe_init_blend_funcs(llvmpipe);
   llvmpipe_init_clip_funcs(llvmpipe);
   llvmpipe_init_draw_funcs(llvmpipe);
   llvmpipe_init_compute_funcs(llvmpipe);
   llvmpipe_init_sampler_funcs(llvmpipe);
   llvmpipe_init_query_funcs(llvmpipe);
   llvmpipe_init_vertex_funcs(llvmpipe);
   llvmpipe_init_so_funcs(llvmpipe);
   llvmpipe_init_fs_funcs(llvmpipe);
   llvmpipe_init_vs_funcs(llvmpipe);
   llvmpipe_init_gs_funcs(llvmpipe);
   llvmpipe_init_tess_funcs(llvmpipe);
   llvmpipe_init_rasterizer_funcs(llvmpipe);
   llvmpipe_init_context_resource_funcs(&llvmpipe->pipe);
   llvmpipe_init_surface_functions(llvmpipe);

#ifdef USE_GLOBAL_LLVM_CONTEXT
   llvmpipe->context = LLVMGetGlobalContext();
#else
   llvmpipe->context = LLVMContextCreate();
#endif

   if (!llvmpipe->context)
      goto fail;

#if LLVM_VERSION_MAJOR == 15
   LLVMContextSetOpaquePointers(llvmpipe->context, false);
#endif

   /*
    * Create drawing context and plug our rendering stage into it.
    */
    // 创建llvmcontext
   llvmpipe->draw = draw_create_with_llvm_context(&llvmpipe->pipe,
                                                  llvmpipe->context);
            return draw_create_context(pipe, context, TRUE);
   if (!llvmpipe->draw)
      goto fail;

   draw_set_disk_cache_callbacks(llvmpipe->draw,
                                 lp_screen,
                                 lp_draw_disk_cache_find_shader,
                                 lp_draw_disk_cache_insert_shader);

   draw_set_constant_buffer_stride(llvmpipe->draw,
                                   lp_get_constant_buffer_stride(screen));

   /* FIXME: devise alternative to draw_texture_samplers */

   llvmpipe->setup = lp_setup_create(&llvmpipe->pipe, llvmpipe->draw);
   if (!llvmpipe->setup)
      goto fail;

   llvmpipe->csctx = lp_csctx_create(&llvmpipe->pipe);
   if (!llvmpipe->csctx)
      goto fail;

   llvmpipe->pipe.stream_uploader = u_upload_create_default(&llvmpipe->pipe);
   if (!llvmpipe->pipe.stream_uploader)
      goto fail;

   llvmpipe->pipe.const_uploader = llvmpipe->pipe.stream_uploader;

   llvmpipe->blitter = util_blitter_create(&llvmpipe->pipe);
   if (!llvmpipe->blitter) {
      goto fail;
   }

   /* must be done before installing Draw stages */
   util_blitter_cache_all_shaders(llvmpipe->blitter);

   /* plug in AA line/point stages */
   draw_install_aaline_stage(llvmpipe->draw, &llvmpipe->pipe);
   draw_install_aapoint_stage(llvmpipe->draw, &llvmpipe->pipe, nir_type_bool32);
   draw_install_pstipple_stage(llvmpipe->draw, &llvmpipe->pipe);

   /* convert points and lines into triangles:
    * (otherwise, draw points and lines natively)
    */
   draw_wide_point_sprites(llvmpipe->draw, FALSE);
   draw_enable_point_sprites(llvmpipe->draw, FALSE);
   draw_wide_point_threshold(llvmpipe->draw, 10000.0);
   draw_wide_line_threshold(llvmpipe->draw, 10000.0);

   /* initial state for clipping - enabled, with no guardband */
   draw_set_driver_clipping(llvmpipe->draw, FALSE, FALSE, FALSE, TRUE);

   lp_reset_counters();

   /* If llvmpipe_set_scissor_states() is never called, we still need to
    * make sure that derived scissor state is computed.
    * See https://bugs.freedesktop.org/show_bug.cgi?id=101709
    */
   llvmpipe->dirty |= LP_NEW_SCISSOR;

   mtx_lock(&lp_screen->ctx_mutex);
   list_addtail(&llvmpipe->list, &lp_screen->ctx_list);
   mtx_unlock(&lp_screen->ctx_mutex);
   return &llvmpipe->pipe;

 fail:
   llvmpipe_destroy(&llvmpipe->pipe);
   return NULL;
}


```

## vs  创建

```c

static void *
llvmpipe_create_vs_state(struct pipe_context *pipe,
                         const struct pipe_shader_state *templ)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   struct draw_vertex_shader *vs;

   vs = draw_create_vertex_shader(llvmpipe->draw, templ);
   if (!vs) {
      return NULL;
   }

   if (LP_DEBUG & DEBUG_TGSI && templ->type == PIPE_SHADER_IR_TGSI) {
      debug_printf("llvmpipe: Create vertex shader %p:\n", (void *) vs);
      tgsi_dump(templ->tokens, 0);
   }

   return vs;
}




```
## 资源创建

### api

```c
void
llvmpipe_init_screen_resource_funcs(struct pipe_screen *screen)
{
#ifdef DEBUG
   /* init linked list for tracking resources */
   {
      static boolean first_call = TRUE;
      if (first_call) {
         memset(&resource_list, 0, sizeof(resource_list));
         list_inithead(&resource_list.list);
         first_call = FALSE;
      }
   }
#endif

   screen->resource_create = llvmpipe_resource_create;
/*   screen->resource_create_front = llvmpipe_resource_create_front; */
   screen->resource_destroy = llvmpipe_resource_destroy;
   screen->resource_from_handle = llvmpipe_resource_from_handle;
   screen->resource_from_memobj = llvmpipe_resource_from_memobj;
   screen->resource_get_handle = llvmpipe_resource_get_handle;
   screen->can_create_resource = llvmpipe_can_create_resource;

   screen->resource_create_unbacked = llvmpipe_resource_create_unbacked;

   screen->memobj_create_from_handle = llvmpipe_memobj_create_from_handle;
   screen->memobj_destroy = llvmpipe_memobj_destroy;

   screen->resource_get_info = llvmpipe_get_resource_info;
   screen->resource_get_param = llvmpipe_resource_get_param;
   screen->resource_from_user_memory = llvmpipe_resource_from_user_memory;
   screen->allocate_memory = llvmpipe_allocate_memory;
   screen->free_memory = llvmpipe_free_memory;
#ifdef PIPE_MEMORY_FD
   screen->allocate_memory_fd = llvmpipe_allocate_memory_fd;
   screen->import_memory_fd = llvmpipe_import_memory_fd;
   screen->free_memory_fd = llvmpipe_free_memory_fd;
#endif
   screen->map_memory = llvmpipe_map_memory;
   screen->unmap_memory = llvmpipe_unmap_memory;

   screen->resource_bind_backing = llvmpipe_resource_bind_backing;
}



void
llvmpipe_init_context_resource_funcs(struct pipe_context *pipe)
{
   pipe->buffer_map = llvmpipe_transfer_map;
   pipe->buffer_unmap = llvmpipe_transfer_unmap;
   pipe->texture_map = llvmpipe_transfer_map;
   pipe->texture_unmap = llvmpipe_transfer_unmap;

   pipe->transfer_flush_region = u_default_transfer_flush_region;
   pipe->buffer_subdata = u_default_buffer_subdata;
   pipe->texture_subdata = u_default_texture_subdata;

   pipe->memory_barrier = llvmpipe_memory_barrier;
}
```

###  llvmpipe_resource_create

llvmpipe_resource_create -> llvmpipe_resource_create_front  -> llvmpipe_resource_create_all

```c
static struct pipe_resource *
llvmpipe_resource_create_all(struct pipe_screen *_screen,
                             const struct pipe_resource *templat,
                             const void *map_front_private,
                             bool alloc_backing)
{
   struct llvmpipe_screen *screen = llvmpipe_screen(_screen);
   struct llvmpipe_resource *lpr = CALLOC_STRUCT(llvmpipe_resource);
   if (!lpr)
      return NULL;

   lpr->base = *templat;
   lpr->screen = screen;
   pipe_reference_init(&lpr->base.reference, 1);
   lpr->base.screen = &screen->base;

   /* assert(lpr->base.bind); */

   if (llvmpipe_resource_is_texture(&lpr->base)) {
      if (lpr->base.bind & (PIPE_BIND_DISPLAY_TARGET |
                            PIPE_BIND_SCANOUT |
                            PIPE_BIND_SHARED)) {
         /* displayable surface */
         if (!llvmpipe_displaytarget_layout(screen, lpr, map_front_private))
            goto fail;
      } else {
         /* texture map */
         if (!llvmpipe_texture_layout(screen, lpr, alloc_backing))
            goto fail;
      }
   } else {
      /* other data (vertex buffer, const buffer, etc) */
      const uint bytes = templat->width0;
      assert(util_format_get_blocksize(templat->format) == 1);
      assert(templat->height0 == 1);
      assert(templat->depth0 == 1);
      assert(templat->last_level == 0);
      /*
       * Reserve some extra storage since if we'd render to a buffer we
       * read/write always LP_RASTER_BLOCK_SIZE pixels, but the element
       * offset doesn't need to be aligned to LP_RASTER_BLOCK_SIZE.
       */
      /*
       * buffers don't really have stride but it's probably safer
       * (for code doing same calculations for buffers and textures)
       * to put something sane in there.
       */
      lpr->row_stride[0] = bytes;

      lpr->size_required = bytes;
      if (!(templat->flags & PIPE_RESOURCE_FLAG_DONT_OVER_ALLOCATE))
         lpr->size_required += (LP_RASTER_BLOCK_SIZE - 1) * 4 * sizeof(float);

      if (alloc_backing) {
         uint64_t alignment = 64;

         if (templat->flags & PIPE_RESOURCE_FLAG_MAP_PERSISTENT)
            os_get_page_size(&alignment);

         lpr->data = align_malloc(lpr->size_required, alignment);

         if (!lpr->data)
            goto fail;
         memset(lpr->data, 0, bytes);
      }
   }

   lpr->id = id_counter++;

#ifdef DEBUG
   simple_mtx_lock(&resource_list_mutex);
   list_addtail(&lpr->list, &resource_list.list);
   simple_mtx_unlock(&resource_list_mutex);
#endif

   return &lpr->base;

 fail:
   FREE(lpr);
   return NULL;
}


```


### llvmpipe_transfer_map

```




```
##  llvmpipe_create_context


```c
struct pipe_context *
llvmpipe_create_context(struct pipe_screen *screen, void *priv,
                        unsigned flags)
{
   struct llvmpipe_context *llvmpipe;
   struct llvmpipe_screen *lp_screen = llvmpipe_screen(screen);

   if (!llvmpipe_screen_late_init(lp_screen))
      return NULL;

   llvmpipe = align_malloc(sizeof(struct llvmpipe_context), 16);
   if (!llvmpipe)
      return NULL;

   memset(llvmpipe, 0, sizeof *llvmpipe);

   list_inithead(&llvmpipe->fs_variants_list.list);

   list_inithead(&llvmpipe->setup_variants_list.list);

   list_inithead(&llvmpipe->cs_variants_list.list);

   llvmpipe->pipe.screen = screen;
   llvmpipe->pipe.priv = priv;

   /* Init the pipe context methods */
   llvmpipe->pipe.destroy = llvmpipe_destroy;
   llvmpipe->pipe.set_framebuffer_state = llvmpipe_set_framebuffer_state;
   llvmpipe->pipe.clear = llvmpipe_clear;
   llvmpipe->pipe.flush = do_flush;
   llvmpipe->pipe.texture_barrier = llvmpipe_texture_barrier;

   llvmpipe->pipe.render_condition = llvmpipe_render_condition;
   llvmpipe->pipe.render_condition_mem = llvmpipe_render_condition_mem;

   llvmpipe->pipe.fence_server_sync = llvmpipe_fence_server_sync;
   llvmpipe->pipe.get_device_reset_status = llvmpipe_get_device_reset_status;
   llvmpipe_init_blend_funcs(llvmpipe);
   llvmpipe_init_clip_funcs(llvmpipe);
   llvmpipe_init_draw_funcs(llvmpipe);
   llvmpipe_init_compute_funcs(llvmpipe);
   llvmpipe_init_sampler_funcs(llvmpipe);
   llvmpipe_init_query_funcs(llvmpipe);
   llvmpipe_init_vertex_funcs(llvmpipe);
   llvmpipe_init_so_funcs(llvmpipe);
   llvmpipe_init_fs_funcs(llvmpipe);
   llvmpipe_init_vs_funcs(llvmpipe);
   llvmpipe_init_gs_funcs(llvmpipe);
   llvmpipe_init_tess_funcs(llvmpipe);
   llvmpipe_init_rasterizer_funcs(llvmpipe);
   llvmpipe_init_context_resource_funcs(&llvmpipe->pipe);
   llvmpipe_init_surface_functions(llvmpipe);

#ifdef USE_GLOBAL_LLVM_CONTEXT
   llvmpipe->context = LLVMGetGlobalContext();
#else
   llvmpipe->context = LLVMContextCreate();
#endif

   if (!llvmpipe->context)
      goto fail;

#if LLVM_VERSION_MAJOR == 15
   LLVMContextSetOpaquePointers(llvmpipe->context, false);
#endif

   /*
    * Create drawing context and plug our rendering stage into it.
    */
   llvmpipe->draw = draw_create_with_llvm_context(&llvmpipe->pipe,
                                                  llvmpipe->context);
   if (!llvmpipe->draw)
      goto fail;

   draw_set_disk_cache_callbacks(llvmpipe->draw,
                                 lp_screen,
                                 lp_draw_disk_cache_find_shader,
                                 lp_draw_disk_cache_insert_shader);

   draw_set_constant_buffer_stride(llvmpipe->draw,
                                   lp_get_constant_buffer_stride(screen));

   /* FIXME: devise alternative to draw_texture_samplers */

   llvmpipe->setup = lp_setup_create(&llvmpipe->pipe, llvmpipe->draw);
   if (!llvmpipe->setup)
      goto fail;

   llvmpipe->csctx = lp_csctx_create(&llvmpipe->pipe);
   if (!llvmpipe->csctx)
      goto fail;

   llvmpipe->pipe.stream_uploader = u_upload_create_default(&llvmpipe->pipe);
   if (!llvmpipe->pipe.stream_uploader)
      goto fail;

   llvmpipe->pipe.const_uploader = llvmpipe->pipe.stream_uploader;

   llvmpipe->blitter = util_blitter_create(&llvmpipe->pipe);
   if (!llvmpipe->blitter) {
      goto fail;
   }

   /* must be done before installing Draw stages */
   util_blitter_cache_all_shaders(llvmpipe->blitter);

   /* plug in AA line/point stages */
   draw_install_aaline_stage(llvmpipe->draw, &llvmpipe->pipe);
   draw_install_aapoint_stage(llvmpipe->draw, &llvmpipe->pipe, nir_type_bool32);
   draw_install_pstipple_stage(llvmpipe->draw, &llvmpipe->pipe);

   /* convert points and lines into triangles:
    * (otherwise, draw points and lines natively)
    */
   draw_wide_point_sprites(llvmpipe->draw, FALSE);
   draw_enable_point_sprites(llvmpipe->draw, FALSE);
   draw_wide_point_threshold(llvmpipe->draw, 10000.0);
   draw_wide_line_threshold(llvmpipe->draw, 10000.0);

   /* initial state for clipping - enabled, with no guardband */
   draw_set_driver_clipping(llvmpipe->draw, FALSE, FALSE, FALSE, TRUE);

   lp_reset_counters();

   /* If llvmpipe_set_scissor_states() is never called, we still need to
    * make sure that derived scissor state is computed.
    * See https://bugs.freedesktop.org/show_bug.cgi?id=101709
    */
   llvmpipe->dirty |= LP_NEW_SCISSOR;

   mtx_lock(&lp_screen->ctx_mutex);
   list_addtail(&llvmpipe->list, &lp_screen->ctx_list);
   mtx_unlock(&lp_screen->ctx_mutex);
   return &llvmpipe->pipe;

 fail:
   llvmpipe_destroy(&llvmpipe->pipe);
   return NULL;
}

```

### llvmpipe_draw_vbo

```c
static void
llvmpipe_draw_vbo(struct pipe_context *pipe, const struct pipe_draw_info *info,
                  unsigned drawid_offset,
                  const struct pipe_draw_indirect_info *indirect,
                  const struct pipe_draw_start_count_bias *draws,
                  unsigned num_draws)
{
   struct llvmpipe_context *lp = llvmpipe_context(pipe);
   struct draw_context *draw = lp->draw;
   const void *mapped_indices = NULL;
   unsigned i;


   if (lp->dirty)
      llvmpipe_update_derived(lp);

   /*
    * Map vertex buffers
    */
   for (i = 0; i < lp->num_vertex_buffers; i++) {
      const void *buf = lp->vertex_buffer[i].is_user_buffer ?
                           lp->vertex_buffer[i].buffer.user : NULL;
      size_t size = ~0;
      if (!buf) {
         if (!lp->vertex_buffer[i].buffer.resource) {
            continue;
         }
         buf = llvmpipe_resource_data(lp->vertex_buffer[i].buffer.resource);
         size = lp->vertex_buffer[i].buffer.resource->width0;
      }
      draw_set_mapped_vertex_buffer(draw, i, buf, size);
   }

   /* Map index buffer, if present */
   if (info->index_size) {
      unsigned available_space = ~0;
      mapped_indices = info->has_user_indices ? info->index.user : NULL;
      if (!mapped_indices) {
         mapped_indices = llvmpipe_resource_data(info->index.resource);
         available_space = info->index.resource->width0;
      }
      draw_set_indexes(draw,
                       (ubyte *) mapped_indices,
                       info->index_size, available_space);
   }

   llvmpipe_prepare_vertex_sampling(lp,
                                    lp->num_sampler_views[PIPE_SHADER_VERTEX],
                                    lp->sampler_views[PIPE_SHADER_VERTEX]);
   llvmpipe_prepare_geometry_sampling(lp,
                                      lp->num_sampler_views[PIPE_SHADER_GEOMETRY],
                                      lp->sampler_views[PIPE_SHADER_GEOMETRY]);
   llvmpipe_prepare_tess_ctrl_sampling(lp,
                                       lp->num_sampler_views[PIPE_SHADER_TESS_CTRL],
                                       lp->sampler_views[PIPE_SHADER_TESS_CTRL]);
   llvmpipe_prepare_tess_eval_sampling(lp,
                                       lp->num_sampler_views[PIPE_SHADER_TESS_EVAL],
                                       lp->sampler_views[PIPE_SHADER_TESS_EVAL]);

   llvmpipe_prepare_vertex_images(lp,
                                  lp->num_images[PIPE_SHADER_VERTEX],
                                  lp->images[PIPE_SHADER_VERTEX]);
   llvmpipe_prepare_geometry_images(lp,
                                    lp->num_images[PIPE_SHADER_GEOMETRY],
                                    lp->images[PIPE_SHADER_GEOMETRY]);
   llvmpipe_prepare_tess_ctrl_images(lp,
                                     lp->num_images[PIPE_SHADER_TESS_CTRL],
                                     lp->images[PIPE_SHADER_TESS_CTRL]);
   llvmpipe_prepare_tess_eval_images(lp,
                                     lp->num_images[PIPE_SHADER_TESS_EVAL],
                                     lp->images[PIPE_SHADER_TESS_EVAL]);
   if (lp->gs && lp->gs->no_tokens) {
      /* we have an empty geometry shader with stream output, so
         attach the stream output info to the current vertex shader */
      if (lp->vs) {
         draw_vs_attach_so(lp->vs, &lp->gs->stream_output);
      }
   }
   draw_collect_pipeline_statistics(draw,
                                    lp->active_statistics_queries > 0 &&
                                    !lp->queries_disabled);

   draw_collect_primitives_generated(draw,
                                     lp->active_primgen_queries &&
                                     !lp->queries_disabled);

   /* draw! */
   draw_vbo(draw, info, drawid_offset, indirect, draws, num_draws,
            lp->patch_vertices);

   /*
    * unmap vertex/index buffers
    */
   for (i = 0; i < lp->num_vertex_buffers; i++) {
      draw_set_mapped_vertex_buffer(draw, i, NULL, 0);
   }
   if (mapped_indices) {
      draw_set_indexes(draw, NULL, 0, 0);
   }

   if (lp->gs && lp->gs->no_tokens) {
      /* we have attached stream output to the vs for rendering,
         now lets reset it */
      if (lp->vs) {
         draw_vs_reset_so(lp->vs);
      }
   }

   llvmpipe_cleanup_stage_sampling(lp, PIPE_SHADER_VERTEX);
   llvmpipe_cleanup_stage_sampling(lp, PIPE_SHADER_GEOMETRY);
   llvmpipe_cleanup_stage_sampling(lp, PIPE_SHADER_TESS_CTRL);
   llvmpipe_cleanup_stage_sampling(lp, PIPE_SHADER_TESS_EVAL);

   llvmpipe_cleanup_stage_images(lp, PIPE_SHADER_VERTEX);
   llvmpipe_cleanup_stage_images(lp, PIPE_SHADER_GEOMETRY);
   llvmpipe_cleanup_stage_images(lp, PIPE_SHADER_TESS_CTRL);
   llvmpipe_cleanup_stage_images(lp, PIPE_SHADER_TESS_EVAL);

   /*
    * TODO: Flush only when a user vertex/index buffer is present
    * (or even better, modify draw module to do this
    * internally when this condition is seen?)
    */
   draw_flush(draw);
}



```



## llvm中期准备顶点管线



## 更新screen

  1150 try_update_scene_state(struct lp_setup_context *setup)


# 上下文创建主要流程


```

# 初始化screen
glXQueryExtensionsString
glXChooseFBConfig
    __glXInitialize
        driswCreateDisplay

        AllocAndFetchScreenConfigs
            driswCreateScreen
                driswCreateScreenDriver
                    driOpenDriver
                    driSWRastCreateNewScreen2
                        setupLoaderExtensions

                        drisw_init_screen
                        
                        _mesa_override_gl_version_contextless

                   driswBindExtensions

## drisw_init_screen
drisw_init_screen
    pipe_loader_sw_probe_dri
    pipe_loader_create_screen
        pipe_loader_create_screen_vk



# 创建上下文
glXCreateNewContext
    dri_common_create_context
        drisw_create_context_attribs
            driCreateContextAttribs
                dri_create_context
                    st_api_create_context
                        llvmpipe_create_context

## label st_api_create_context
st_api_create_context
    llvmpipe_create_context
    st_create_context
    st_create_context_priv



# 设置当前上下文
glXMakeCurrent
glXMakeContextCurrent
    MakeContextCurrent
        drisw_bind_context
            driFetchDrawable
            driBindContext
                dri_make_current
                    st_api_make_current
## st_api_make_current
st_api_make_current
    st_framebuffer_reuse_or_create
    _mesa_make_current
    st_context_validate

```
# BufferObject 分配流程

# shader 编译流程

## 创建vs 

```c
/**
 * Create LLVM-generated code for a vertex shader.
 */
struct draw_llvm_variant *
draw_llvm_create_variant(struct draw_llvm *llvm,
                         unsigned num_inputs,
                         const struct draw_llvm_variant_key *key)
{
   struct draw_llvm_variant *variant;
   struct llvm_vertex_shader *shader =
      llvm_vertex_shader(llvm->draw->vs.vertex_shader);
   char module_name[64];
   unsigned char ir_sha1_cache_key[20];
   struct lp_cached_code cached = { 0 };
   bool needs_caching = false;
   variant = MALLOC(sizeof *variant +
                    shader->variant_key_size -
                    sizeof variant->key);

   variant->llvm = llvm;
   variant->shader = shader;
   memcpy(&variant->key, key, shader->variant_key_size);

   snprintf(module_name, sizeof(module_name), "draw_llvm_vs_variant%u",
            variant->shader->variants_cached);

    // 创建llvm上下文
   variant->gallivm = gallivm_create(module_name, llvm->context, &cached);

   create_jit_types(variant);

   variant->vertex_header_type = create_jit_vertex_header(variant->gallivm, num_inputs);
   variant->vertex_header_ptr_type = LLVMPointerType(variant->vertex_header_type, 0);

   draw_llvm_generate(llvm, variant);

   gallivm_compile_module(variant->gallivm);

   variant->jit_func = (draw_jit_vert_func)
         gallivm_jit_function(variant->gallivm, variant->function);

   if (needs_caching)
      llvm->draw->disk_cache_insert_shader(llvm->draw->disk_cache_cookie,
                                           &cached,
                                           ir_sha1_cache_key);
   gallivm_free_ir(variant->gallivm);

   variant->list_item_global.base = variant;
   variant->list_item_local.base = variant;
   /*variant->no = */shader->variants_created++;
   variant->list_item_global.base = variant;

   return variant;
}

```



## 产生fs LLVM IR

envvar: LP_DEBUG=fs, ir

file : lp_state_fs.c
func : generate_fragment


### fs 参数
```c
   arg_types[0] = variant->jit_context_ptr_type;       /* context */
   arg_types[1] = int32_type;                          /* x */
   arg_types[2] = int32_type;                          /* y */
   arg_types[3] = int32_type;                          /* facing */
   arg_types[4] = LLVMPointerType(fs_elem_type, 0);    /* a0 */
   arg_types[5] = LLVMPointerType(fs_elem_type, 0);    /* dadx */
   arg_types[6] = LLVMPointerType(fs_elem_type, 0);    /* dady */
   arg_types[7] = LLVMPointerType(int8p_type, 0);  /* color */
   arg_types[8] = int8p_type;       /* depth */
   arg_types[9] = LLVMInt64TypeInContext(gallivm->context);  /* mask_input */
   arg_types[10] = variant->jit_thread_data_ptr_type;  /* per thread data */
   arg_types[11] = int32p_type;     /* stride */
   arg_types[12] = int32_type;                         /* depth_stride */
   arg_types[13] = int32p_type;     /* color sample strides */
   arg_types[14] = int32_type;                         /* depth sample stride */
```


### 产生fs 输入LLVM IR 


lp_build_interp_soa_init(struct lp_build_interp_soa_context *bld,



### 产生 fs LLVM IR ， 深度模板alpha测试IR


```
generate_fs_loop(struct gallivm_state *gallivm,
```

* simple_shader始终位0
* LLVM
####  计算x，y偏移


```
/**
 * Calculate offsets for quad-based interpolation.
 */
static void
calc_offsets(struct lp_build_context *coeff_bld,
             unsigned quad_start_index,
             LLVMValueRef *pixoffx,
             LLVMValueRef *pixoffy)
{
    unsigned num_pix = coeff_bld->type.length; // 确定插值的像素数
    struct gallivm_state *gallivm = coeff_bld->gallivm; // 获取 Gallium LLVM 状态
    LLVMBuilderRef builder = coeff_bld->gallivm->builder; // 获取 LLVM 构建器
    LLVMValueRef nr, pixxf, pixyf; // 定义 LLVM 值，用于存储像素编号和像素偏移量

    *pixoffx = coeff_bld->undef; // 初始化 x 坐标偏移为 undef（未定义值）
    *pixoffy = coeff_bld->undef; // 初始化 y 坐标偏移为 undef（未定义值）

    for (unsigned i = 0; i < num_pix; i++) { // 遍历每个像素
        nr = lp_build_const_int32(gallivm, i); // 创建常量整数值，表示像素编号
        pixxf = lp_build_const_float(gallivm, quad_offset_x[i % num_pix] +
                                     (quad_start_index & 1) * 2); // 计算 x 坐标偏移
        pixyf = lp_build_const_float(gallivm, quad_offset_y[i % num_pix] +
                                     (quad_start_index & 2)); // 计算 y 坐标偏移
        // 将计算得到的偏移量插入到 *pixoffx 和 *pixoffy 中
        *pixoffx = LLVMBuildInsertElement(builder, *pixoffx, pixxf, nr, "");
        *pixoffy = LLVMBuildInsertElement(builder, *pixoffy, pixyf, nr, "");
    }
}

```

## 创建tcs


```c
struct draw_tcs_llvm_variant *
draw_tcs_llvm_create_variant(struct draw_llvm *llvm,
                             unsigned num_outputs,
                             const struct draw_tcs_llvm_variant_key *key)
{
   struct draw_tcs_llvm_variant *variant;
   struct llvm_tess_ctrl_shader *shader = llvm_tess_ctrl_shader(llvm->draw->tcs.tess_ctrl_shader);
   char module_name[64];
   unsigned char ir_sha1_cache_key[20];
   struct lp_cached_code cached = { 0 };
   bool needs_caching = false;

   variant = MALLOC(sizeof *variant +
                    shader->variant_key_size - sizeof variant->key);
   if (!variant)
      return NULL;

   variant->llvm = llvm;
   variant->shader = shader;

   snprintf(module_name, sizeof(module_name), "draw_llvm_tcs_variant%u",
            variant->shader->variants_cached);

   memcpy(&variant->key, key, shader->variant_key_size);

   if (shader->base.state.ir.nir && llvm->draw->disk_cache_cookie) {
      draw_get_ir_cache_key(shader->base.state.ir.nir,
                            key,
                            shader->variant_key_size,
                            num_outputs,
                            ir_sha1_cache_key);

      llvm->draw->disk_cache_find_shader(llvm->draw->disk_cache_cookie,
                                         &cached,
                                         ir_sha1_cache_key);
      if (!cached.data_size)
         needs_caching = true;
   }

   variant->gallivm = gallivm_create(module_name, llvm->context, &cached);

   create_tcs_jit_types(variant);

   if (gallivm_debug & (GALLIVM_DEBUG_TGSI | GALLIVM_DEBUG_IR)) {
      nir_print_shader(llvm->draw->tcs.tess_ctrl_shader->state.ir.nir, stderr);
      draw_tcs_llvm_dump_variant_key(&variant->key);
   }

   draw_tcs_llvm_generate(llvm, variant);

   gallivm_compile_module(variant->gallivm);

   variant->jit_func = (draw_tcs_jit_func)
      gallivm_jit_function(variant->gallivm, variant->function);

   if (needs_caching)
      llvm->draw->disk_cache_insert_shader(llvm->draw->disk_cache_cookie,
                                           &cached,
                                           ir_sha1_cache_key);
   gallivm_free_ir(variant->gallivm);

   variant->list_item_global.base = variant;
   variant->list_item_local.base = variant;
   /*variant->no = */shader->variants_created++;
   variant->list_item_global.base = variant;

   return variant;
}

```


## 创建tes

```c
struct draw_tes_llvm_variant *
draw_tes_llvm_create_variant(struct draw_llvm *llvm,
                             unsigned num_outputs,
                             const struct draw_tes_llvm_variant_key *key)
{
   struct draw_tes_llvm_variant *variant;
   struct llvm_tess_eval_shader *shader = llvm_tess_eval_shader(llvm->draw->tes.tess_eval_shader);
   char module_name[64];
   unsigned char ir_sha1_cache_key[20];
   struct lp_cached_code cached = { 0 };
   bool needs_caching = false;

   variant = MALLOC(sizeof *variant +
                    shader->variant_key_size - sizeof variant->key);
   if (!variant)
      return NULL;

   variant->llvm = llvm;
   variant->shader = shader;

   snprintf(module_name, sizeof(module_name), "draw_llvm_tes_variant%u",
            variant->shader->variants_cached);

   memcpy(&variant->key, key, shader->variant_key_size);
   if (shader->base.state.ir.nir && llvm->draw->disk_cache_cookie) {
      draw_get_ir_cache_key(shader->base.state.ir.nir,
                            key,
                            shader->variant_key_size,
                            num_outputs,
                            ir_sha1_cache_key);

      llvm->draw->disk_cache_find_shader(llvm->draw->disk_cache_cookie,
                                         &cached,
                                         ir_sha1_cache_key);
      if (!cached.data_size)
         needs_caching = true;
   }
   variant->gallivm = gallivm_create(module_name, llvm->context, &cached);

   create_tes_jit_types(variant);

   variant->vertex_header_type = create_jit_vertex_header(variant->gallivm, num_outputs);
   variant->vertex_header_ptr_type = LLVMPointerType(variant->vertex_header_type, 0);

   if (gallivm_debug & (GALLIVM_DEBUG_TGSI | GALLIVM_DEBUG_IR)) {
      nir_print_shader(llvm->draw->tes.tess_eval_shader->state.ir.nir, stderr);
      draw_tes_llvm_dump_variant_key(&variant->key);
   }

   draw_tes_llvm_generate(llvm, variant);

   gallivm_compile_module(variant->gallivm);

   variant->jit_func = (draw_tes_jit_func)
      gallivm_jit_function(variant->gallivm, variant->function);

   if (needs_caching)
      llvm->draw->disk_cache_insert_shader(llvm->draw->disk_cache_cookie,
                                           &cached,
                                           ir_sha1_cache_key);
   gallivm_free_ir(variant->gallivm);

   variant->list_item_global.base = variant;
   variant->list_item_local.base = variant;
   /*variant->no = */shader->variants_created++;
   variant->list_item_global.base = variant;

   return variant;
}

```

## 创建cs

see cs.pdf

## 创建gs

```c
struct draw_gs_llvm_variant *
draw_gs_llvm_create_variant(struct draw_llvm *llvm,
                            unsigned num_outputs,
                            const struct draw_gs_llvm_variant_key *key)
{
   struct draw_gs_llvm_variant *variant;
   struct llvm_geometry_shader *shader =
      llvm_geometry_shader(llvm->draw->gs.geometry_shader);
   char module_name[64];
   unsigned char ir_sha1_cache_key[20];
   struct lp_cached_code cached = { 0 };
   bool needs_caching = false;

   variant = MALLOC(sizeof *variant +
                    shader->variant_key_size -
                    sizeof variant->key);
   if (!variant)
      return NULL;

   variant->llvm = llvm;
   variant->shader = shader;

   snprintf(module_name, sizeof(module_name), "draw_llvm_gs_variant%u",
            variant->shader->variants_cached);

   memcpy(&variant->key, key, shader->variant_key_size);

   if (shader->base.state.ir.nir && llvm->draw->disk_cache_cookie) {
      draw_get_ir_cache_key(shader->base.state.ir.nir,
                            key,
                            shader->variant_key_size,
                            num_outputs,
                            ir_sha1_cache_key);

      llvm->draw->disk_cache_find_shader(llvm->draw->disk_cache_cookie,
                                         &cached,
                                         ir_sha1_cache_key);
      if (!cached.data_size)
         needs_caching = true;
   }
   variant->gallivm = gallivm_create(module_name, llvm->context, &cached);

   create_gs_jit_types(variant);

   variant->vertex_header_type = create_jit_vertex_header(variant->gallivm, num_outputs);
   variant->vertex_header_ptr_type = LLVMPointerType(variant->vertex_header_type, 0);

   draw_gs_llvm_generate(llvm, variant);

   gallivm_compile_module(variant->gallivm);

   variant->jit_func = (draw_gs_jit_func)
         gallivm_jit_function(variant->gallivm, variant->function);

   if (needs_caching)
      llvm->draw->disk_cache_insert_shader(llvm->draw->disk_cache_cookie,
                                           &cached,
                                           ir_sha1_cache_key);
   gallivm_free_ir(variant->gallivm);

   variant->list_item_global.base = variant;
   variant->list_item_local.base = variant;
   /*variant->no = */shader->variants_created++;
   variant->list_item_global.base = variant;

   return variant;
}





```
# LLVM JIT

LLVM JIT（即时编译）是 LLVM（Low Level Virtual Machine）编译器基础架构的一部分，它允许在运行时动态地将高级语言的代码编译成机器代码。JIT编译与传统的静态编译不同，静态编译是在程序执行之前将源代码编译成机器代码，而JIT编译则在程序运行时将源代码或中间表示（IR）编译成机器代码。

## JIT的使用示列

```cpp

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

int main() {
  LLVMContext Context;
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();

  // Create a new LLVM module and set up the builder.
  std::unique_ptr<Module> M = std::make_unique<Module>("JIT Example", Context);
  IRBuilder<> Builder(Context);

  // Define a simple function: int add(int a, int b) { return a + b; }
  FunctionType *FuncType =
      FunctionType::get(Type::getInt32Ty(Context),
                        {Type::getInt32Ty(Context), Type::getInt32Ty(Context)},
                        false);

  Function *AddFunc =
      Function::Create(FuncType, Function::ExternalLinkage, "add", M.get());
  BasicBlock *BB = BasicBlock::Create(Context, "entry", AddFunc);
  Builder.SetInsertPoint(BB);

  Value *Arg1 = AddFunc->arg_begin();
  Value *Arg2 = std::next(AddFunc->arg_begin());
  Value *Sum = Builder.CreateAdd(Arg1, Arg2);
  Builder.CreateRet(Sum);

  // Print LLVM IR for reference.
  outs() << "LLVM IR:\n";
  M->print(outs(), nullptr);

  // Create MCJIT execution engine.
  std::string ErrorStr;
  ExecutionEngine *EE = EngineBuilder(std::move(M))
                           .setErrorStr(&ErrorStr)
                           .setEngineKind(EngineKind::JIT)
                           .create();

  if (!EE) {
    errs() << "Failed to create MCJIT: " << ErrorStr << "\n";
    return 1;
  }

  // Get a pointer to the JIT-compiled function.
  auto AddFnPtr = (int (*)(int, int))EE->getFunctionAddress("add");

  if (!AddFnPtr) {
    errs() << "Failed to get pointer to add function\n";
    return 1;
  }

  // Call the JIT-compiled function.
  int Result = AddFnPtr(3, 4);
  outs() << "Result: " << Result << "\n";

  return 0;
}

```



## MCJIT设计与实现

### 介绍

本文档描述了MCJIT执行引擎和RuntimeDyld组件的内部工作原理。它旨在作为实现的高级概述，展示了在代码生成和动态加载过程中对象的流程和交互。

### 引擎创建

在大多数情况下，使用EngineBuilder对象来创建MCJIT执行引擎的实例。EngineBuilder接受一个llvm::Module对象作为其构造函数的参数。客户端可以设置各种选项，这些选项稍后将传递给MCJIT引擎，包括选择MCJIT作为要创建的引擎类型。特别关注的是EngineBuilder::setMCJITMemoryManager函数。如果客户端在此时没有显式创建内存管理器，那么在实例化MCJIT引擎时将创建一个默认内存管理器（具体来说是SectionMemoryManager）。

一旦选项设置完毕，客户端调用EngineBuilder::create来创建MCJIT引擎的实例。如果客户端不使用带有TargetMachine参数的此函数形式，将根据与用于创建EngineBuilder的Module相关联的目标三元组创建一个新的TargetMachine。

![MCJIT Engine Builder](path_to_image/MCJIT-engine-builder.png)

EngineBuilder::create将调用静态MCJIT::createJIT函数，传递其指向模块、内存管理器和目标机器对象的指针，这些对象随后都将由MCJIT对象拥有。

MCJIT类有一个成员变量Dyld，其中包含RuntimeDyld包装类的实例。此成员将用于MCJIT与在加载对象时创建的实际RuntimeDyldImpl对象之间的通信。

![MCJIT Creation](path_to_image/MCJIT-creation.png)

创建时，MCJIT持有从EngineBuilder接收的Module对象的指针，但它不会立即为此模块生成代码。代码生成被推迟，直到显式调用MCJIT::finalizeObject方法或调用需要生成代码的函数，例如MCJIT::getPointerToFunction。


这段文字描述了 LLVM 的 MCJIT 在代码生成和加载过程中的一些关键步骤。我将其翻译为中文：

### 代码生成

当触发代码生成时，MCJIT 首先尝试从其 ObjectCache 成员中获取对象映像（如果已设置）。如果无法检索缓存的对象映像，则 MCJIT 将调用其 emitObject 方法。MCJIT::emitObject 使用本地的 PassManager 实例，并创建一个新的 ObjectBufferStream 实例，将二者传递给 TargetMachine::addPassesToEmitMC，然后调用 PassManager::run 在其创建的 Module 上。

![MCJIT加载](URL_TO_IMAGE/MCJIT-load.png)

PassManager::run 调用导致 MC 代码生成机制生成一个完整的可重定位二进制对象映像（格式可能是 ELF 或 MachO，取决于目标），并将其刷新到 ObjectBufferStream 对象以完成过程。如果正在使用 ObjectCache，则图像将在此处传递给 ObjectCache。

此时，ObjectBufferStream 包含原始的对象映像。在执行代码之前，必须将此映像的代码和数据部分加载到适当的内存中，应用重定位，并完成内存权限和代码缓存失效（如果需要）。

### 对象加载

一旦获得了对象映像（通过代码生成或从 ObjectCache 中检索），它将被传递给 RuntimeDyld 进行加载。RuntimeDyld 包装类检查对象以确定其文件格式，并创建 RuntimeDyldELF 或 RuntimeDyldMachO 的实例（两者都派生自 RuntimeDyldImpl 基类），然后调用 RuntimeDyldImpl::loadObject 方法执行实际加载。

![MCJIT加载对象](URL_TO_IMAGE/MCJIT-dyld-load.png)

RuntimeDyldImpl::loadObject 从其接收的 ObjectBuffer 创建一个 ObjectImage 实例。ObjectImage 封装了 ObjectFile 类，是一个帮助类，解析二进制对象映像并提供对格式特定头部中包含的信息的访问，包括部分、符号和重定位信息。

然后，RuntimeDyldImpl::loadObject 遍历图像中的符号。收集有关通用符号的信息以供以后使用。对于每个函数或数据符号，将加载相关的部分到内存中，并将符号存储在符号表映射数据结构中。遍历完成后，为通用符号发出一个部分。

接下来，RuntimeDyldImpl::loadObject 遍历对象映像中的部分，并对每个部分的重定位进行迭代。对于每个重定位，它调用格式特定的 processRelocationRef 方法，该方法将检查重定位并将其存储在两个数据结构中，一个基于部分的重定位列表映射和一个外部符号重定位映射。

![MCJIT加载对象](URL_TO_IMAGE/MCJIT-load-object.png)

当 RuntimeDyldImpl::loadObject 返回时，对象的所有代码和数据部分将已加载到由内存管理器分配的内存中，重定位信息已准备好，但尚未应用重定位，生成的代码仍未准备好执行。

[当前（截至 2013 年 8 月）MCJIT 引擎将在 loadObject 完成时立即应用重定位。但是，这不应该发生。因为代码可能已为远程目标生成，客户端应该有机会在应用重定位之前重新映射部分地址。可以多次应用重定位，但在地址需要重新映射的情况下，此第一次应用是徒劳的。]

### 地址重映射

在生成初始代码后且在调用 finalizeObject 之前的任何时候，客户端都可以重新映射对象中部分的地址。通常，这是因为代码是为外部进程生成的，正在映射到该进程的地址空间中。客户端通过调用 MCJIT::mapSectionAddress 来重新映射部分地址。这应该在将部分内存复制到新位置之前完成。

当调用 MCJIT::mapSectionAddress 时，MCJIT 将调用 RuntimeDyldImpl（通过其 Dyld 成员）。RuntimeDyldImpl 将新地址存储在内部数据结构中，但此时不会更新代码，因为其他部分可能会更改。

当客户端完成重新映射部分地址时，将调用 MCJIT::finalizeObject 来完成重新映射过程。

### 最终准备

当调用 MCJIT::finalizeObject 时，MCJIT 调用 RuntimeDyld::resolveRelocations。此函数将尝试定位任何外部符号，然后应用对象的所有重定位。

外部符号通过调用内存管理器的 getPointerToNamedFunction 方法解析。内存管理器将返回目标地址空间中所请求符号的地址。 （注意，这可能不是主机进程中的有效指针。）然后，RuntimeDyld 将遍历其存储的与此符号关联的重定位列表，并调用 resolveRelocation 方法，通过格式特定的实现将重定位应用于加载的部分内存。

接下来，RuntimeDyld::resolveRelocations 遍历部分列表，并对于每个部分，遍历一个保存的与该符号引用的重定位列表，并为该列表中的每个条目调用 resolveRelocation。此处的重定位列表是一个重定位列表，其中与与该列表相关联的符号位于关联列表中的部分中。这些位置的每个位置都将有一个目标位置，该位置很可能位于不同的部分中。

![MCJIT解决重定位](URL_TO_IMAGE/MCJIT-resolve-relocations.png)

一旦按上述方式应用了重定位，MCJ

IT 将调用 RuntimeDyld::getEHFrameSection，如果返回非零结果，则将部分数据传递给内存管理器的 registerEHFrames 方法。这使得内存管理器可以调用任何所需的特定于目标的函数，例如将 EH frame 信息与调试器注册。

最后，MCJIT 调用内存管理器的 finalizeMemory 方法。在此方法中，内存管理器将在必要时使目标代码缓存失效，并对其为代码和数据内存分配的内存页应用最终权限。


# SectionMemoryManager

`SectionMemoryManager` 是 LLVM 提供的一个内存管理器类，用于管理运行时生成的代码的内存分配和释放。它是 LLVM 中的一部分，特别是在使用 MCJIT（Machine Code Just-In-Time Compilation，机器码即时编译）引擎时，常常与动态生成的代码一起使用。

主要功能和作用包括：

1. **代码和数据内存分配：** `SectionMemoryManager` 负责为 JIT 编译生成的代码和数据分配内存。当 MCJIT 引擎需要将 LLVM IR 编译为机器码时，`SectionMemoryManager` 负责为生成的代码和数据分配适当的内存。

2. **内存权限设置：** 它也可以设置生成的代码和数据内存的权限，例如可执行、可读或可写等。

3. **代码缓存失效：** 在某些情况下，为了确保生成的代码被正确执行，可能需要对代码缓存进行失效。`SectionMemoryManager` 可以处理这方面的任务。

4. **EH Frame 注册：** 当运行时异常处理（Exception Handling，EH）框架需要注册时，`SectionMemoryManager` 可以处理注册 EH frame 的逻辑。

使用 `SectionMemoryManager` 的一个常见场景是在 MCJIT 的上下文中，通过设置 `EngineBuilder::setMCJITMemoryManager` 方法来为 MCJIT 引擎指定内存管理器。以下是一个简单的示例：

```cpp
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"

int main() {
    // 创建一个 SectionMemoryManager 实例
    llvm::SectionMemoryManager *memoryManager = new llvm::SectionMemoryManager();

    // 创建一个 MCJIT 引擎，并将 SectionMemoryManager 设置为其内存管理器
    llvm::EngineBuilder builder(/* ... other arguments ... */);
    builder.setMCJITMemoryManager(std::unique_ptr<llvm::SectionMemoryManager>(memoryManager));
    llvm::ExecutionEngine *engine = builder.create();

    // 在这里可以使用 MCJIT 引擎执行 JIT 编译和执行的操作

    // 释放资源
    delete engine;

    return 0;
}
```

在这个示例中，我们首先创建了一个 `SectionMemoryManager` 实例，然后将其设置为 MCJIT 引擎的内存管理器。这允许我们自定义内存分配和其他与内存相关的操作，以满足特定的需求。请注意，`SectionMemoryManager` 是一个相对基本的内存管理器，对于更高级的需求，可能需要实现自定义的内存管理器。

# 环境变量

## LIBGL_DIAGNOSTIC

__glXInitializeVisualConfigFromTags

## MESA_NO_ERROR


## MESA_EXTENSION_OVERRIDE

one_time_init


## GALLIUM_DUMP_VS
dump vs

## GALLIUM_PRINT_OPTIONS
打印环境变量选择

## DRAW_USE_LLVM = true


## GALLIVM_LLC_OPTIONS 

init_native_targets

## GALLIVM_DEBUG


```c 
static const struct debug_named_value lp_bld_debug_flags[] = {
   { "tgsi",   GALLIVM_DEBUG_TGSI, NULL },
   { "ir",     GALLIVM_DEBUG_IR, NULL },
   { "asm",    GALLIVM_DEBUG_ASM, NULL },
   { "perf",   GALLIVM_DEBUG_PERF, NULL },
   { "gc",     GALLIVM_DEBUG_GC, NULL },
/* Don't allow setting DUMP_BC for release builds, since writing the files may be an issue with setuid. */
#ifdef DEBUG
   { "dumpbc", GALLIVM_DEBUG_DUMP_BC, NULL },
#endif
   DEBUG_NAMED_VALUE_END
};

```
lp_bld_init.c

## LP_NATIVE_VECTOR_WIDTH 


```c
   // Default to 256 until we're confident llvmpipe with 512 is as correct and not slower than 256
   lp_native_vector_width = MIN2(util_get_cpu_caps()->max_vector_bits, 256);

   lp_native_vector_width = debug_get_num_option("LP_NATIVE_VECTOR_WIDTH",
                                                 lp_native_vector_width);
```




## LP_DEBUG


```c
static const struct debug_named_value lp_debug_flags[] = {
   { "pipe", DEBUG_PIPE, NULL },
   { "tgsi", DEBUG_TGSI, NULL },
   { "tex", DEBUG_TEX, NULL },
   { "setup", DEBUG_SETUP, NULL },
   { "rast", DEBUG_RAST, NULL },
   { "query", DEBUG_QUERY, NULL },
   { "screen", DEBUG_SCREEN, NULL },
   { "counters", DEBUG_COUNTERS, NULL },
   { "scene", DEBUG_SCENE, NULL },
   { "fence", DEBUG_FENCE, NULL },
   { "no_fastpath", DEBUG_NO_FASTPATH, NULL },
   { "linear", DEBUG_LINEAR, NULL },
   { "linear2", DEBUG_LINEAR2, NULL },
   { "mem", DEBUG_MEM, NULL },
   { "fs", DEBUG_FS, NULL },
   { "cs", DEBUG_CS, NULL },
   { "tgsi_ir", DEBUG_TGSI_IR, NULL },
   { "accurate_a0", DEBUG_ACCURATE_A0 },
   DEBUG_NAMED_VALUE_END
};

```
* mem在分配data block时使用调试
## LP_NO_RAST

禁止光栅化lp_rast.c:lp_rast_create

# API -> llvmpipe 



glFlush -> llvmpipe_flush



# llvm draw

## draw_pt_init

```c
boolean
draw_pt_init(struct draw_context *draw)
{
   draw->pt.test_fse = debug_get_option_draw_fse();
   draw->pt.no_fse = debug_get_option_draw_no_fse();

   draw->pt.front.vsplit = draw_pt_vsplit(draw);

   draw->pt.middle.fetch_shade_emit = draw_pt_middle_fse(draw);

   draw->pt.middle.general = draw_pt_fetch_pipeline_or_emit(draw);

#ifdef DRAW_LLVM_AVAILABLE
   if (draw->llvm)
      draw->pt.middle.llvm = draw_pt_fetch_pipeline_or_emit_llvm(draw);
#endif

   return TRUE;
}



```
draw_init


### draw LLVM IR 产生


```c
static void
draw_llvm_generate(struct draw_llvm *llvm, struct draw_llvm_variant *variant)
{
   struct gallivm_state *gallivm = variant->gallivm;
   LLVMContextRef context = gallivm->context;
   LLVMTypeRef int32_type = LLVMInt32TypeInContext(context);
   LLVMTypeRef arg_types[13];
   unsigned num_arg_types = ARRAY_SIZE(arg_types);
   LLVMTypeRef func_type;
   LLVMValueRef context_ptr;
   LLVMBasicBlockRef block;
   LLVMBuilderRef builder;
   char func_name[64];
   struct lp_type vs_type;
   LLVMValueRef count, fetch_elts, start;
   LLVMValueRef vertex_id_offset;
   LLVMValueRef stride, step, io_itr;
   LLVMValueRef ind_vec, start_vec, have_elts, fetch_max, tmp;
   LLVMValueRef io_ptr, vbuffers_ptr, vb_ptr;
   LLVMValueRef vb_stride[PIPE_MAX_ATTRIBS];
   LLVMValueRef map_ptr[PIPE_MAX_ATTRIBS];
   LLVMValueRef buffer_size_adj[PIPE_MAX_ATTRIBS];
   LLVMValueRef instance_index[PIPE_MAX_ATTRIBS];
   LLVMValueRef fake_buf_ptr, fake_buf;

   struct draw_context *draw = llvm->draw;
   const struct tgsi_shader_info *vs_info = &draw->vs.vertex_shader->info;
   unsigned i, j;
   struct lp_build_context bld, blduivec;
   struct lp_build_loop_state lp_loop;
   struct lp_build_if_state if_ctx;
   const int vector_length = lp_native_vector_width / 32;
   LLVMValueRef outputs[PIPE_MAX_SHADER_OUTPUTS][TGSI_NUM_CHANNELS];
   struct lp_build_sampler_soa *sampler = 0;
   struct lp_build_image_soa *image = NULL;
   LLVMValueRef ret, clipmask_bool_ptr;
   struct draw_llvm_variant_key *key = &variant->key;
   /* If geometry shader is present we need to skip both the viewport
    * transformation and clipping otherwise the inputs to the geometry
    * shader will be incorrect.
    * The code can't handle vp transform when vs writes vp index neither
    * (though this would be fixable here, but couldn't just broadcast
    * the values).
    */
   const boolean bypass_viewport = key->has_gs_or_tes || key->bypass_viewport ||
                                   vs_info->writes_viewport_index;
   const boolean enable_cliptest = !key->has_gs_or_tes && (key->clip_xy ||
                                                    key->clip_z ||
                                                    key->clip_user ||
                                                    key->need_edgeflags);
   LLVMValueRef variant_func;
   const unsigned pos = draw->vs.position_output;
   const unsigned cv = draw->vs.clipvertex_output;
   boolean have_clipdist = FALSE;
   struct lp_bld_tgsi_system_values system_values;

   memset(&system_values, 0, sizeof(system_values));
   memset(&outputs, 0, sizeof(outputs));
   snprintf(func_name, sizeof(func_name), "draw_llvm_vs_variant");

   i = 0;
   arg_types[i++] = get_context_ptr_type(variant);       /* context */
   arg_types[i++] = get_vertex_header_ptr_type(variant); /* vertex_header */
   arg_types[i++] = get_buffer_ptr_type(variant);        /* vbuffers */
   arg_types[i++] = int32_type;                          /* count */
   arg_types[i++] = int32_type;                          /* start/fetch_elt_max */
   arg_types[i++] = int32_type;                          /* stride */
   arg_types[i++] = get_vb_ptr_type(variant);            /* pipe_vertex_buffer's */
   arg_types[i++] = int32_type;                          /* instance_id */
   arg_types[i++] = int32_type;                          /* vertex_id_offset */
   arg_types[i++] = int32_type;                          /* start_instance */
   arg_types[i++] = LLVMPointerType(int32_type, 0);      /* fetch_elts  */
   arg_types[i++] = int32_type;                          /* draw_id */
   arg_types[i++] = int32_type;                          /* view_id */

   func_type = LLVMFunctionType(LLVMInt8TypeInContext(context),
                                arg_types, num_arg_types, 0);

   variant_func = LLVMAddFunction(gallivm->module, func_name, func_type);
   variant->function = variant_func;

   LLVMSetFunctionCallConv(variant_func, LLVMCCallConv);
   for (i = 0; i < num_arg_types; ++i)
      if (LLVMGetTypeKind(arg_types[i]) == LLVMPointerTypeKind)
         lp_add_function_attr(variant_func, i + 1, LP_FUNC_ATTR_NOALIAS);

   if (gallivm->cache && gallivm->cache->data_size)
      return;

   context_ptr               = LLVMGetParam(variant_func, 0);
   io_ptr                    = LLVMGetParam(variant_func, 1);
   vbuffers_ptr              = LLVMGetParam(variant_func, 2);
   count                     = LLVMGetParam(variant_func, 3);
   start                     = LLVMGetParam(variant_func, 4);
   /*
    * XXX: stride is actually unused. The stride we use is strictly calculated
    * from the number of outputs (including the draw_extra outputs).
    * Should probably fix some day (we need a new vs just because of extra
    * outputs which the generated vs won't touch).
    */
   stride                    = LLVMGetParam(variant_func, 5);
   vb_ptr                    = LLVMGetParam(variant_func, 6);
   system_values.instance_id = LLVMGetParam(variant_func, 7);
   vertex_id_offset          = LLVMGetParam(variant_func, 8);
   system_values.base_instance = LLVMGetParam(variant_func, 9);
   fetch_elts                = LLVMGetParam(variant_func, 10);
   system_values.draw_id     = LLVMGetParam(variant_func, 11);
   system_values.view_index  = LLVMGetParam(variant_func, 12);

   lp_build_name(context_ptr, "context");
   lp_build_name(io_ptr, "io");
   lp_build_name(vbuffers_ptr, "vbuffers");
   lp_build_name(count, "count");
   lp_build_name(start, "start");
   lp_build_name(stride, "stride");
   lp_build_name(vb_ptr, "vb");
   lp_build_name(system_values.instance_id, "instance_id");
   lp_build_name(vertex_id_offset, "vertex_id_offset");
   lp_build_name(system_values.base_instance, "start_instance");
   lp_build_name(fetch_elts, "fetch_elts");
   lp_build_name(system_values.draw_id, "draw_id");

   /*
    * Function body
    */

   block = LLVMAppendBasicBlockInContext(gallivm->context, variant_func, "entry");
   builder = gallivm->builder;
   LLVMPositionBuilderAtEnd(builder, block);

   memset(&vs_type, 0, sizeof vs_type);
   vs_type.floating = TRUE; /* floating point values */
   vs_type.sign = TRUE;     /* values are signed */
   vs_type.norm = FALSE;    /* values are not limited to [0,1] or [-1,1] */
   vs_type.width = 32;      /* 32-bit float */
   vs_type.length = vector_length;

   lp_build_context_init(&bld, gallivm, lp_type_uint(32));
   lp_build_context_init(&blduivec, gallivm, lp_uint_type(vs_type));

   /* hold temporary "bool" clipmask */
   clipmask_bool_ptr = lp_build_alloca(gallivm, blduivec.vec_type, "");

   fake_buf = lp_build_alloca_undef(gallivm,
                 LLVMVectorType(LLVMInt64TypeInContext(context), 4), "");
   fake_buf = LLVMBuildBitCast(builder, fake_buf,
                 LLVMPointerType(LLVMInt8TypeInContext(context), 0), "");
   fake_buf_ptr = LLVMBuildGEP2(builder, LLVMInt8TypeInContext(context), fake_buf, &bld.zero, 1, "");

   /* code generated texture sampling */
   sampler = draw_llvm_sampler_soa_create(draw_llvm_variant_key_samplers(key),
                                          MAX2(key->nr_samplers,
                                               key->nr_sampler_views));
   image = draw_llvm_image_soa_create(draw_llvm_variant_key_images(key),
                                      key->nr_images);

   step = lp_build_const_int32(gallivm, vector_length);

   ind_vec = blduivec.undef;
   for (i = 0; i < vs_type.length; i++) {
      LLVMValueRef index = lp_build_const_int32(gallivm, i);
      ind_vec = LLVMBuildInsertElement(builder, ind_vec, index, index, "");
   }

   have_elts = LLVMBuildICmp(builder, LLVMIntNE,
                             LLVMConstPointerNull(arg_types[10]), fetch_elts, "");

   fetch_max = LLVMBuildSub(builder, count, bld.one, "fetch_max");
   fetch_max = lp_build_broadcast_scalar(&blduivec, fetch_max);
   /*
    * Only needed for non-indexed path.
    */
   start_vec = lp_build_broadcast_scalar(&blduivec, start);

   /*
    * Pre-calculate everything which is constant per shader invocation.
    */
   for (j = 0; j < key->nr_vertex_elements; ++j) {
      LLVMValueRef vb_buffer_offset, buffer_size, temp_ptr;
      LLVMValueRef vb_info, vbuffer_ptr, buf_offset, ofbit;
      struct pipe_vertex_element *velem = &key->vertex_element[j];
      LLVMValueRef vb_index =
         lp_build_const_int32(gallivm, velem->vertex_buffer_index);
      LLVMValueRef bsize = lp_build_const_int32(gallivm,
                                                util_format_get_blocksize(velem->src_format));
      LLVMValueRef src_offset = lp_build_const_int32(gallivm,
                                                     velem->src_offset);
      struct lp_build_if_state if_ctx;

      if (velem->src_format != PIPE_FORMAT_NONE) {
         vbuffer_ptr = LLVMBuildGEP2(builder, variant->buffer_type, vbuffers_ptr, &vb_index, 1, "");
         vb_info = LLVMBuildGEP2(builder, variant->vb_type, vb_ptr, &vb_index, 1, "");
         vb_stride[j] = draw_jit_vbuffer_stride(gallivm, variant->vb_type, vb_info);
         vb_stride[j] = LLVMBuildZExt(gallivm->builder, vb_stride[j],
                                      LLVMInt32TypeInContext(context), "");
         vb_buffer_offset = draw_jit_vbuffer_offset(gallivm, variant->vb_type, vb_info);
         map_ptr[j] = draw_jit_dvbuffer_map(gallivm, variant->buffer_type, vbuffer_ptr);
         buffer_size = draw_jit_dvbuffer_size(gallivm, variant->buffer_type, vbuffer_ptr);

         ofbit = NULL;
         /*
          * We'll set buffer_size_adj to zero if we have of, so it will
          * always overflow later automatically without having to keep ofbit.
          * Overflows (with normal wraparound) doing the actual offset
          * calculation should be ok, just not for the buffer size calc.
          * It would also be possible to detect such overflows and return
          * zeros if that happens, but this would be more complex.
          */
         buf_offset = lp_build_add(&bld, vb_buffer_offset, src_offset);
         tmp = lp_build_sub(&bld, bsize, bld.one);
         buffer_size_adj[j] = lp_build_usub_overflow(gallivm, buffer_size, tmp,
                                                     &ofbit);
         buffer_size_adj[j] = lp_build_usub_overflow(gallivm, buffer_size_adj[j],
                                                     buf_offset, &ofbit);

         /*
          * We can't easily set fake vertex buffers outside the generated code.
          * Hence, set fake vertex buffers here instead basically, so fetch
          * code can always fetch using offset 0, eliminating all control flow
          * inside the main loop.
          * (Alternatively, could have control flow per vector skipping fetch
          * if ofbit is true.)
          */
         if (velem->instance_divisor) {
            /*
             * Index is equal to the start instance plus the number of current
             * instance divided by the divisor. In this case we compute it as:
             * index = start_instance + (instance_id  / divisor).
             * Note we could actually do the fetch here, outside the loop -
             * it's all constant, hopefully llvm recognizes this.
             */
            LLVMValueRef current_instance;
            current_instance = LLVMBuildUDiv(builder, system_values.instance_id,
                                             lp_build_const_int32(gallivm,
                                                                  velem->instance_divisor),
                                             "instance_divisor");
            instance_index[j] = lp_build_uadd_overflow(gallivm, system_values.base_instance,
                                                       current_instance, &ofbit);
         }

         buffer_size_adj[j] = LLVMBuildSelect(builder, ofbit, bld.zero,
                                              buffer_size_adj[j], "");

         LLVMTypeRef byte_type = LLVMInt8TypeInContext(context);
         LLVMTypeRef byte_ptr_type = LLVMPointerType(byte_type, 0);
         temp_ptr = lp_build_alloca_undef(gallivm, byte_ptr_type, "");

         lp_build_if(&if_ctx, gallivm, ofbit);
         {
            LLVMBuildStore(builder, fake_buf_ptr, temp_ptr);
         }
         lp_build_else(&if_ctx);
         {
            map_ptr[j] = LLVMBuildGEP2(builder, byte_type, map_ptr[j], &buf_offset, 1, "");
            LLVMBuildStore(builder, map_ptr[j], temp_ptr);
         }
         lp_build_endif(&if_ctx);
         map_ptr[j] = LLVMBuildLoad2(builder, byte_ptr_type, temp_ptr, "map_ptr");

         if (0) {
            lp_build_printf(gallivm, "velem %d, vbuf index = %u, vb_stride = %u\n",
                            lp_build_const_int32(gallivm, j),
                            vb_index, vb_stride[j]);
            lp_build_printf(gallivm,
                            "   vb_buffer_offset = %u, src_offset = %u, buf_offset = %u\n",
                            vb_buffer_offset, src_offset, buf_offset);
            lp_build_printf(gallivm, "   buffer size = %u, blocksize = %u\n",
                            buffer_size, bsize);
            lp_build_printf(gallivm, "   instance_id = %u\n", system_values.instance_id);
         }
      }
   }

   lp_build_loop_begin(&lp_loop, gallivm, bld.zero);
   {
      LLVMValueRef inputs[PIPE_MAX_SHADER_INPUTS][TGSI_NUM_CHANNELS];
      LLVMValueRef io;
      LLVMValueRef clipmask;   /* holds the clipmask value */
      LLVMValueRef true_index_array, index_store;
      const LLVMValueRef (*ptr_aos)[TGSI_NUM_CHANNELS];

      io_itr = lp_loop.counter;

      io = LLVMBuildGEP2(builder, variant->vertex_header_type, io_ptr, &io_itr, 1, "");
#if DEBUG_STORE
      lp_build_printf(gallivm, " --- io %d = %p, loop counter %d\n",
                      io_itr, io, lp_loop.counter);
#endif

      true_index_array = lp_build_broadcast_scalar(&blduivec, lp_loop.counter);
      true_index_array = LLVMBuildAdd(builder, true_index_array, ind_vec, "");

      LLVMValueRef exec_mask = lp_build_cmp(&blduivec, PIPE_FUNC_LEQUAL, true_index_array, fetch_max);
      /*
       * Limit indices to fetch_max, otherwise might try to access indices
       * beyond index buffer (or rather vsplit elt buffer) size.
       * Could probably safely (?) skip this for non-indexed draws and
       * simplify things minimally (by removing it could combine the ind_vec
       * and start_vec adds). I think the only effect for non-indexed draws will
       * be that for the invalid elements they will be all fetched from the
       * same location as the last valid one, but noone should really care.
       */
      true_index_array = lp_build_min(&blduivec, true_index_array, fetch_max);

      index_store = lp_build_alloca_undef(gallivm, blduivec.vec_type, "index_store");

      lp_build_if(&if_ctx, gallivm, have_elts);
      {
         /*
          * Note: you'd expect some comparison/clamp against fetch_elt_max
          * here.
          * There used to be one here but it was incorrect: overflow was
          * detected if index > fetch_elt_max - but the correct condition
          * would be index >= fetch_elt_max (since this is just size of elts
          * buffer / element size).
          * Using the correct condition however will cause failures - due to
          * vsplit/vcache code which rebases indices. So, as an example, if
          * fetch_elt_max is just 1 and fetch_count 2, vsplit cache will
          * replace all invalid indices with 0 - which in case of elt_bias
          * not being zero will get a different fetch index than the valid
          * index 0. So, just rely on vsplit code preventing out-of-bounds
          * fetches. This is also why it's safe to do elts fetch even if there
          * was no index buffer bound - the real buffer is never seen here, at
          * least not if there are index buffer overflows...
          */

         /*
          * XXX should not have to do this, as scale can be handled
          * natively by loads (hits asserts though).
          */
         tmp = lp_build_shl_imm(&blduivec, true_index_array, 2);
         fetch_elts = LLVMBuildBitCast(builder, fetch_elts,
                                       LLVMPointerType(LLVMInt8TypeInContext(context),
                                                       0), "");
         tmp = lp_build_gather(gallivm, vs_type.length,
                               32, bld.type, TRUE,
                               fetch_elts, tmp, FALSE);
         LLVMBuildStore(builder, tmp, index_store);
      }
      lp_build_else(&if_ctx);
      {
         tmp = LLVMBuildAdd(builder, true_index_array, start_vec, "");
         LLVMBuildStore(builder, tmp, index_store);
      }
      lp_build_endif(&if_ctx);

      true_index_array = LLVMBuildLoad2(builder, blduivec.vec_type, index_store, "");

      for (j = 0; j < key->nr_vertex_elements; ++j) {
         struct pipe_vertex_element *velem = &key->vertex_element[j];
         const struct util_format_description *format_desc =
            util_format_description(velem->src_format);

         if (format_desc->format == PIPE_FORMAT_NONE) {
            for (i = 0; i < TGSI_NUM_CHANNELS; i++) {
               inputs[j][i] = lp_build_zero(gallivm, vs_type);
            }
         } else if (velem->instance_divisor) {
            fetch_instanced(gallivm, format_desc, vs_type,
                            vb_stride[j], map_ptr[j],
                            buffer_size_adj[j],
                            inputs[j], instance_index[j]);
         } else {
            fetch_vector(gallivm, format_desc, vs_type,
                         vb_stride[j], map_ptr[j],
                         buffer_size_adj[j],
                         inputs[j], true_index_array);
         }
      }

      struct lp_build_mask_context mask;

      lp_build_mask_begin(&mask, gallivm, vs_type, exec_mask);
      /* In the paths with elts vertex id has to be unaffected by the
       * index bias and because indices inside our elements array have
       * already had index bias applied we need to subtract it here to
       * get back to the original index.
       * In the linear paths vertex id has to be unaffected by the
       * original start index and because we abuse the 'start' variable
       * to either represent the actual start index or the index at which
       * the primitive was split (we split rendering into chunks of at
       * most 4095-vertices) we need to back out the original start
       * index out of our vertex id here.
       * for ARB_shader_draw_parameters, base_vertex should be 0 for
       * non-indexed draws.
       */
      LLVMValueRef base_vertex = lp_build_select(&bld, have_elts, vertex_id_offset, lp_build_const_int32(gallivm, 0));
      system_values.basevertex = lp_build_broadcast_scalar(&blduivec, base_vertex);

      /* first vertex is for Vulkan base vertex support */
      LLVMValueRef first_vertex = vertex_id_offset;
      system_values.firstvertex = lp_build_broadcast_scalar(&blduivec, first_vertex);

      system_values.vertex_id = true_index_array;
      system_values.vertex_id_nobase = LLVMBuildSub(builder, true_index_array,
                                                    lp_build_broadcast_scalar(&blduivec, vertex_id_offset), "");

      ptr_aos = (const LLVMValueRef (*)[TGSI_NUM_CHANNELS]) inputs;
      generate_vs(variant,
                  builder,
                  vs_type,
                  outputs,
                  ptr_aos,
                  &system_values,
                  context_ptr,
                  sampler,
                  image,
                  key->clamp_vertex_color,
                  &mask);

      lp_build_mask_end(&mask);
      if (pos != -1 && cv != -1) {
         /* store original positions in clip before further manipulation */
         store_clip(gallivm, vs_type, variant->vertex_header_type, io, outputs, pos);

         /* do cliptest */
         if (enable_cliptest) {
            LLVMValueRef temp = LLVMBuildLoad2(builder, blduivec.vec_type, clipmask_bool_ptr, "");
            /* allocate clipmask, assign it integer type */
            clipmask = generate_clipmask(llvm,
                                         gallivm,
                                         vs_type,
                                         outputs,
                                         key,
                                         variant->context_type,
                                         context_ptr, &have_clipdist);
            temp = LLVMBuildOr(builder, clipmask, temp, "");
            /* store temporary clipping boolean value */
            LLVMBuildStore(builder, temp, clipmask_bool_ptr);
         } else {
            clipmask = blduivec.zero;
         }

         /* do viewport mapping */
         if (!bypass_viewport) {
            generate_viewport(variant, builder, vs_type, outputs, context_ptr);
         }
      } else {
         clipmask = blduivec.zero;
      }

      /* store clipmask in vertex header,
       * original positions in clip
       * and transformed positions in data
       */
      convert_to_aos(gallivm, variant->vertex_header_type, io, NULL, outputs, clipmask,
                     vs_info->num_outputs, vs_type, -1,
                     enable_cliptest && key->need_edgeflags);
   }
   lp_build_loop_end_cond(&lp_loop, count, step, LLVMIntUGE);

   draw_llvm_sampler_soa_destroy(sampler);
   draw_llvm_image_soa_destroy(image);

   /* return clipping boolean value for function */
   ret = clipmask_booli8(gallivm, vs_type, blduivec.vec_type, clipmask_bool_ptr,
                         enable_cliptest && key->need_edgeflags);

   LLVMBuildRet(builder, ret);

   gallivm_verify_function(gallivm, variant_func);
}

```



# varinat的类型结构体创建

```
/**
 * Create LLVM types for various structures.
 */
static void
create_jit_types(struct draw_llvm_variant *variant)
{
   struct gallivm_state *gallivm = variant->gallivm;

   variant->context_type = create_jit_context_type(gallivm, "draw_jit_context");
   variant->context_ptr_type = LLVMPointerType(variant->context_type, 0);

   variant->buffer_type = create_jit_dvbuffer_type(gallivm, "draw_vertex_buffer");
   variant->buffer_ptr_type = LLVMPointerType(variant->buffer_type, 0);

   variant->vb_type = create_jit_vertex_buffer_type(gallivm, "pipe_vertex_buffer");
   variant->vb_ptr_type = LLVMPointerType(variant->vb_type, 0);
}

```

# variant->vb_type 

pipe_vertex_buffer

# variant- vertex_header

```
/**
 * Basic vertex info.  Used to represent vertices after VS (through GS, TESS,
 * etc.) to vbuf output.
 */
struct vertex_header {
   unsigned clipmask:DRAW_TOTAL_CLIP_PLANES;
   unsigned edgeflag:1;
   unsigned pad:1;
   unsigned vertex_id:16;

   float clip_pos[4];
   float data[][4]; // the vertex attributes
};


```



# LLVM C API
## LLVMAddGlobalMapping

`LLVMAddGlobalMapping` 函数用于在 LLVM 执行引擎中映射全局变量或函数到指定的地址。这个函数通常在 JIT（Just-In-Time）编译的场景中使用，它允许你将全局变量或函数映射到预定义的内存地址，以便在运行时能够直接访问这些变量或函数。

函数原型通常如下：

```c
void LLVMAddGlobalMapping(LLVMExecutionEngineRef EE, LLVMValueRef Global, void* Addr);
```

- `LLVMExecutionEngineRef EE`: 表示 LLVM 执行引擎的引用，该引擎用于执行 LLVM IR 代码。

- `LLVMValueRef Global`: 表示全局变量或函数的 LLVM 值的引用，即对应于全局变量或函数的 LLVM IR 表示。

- `void* Addr`: 表示你想要映射到的地址。

通过调用 `LLVMAddGlobalMapping`，你可以将全局变量或函数映射到一个特定的内存地址，而不是让 LLVM 执行引擎自行决定它们的位置。这样做的一个常见应用场景是在 JIT 编译后，将全局变量或函数的地址传递给外部代码（例如，通过 C 函数指针），使外部代码能够直接访问这些 LLVM IR 元素。

以下是一个简单的示例：

```c
#include <llvm-c/ExecutionEngine.h>

// ...

// 在 JIT 编译后，将全局变量映射到地址
LLVMAddGlobalMapping(executionEngine, globalVar, (void*)myGlobalVarAddress);

// 在 JIT 编译后，将函数映射到地址
LLVMAddGlobalMapping(executionEngine, myFunction, (void*)myFunctionAddress);
```

这样一来，你可以通过 `myGlobalVarAddress` 直接访问全局变量，通过 `myFunctionAddress` 直接调用函数。

##  LLVMBuildZExt


`LLVMBuildZExt` 是 LLVM IR 构建 API 中的一个函数，用于创建 `zext`（zero extend）指令。该指令用于将整数值从一个较小的位宽零扩展到较大的位宽。

函数原型通常如下：

```c
LLVMValueRef LLVMBuildZExt(LLVMBuilderRef B, LLVMValueRef Val, LLVMTypeRef DestTy, const char *Name);
```

- `LLVMBuilderRef B`: 表示 LLVM IR 构建器的引用，用于构建 LLVM 指令序列。

- `LLVMValueRef Val`: 表示要进行零扩展的整数值的 LLVM 值。

- `LLVMTypeRef DestTy`: 表示零扩展后的目标类型，即较大的位宽。

- `const char *Name`: 表示生成的指令的名称。

该函数返回一个 `LLVMValueRef`，即新创建的 `zext` 指令的 LLVM 值。

以下是一个简单的示例，说明如何使用 `LLVMBuildZExt`：

```c
#include <llvm-c/Core.h>

// ...

// 创建 LLVM 上下文和构建器
LLVMContextRef context = LLVMContextCreate();
LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);

// 定义一个 i8 类型的值
LLVMTypeRef int8Type = LLVMInt8TypeInContext(context);
LLVMValueRef originalValue = /* some i8 value */;

// 进行零扩展，将 i8 值扩展为 i32
LLVMTypeRef int32Type = LLVMInt32TypeInContext(context);
LLVMValueRef zextResult = LLVMBuildZExt(builder, originalValue, int32Type, "zext_result");

// 在这里可以继续构建其他指令

// 释放资源
LLVMDisposeBuilder(builder);
LLVMContextDispose(context);
```

上述代码中，`LLVMBuildZExt` 将 `originalValue` 进行零扩展，从 `i8` 类型扩展为 `i32` 类型。你可以根据实际情况调整源类型和目标类型。


## LLVMBuildSelect


`LLVMBuildSelect` 是 LLVM IR 构建 API 中的一个函数，用于创建 `select` 指令，也称为三元选择指令。该指令根据条件选择两个值中的一个，并返回选定的值。

函数原型通常如下：

```c
LLVMValueRef LLVMBuildSelect(LLVMBuilderRef B, LLVMValueRef If, LLVMValueRef Then, LLVMValueRef Else, const char *Name);
```

- `LLVMBuilderRef B`: 表示 LLVM IR 构建器的引用，用于构建 LLVM 指令序列。

- `LLVMValueRef If`: 表示条件表达式的 LLVM 值。如果该值为 true，则选择 `Then`；否则选择 `Else`。

- `LLVMValueRef Then`: 表示条件为 true 时的值。

- `LLVMValueRef Else`: 表示条件为 false 时的值。

- `const char *Name`: 表示生成的 `select` 指令的名称。

该函数返回一个 `LLVMValueRef`，即新创建的 `select` 指令的 LLVM 值。

以下是一个简单的示例，说明如何使用 `LLVMBuildSelect`：

```c
#include <llvm-c/Core.h>

// ...

// 创建 LLVM 上下文和构建器
LLVMContextRef context = LLVMContextCreate();
LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);

// 定义条件、Then 值和 Else 值
LLVMValueRef condition = /* some condition */;
LLVMValueRef thenValue = /* value if true */;
LLVMValueRef elseValue = /* value if false */;

// 使用 LLVMBuildSelect 创建 select 指令
LLVMValueRef selectResult = LLVMBuildSelect(builder, condition, thenValue, elseValue, "select_result");

// 在这里可以继续构建其他指令

// 释放资源
LLVMDisposeBuilder(builder);
LLVMContextDispose(context);
```

上述代码中，`LLVMBuildSelect` 根据 `condition` 的值选择 `thenValue` 或 `elseValue`，并将结果存储在 `selectResult` 中。你可以根据实际需求调整条件和值。



## LLVMBuildStore

`LLVMBuildStore` 是 LLVM IR 构建 API 中的一个函数，用于创建 `store` 指令，用于将一个值存储到内存中。

函数原型通常如下：

```c
LLVMValueRef LLVMBuildStore(LLVMBuilderRef B, LLVMValueRef Val, LLVMValueRef Ptr);
```

- `LLVMBuilderRef B`: 表示 LLVM IR 构建器的引用，用于构建 LLVM 指令序列。

- `LLVMValueRef Val`: 表示要存储的值的 LLVM 值。

- `LLVMValueRef Ptr`: 表示存储位置的指针（指向内存的指针）的 LLVM 值。

该函数返回一个 `LLVMValueRef`，通常对于 `store` 指令而言，返回的值没有特别的含义。

以下是一个简单的示例，说明如何使用 `LLVMBuildStore`：

```c
#include <llvm-c/Core.h>

// ...

// 创建 LLVM 上下文和构建器
LLVMContextRef context = LLVMContextCreate();
LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);

// 定义要存储的值和存储位置的指针
LLVMValueRef valueToStore = /* some value */;
LLVMValueRef storagePointer = /* pointer to the memory location where the value should be stored */;

// 使用 LLVMBuildStore 创建 store 指令
LLVMBuildStore(builder, valueToStore, storagePointer);

// 在这里可以继续构建其他指令

// 释放资源
LLVMDisposeBuilder(builder);
LLVMContextDispose(context);
```

上述代码中，`LLVMBuildStore` 用于创建一个 `store` 指令，将 `valueToStore` 存储到 `storagePointer` 指向的内存位置。这个指令用于在 LLVM IR 中表示内存写入操作。



 
## LLVMBuildSIToFP

`LLVMBuildSIToFP` 是 LLVM 中的一个函数，用于构建从整数类型到浮点类型的转换指令。在 LLVM IR 中，这个函数的原型通常是：

```c
LLVMValueRef LLVMBuildSIToFP(LLVMBuilderRef, LLVMValueRef IntVal, LLVMTypeRef FloatTy, const char *Name);
```

这个函数接受以下参数：

- `LLVMBuilderRef`：LLVM 指令构建器的引用，用于构建指令。
- `LLVMValueRef IntVal`：整数类型的 LLVM 值，需要进行从整数到浮点的转换。
- `LLVMTypeRef FloatTy`：目标浮点类型的 LLVM 类型。
- `const char *Name`：指令的名称。

函数的作用是创建一个 LLVM 指令，将整数值 `IntVal` 转换为浮点类型，结果的类型为 `FloatTy`。这种指令在编译器优化和代码生成过程中经常用于处理不同类型之间的转换。


##  LLVMSetInitializer

`LLVMSetInitializer` 是 LLVM 中的一个函数，用于设置全局变量的初始值。LLVM（Low Level Virtual Machine）是一个开源的编译器基础设施，用于优化和生成目标代码。以下是关于 `LLVMSetInitializer` 的一些基本信息和用法示例。

函数原型：

```c
void LLVMSetInitializer(LLVMValueRef GlobalVar, LLVMValueRef ConstantVal);
```

参数说明：
- `GlobalVar`：LLVMValueRef 类型，表示全局变量的引用。
- `ConstantVal`：LLVMValueRef 类型，表示用于初始化全局变量的常量值。

使用示例：

```c
#include <llvm-c/Core.h>

int main() {
    // 初始化 LLVM 上下文
    LLVMContextRef context = LLVMContextCreate();

    // 创建一个 LLVM 模块
    LLVMModuleRef module = LLVMModuleCreateWithName("MyModule");

    // 定义一个全局变量
    LLVMTypeRef intType = LLVMInt32TypeInContext(context);
    LLVMValueRef globalVar = LLVMAddGlobal(module, intType, "myGlobalVar");

    // 创建一个常量值（32 位整数类型，初始值为 42）
    LLVMValueRef constantValue = LLVMConstInt(intType, 42, 0);

    // 使用 LLVMSetInitializer 设置全局变量的初始值
    LLVMSetInitializer(globalVar, constantValue);

    // 在这里可以继续添加函数、基本块等

    // 打印 LLVM 模块内容
    LLVMDumpModule(module);

    // 释放资源
    LLVMDisposeModule(module);
    LLVMContextDispose(context);

    return 0;
}
```

上述示例创建了一个 LLVM 模块，定义了一个 32 位整数类型的全局变量 `myGlobalVar`，并使用 `LLVMSetInitializer` 将其初始值设置为 42。注意，这只是一个简单的示例，实际应用中可能涉及更复杂的操作和结构。在使用 LLVM 时，通常需要更多的代码来构建函数、基本块、指令等。

## LLVMBuildArrayAlloca


`LLVMBuildArrayAlloca` 函数是 LLVM IR 中的一个构建指令（Instruction）的函数，用于在函数中分配一个数组的内存空间。这个函数会返回一个 LLVM 值，该值代表分配得到的数组的起始地址。

函数原型如下：

```c
LLVMValueRef LLVMBuildArrayAlloca(LLVMBuilderRef Builder, LLVMTypeRef Ty, LLVMValueRef Val, const char *Name);
```

参数说明：
- `Builder`：LLVMBuilderRef 类型，表示 LLVM IR 构建器。
- `Ty`：LLVMTypeRef 类型，表示数组元素的类型。
- `Val`：LLVMValueRef 类型，表示数组的元素数量。这可以是一个整数值的常量或者是一个 LLVM 值。
- `Name`：分配的数组的名称，可以为 `NULL`。

使用示例：

```c
#include <llvm-c/Core.h>
#include <llvm-c/Target/TargetMachine.h>

int main() {
    // 初始化 LLVM 上下文
    LLVMContextRef context = LLVMContextCreate();

    // 创建一个 LLVM 模块
    LLVMModuleRef module = LLVMModuleCreateWithName("MyModule");

    // 在模块中创建一个函数
    LLVMTypeRef functionReturnType = LLVMVoidTypeInContext(context);
    LLVMTypeRef functionParamTypes[] = { LLVMInt32TypeInContext(context) };
    LLVMTypeRef functionType = LLVMFunctionType(functionReturnType, functionParamTypes, 1, 0);
    LLVMValueRef myFunction = LLVMAddFunction(module, "myFunction", functionType);

    // 创建基本块和构建器
    LLVMBasicBlockRef entryBlock = LLVMAppendBasicBlockInContext(context, myFunction, "entry");
    LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);
    LLVMPositionBuilderAtEnd(builder, entryBlock);

    // 使用 LLVMBuildArrayAlloca 创建一个数组的内存空间
    LLVMTypeRef arrayElementType = LLVMInt32TypeInContext(context);
    LLVMValueRef arraySize = LLVMConstInt(LLVMInt32TypeInContext(context), 10, 0); // 数组大小为 10
    LLVMValueRef arrayAlloca = LLVMBuildArrayAlloca(builder, arrayElementType, arraySize, "myArray");

    // 在这里可以继续添加其他指令

    // 打印 LLVM 模块内容
    LLVMDumpModule(module);

    // 释放资源
    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(module);
    LLVMContextDispose(context);

    return 0;
}
```

在上述示例中，`LLVMBuildArrayAlloca` 被用来在函数的入口块中创建一个大小为 10 的整数数组的内存空间。这只是一个简单的用法示例，实际应用中可能需要更多的指令来完善函数的实现。


## LLVMBuildTrunc


`LLVMBuildTrunc` 是 LLVM IR 中的一个构建指令（Instruction）的函数，用于创建一个整数截断指令。这个指令将给定的整数值截断为更小位宽的整数值。

函数原型如下：

```c
LLVMValueRef LLVMBuildTrunc(LLVMBuilderRef B, LLVMValueRef Val, LLVMTypeRef DestTy, const char *Name);
```

参数说明：
- `B`：LLVMBuilderRef 类型，表示 LLVM IR 构建器。
- `Val`：LLVMValueRef 类型，表示要截断的整数值。
- `DestTy`：LLVMTypeRef 类型，表示目标整数类型，即截断后的类型。
- `Name`：指令的名称，可以为 `NULL`。

使用示例：

```c
#include <llvm-c/Core.h>

int main() {
    // 初始化 LLVM 上下文
    LLVMContextRef context = LLVMContextCreate();

    // 创建一个 LLVM 模块
    LLVMModuleRef module = LLVMModuleCreateWithName("MyModule");

    // 创建一个 LLVM 函数
    LLVMTypeRef returnType = LLVMVoidTypeInContext(context);
    LLVMTypeRef paramType = LLVMInt32TypeInContext(context);
    LLVMTypeRef functionType = LLVMFunctionType(returnType, &paramType, 1, 0);
    LLVMValueRef myFunction = LLVMAddFunction(module, "myFunction", functionType);

    // 创建基本块和构建器
    LLVMBasicBlockRef entryBlock = LLVMAppendBasicBlockInContext(context, myFunction, "entry");
    LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);
    LLVMPositionBuilderAtEnd(builder, entryBlock);

    // 创建一个整数值
    LLVMValueRef intValue = LLVMConstInt(LLVMInt64TypeInContext(context), 123456789, 0);

    // 使用 LLVMBuildTrunc 创建截断指令
    LLVMValueRef truncValue = LLVMBuildTrunc(builder, intValue, LLVMInt32TypeInContext(context), "truncResult");

    // 在这里可以继续添加其他指令

    // 打印 LLVM 模块内容
    LLVMDumpModule(module);

    // 释放资源
    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(module);
    LLVMContextDispose(context);

    return 0;
}
```

上述示例创建了一个 LLVM 模块和一个函数，在函数中使用 `LLVMBuildTrunc` 指令将一个 64 位整数值截断为 32 位整数值。在实际应用中，截断操作通常涉及到位宽的调整，确保截断后的值能够适应目标类型。

## mem2reg scalarrepl


这段注释解释了在 LLVM 中进行数组（或标量/向量）的内存分配，并提到了一些与内存分配相关的优化 passes（如 mem2reg 和 scalarrepl）。

1. **mem2reg Pass：** 该 Pass 负责将内存中的值提升到寄存器中，但它并不支持将结构体或数组提升到寄存器。注释指出，即使不能通过 mem2reg 提升数组，仍然需要在第一个基本块中放置 allocas（alloca 是 LLVM 中用于分配栈上内存的指令），因为不这样做可能会导致 X86 后端无法成功地对齐栈。

2. **scalarrepl Pass：** 这个 Pass 被称为“scalar replacement”，它据称在许多情况下可以推广数组。这意味着 scalarrepl Pass 更加强大，能够处理某些情况下的数组优化。

3. **参考链接：** 注释还提供了一个参考链接，指向 LLVM 文档的一个部分，可能包含有关内存分配和相关优化的更多信息。

总体来说，这段注释简要说明了数组内存分配的一些问题和优化。如果你想深入了解有关 mem2reg 和 scalarrepl Pass 的更多信息，可以参考相应的 LLVM 文档和参考链接。\



#  draw LLVM IR

```
i = 0;
   arg_types[i++] = get_context_ptr_type(variant);       /* context */
   arg_types[i++] = get_vertex_header_ptr_type(variant); /* vertex_header */
   arg_types[i++] = get_buffer_ptr_type(variant);        /* vbuffers */
3   arg_types[i++] = int32_type;                          /* count */
   arg_types[i++] = int32_type;                          /* start/fetch_elt_max */
   arg_types[i++] = int32_type;                          /* stride */
6   arg_types[i++] = get_vb_ptr_type(variant);            /* pipe_vertex_buffer's */
   arg_types[i++] = int32_type;                          /* instance_id */
   arg_types[i++] = int32_type;                          /* vertex_id_offset */
9   arg_types[i++] = int32_type;                          /* start_instance */
   arg_types[i++] = LLVMPointerType(int32_type, 0);      /* fetch_elts  */
   arg_types[i++] = int32_type;                          /* draw_id */
   arg_types[i++] = int32_type;                          /* view_id */

```

#  SoA 和AoS布局


"SoA" 是 "Structure of Arrays" 的缩写，表示数组的结构。相对于 "AoSoA"（Array of Structures of Arrays）或 "AoS"（Array of Structures）布局，SoA 布局将数据按照每个成员单独存储，而不是按照结构体的实例存储。

在计算机图形学和图像处理领域，SoA 布局通常用于描述和存储向量化数据，例如在图像处理中，存储颜色通道的数据。

具体到你提到的情境，"Fetch a texels from a texture, returning them in SoA layout" 表示从纹理中获取纹素（texel），并以 SoA 布局返回它们。这可能涉及从纹理中读取多个通道的数据，将每个通道的数据分别存储，而不是将整个纹素作为结构体存储。这样的布局可能更适合某些计算模型或硬件架构，特别是对于向量化的处理。

SoA 布局的一个简单示例是将 RGB 颜色信息分别存储在三个不同的数组中，而不是将整个颜色结构体存储在一个数组中。这有助于并行处理，尤其是在 SIMD（Single Instruction, Multiple Data）指令集上执行向量化操作。

让我们以一个简单的图像处理任务为例，说明 SoA 布局的实际应用。假设我们有一个二维图像，每个像素包含红、绿和蓝三个通道。在 SoA 布局中，我们将每个颜色通道分别存储在不同的数组中。

假设我们有一个像素矩阵，它是一个结构体数组（AoS 布局）：

```c
struct Pixel {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

struct Pixel image[WIDTH][HEIGHT];
```

在 SoA 布局中，我们将颜色通道分别存储在不同的数组中：

```c
uint8_t redChannel[WIDTH][HEIGHT];
uint8_t greenChannel[WIDTH][HEIGHT];
uint8_t blueChannel[WIDTH][HEIGHT];
```

在实际图像处理中，我们可能需要执行一些基于通道的操作，比如增加亮度，对比度调整等。使用 SoA 布局，我们可以更容易地实现向量化操作。例如，如果我们想要增加所有像素的红色通道值，我们可以执行以下操作：

```c
for (int i = 0; i < WIDTH; ++i) {
    for (int j = 0; j < HEIGHT; ++j) {
        redChannel[i][j] += 10;  // 增加红色通道亮度
    }
}
```

这样的布局有助于并行处理，特别是在 SIMD 指令集上，因为我们可以轻松地对每个颜色通道执行相同的操作。

需要注意的是，SoA 布局并非在所有场景下都是最优选择，它取决于具体的应用和处理需求。在某些情况下，AoS 布局可能更自然，而在其他情况下，SoA 布局可能更适用。




# SSE AVX


SSE（Streaming SIMD Extensions）和 AVX（Advanced Vector Extensions）都是英特尔（Intel）和 AMD（Advanced Micro Devices）为 x86 架构引入的 SIMD（Single Instruction, Multiple Data）指令集扩展。它们都旨在提高浮点运算性能，通过在单个指令中同时处理多个数据元素。

以下是 SSE 和 AVX 的主要区别：

1. **寄存器宽度：**
   - **SSE：** SSE 提供 XMM 寄存器，每个寄存器宽度为128位。XMM 寄存器可以容纳四个单精度浮点数或四个整数。
   - **AVX：** AVX 引入了 YMM 寄存器，每个寄存器宽度为256位。YMM 寄存器可以容纳八个单精度浮点数或八个整数。AVX2 进一步引入了 ZMM 寄存器，宽度为512位。

2. **指令宽度：**
   - **SSE：** SSE 提供一些基本的浮点指令，如加法、乘法、逻辑运算等，以及一些特殊的指令，如逆数、平方根等。
   - **AVX：** AVX 扩展了指令集，引入了更多的高级浮点指令，例如 FMA（fused multiply-add）、交错加载和存储等。这些指令可以在更宽的寄存器上执行，从而提高并行性。

3. **向量长度：**
   - **SSE：** SSE 提供单精度浮点数的向量化操作，每个指令处理四个单精度浮点数。
   - **AVX：** AVX 提供更宽的向量寄存器，每个指令可以处理八个单精度浮点数或四个双精度浮点数。AVX2 进一步扩展了向量长度。

4. **兼容性：**
   - **SSE：** SSE 是较早引入的 SIMD 扩展，几乎所有支持 x86 架构的处理器都支持 SSE。
   - **AVX：** AVX 是 SSE 的后续版本，要求较新的处理器支持。AVX2 则需要更高级别的支持。

5. **性能：**
   - **AVX：** 由于 AVX 提供更宽的寄存器和更多的高级指令，它通常在对浮点性能有更高要求的应用中表现更好。 FMA 指令在 AVX 中是一个显著的性能优势。

总体而言，AVX 是 SSE 的进化版本，提供了更多的功能和更大的向量寄存器，使得它在更广泛的应用中能够发挥更大的优势。但是，具体的选择取决于处理器支持和应用程序的要求。在支持的情况下，AVX 通常是更好的选择，因为它能够提供更高的性能。



##  Code generate the who



### The shader JIT function operates on blocks of quads.
Each block has 2x2 quads and 


# JIT编译器代码的产生



```
/**
 * Generate a new fragment shader variant from the shader code and
 * other state indicated by the key.
 */
static struct lp_fragment_shader_variant *
generate_variant(struct llvmpipe_context *lp,
                 struct lp_fragment_shader *shader,
                 const struct lp_fragment_shader_variant_key *key)
{
   struct lp_fragment_shader_variant *variant =
      MALLOC(sizeof *variant + shader->variant_key_size - sizeof variant->key);

   memcpy(&variant->key, key, shader->variant_key_size);

   struct llvmpipe_screen *screen = llvmpipe_screen(lp->pipe.screen);

   char module_name[64];
   snprintf(module_name, sizeof(module_name), "fs%u_variant%u",
            shader->no, shader->variants_created);

    // 创建gallivm, lp->context是在llvmpipe_create_context创建时创建
    // 这一步没有创建gallivm.codek,target
    // 在之后的compile_module时创建
   variant->gallivm = gallivm_create(module_name, lp->context, &cached);
           gallivm = CALLOC_STRUCT(gallivm_state);
           if (gallivm) {
                if (!init_gallivm_state(gallivm, name, context, cache))  \
                    gallivm->context = context;
                    gallivm->cache = cache;
                    gallivm->module = LLVMModuleCreateWithNameInContext(name,
                    gallivm->builder = LLVMCreateBuilderInContext(gallivm->context);
                    gallivm->memorymgr = lp_get_default_memory_manager();
                    gallivm->passmgr = LLVMCreateFunctionPassManagerForModule(gallivm->module);
                    LLVMAddFunctionAttrsPass(gallivm->cgpassmgr);
                    gallivm->coro_malloc_hook_type = malloc_type;
                    gallivm->coro_malloc_hook = LLVMAddFunction(gallivm->module, "coro_malloc", malloc_type);
           }


   variant->list_item_global.base = variant;
   variant->list_item_local.base = variant;
   variant->no = shader->variants_created++;

   /* The scissor is ignored here as only tiles inside the scissoring
    * rectangle will refer to this.
    */
   const boolean no_kill =
         fullcolormask &&
         !key->stencil[0].enabled &&
         !key->alpha.enabled &&
         !key->multisample &&
         !key->blend.alpha_to_coverage &&
         !key->depth.enabled &&
         !shader->info.base.uses_kill &&
         !shader->info.base.writes_samplemask &&
         !shader->info.base.uses_fbfetch;

   variant->opaque =
         no_kill &&
         !key->blend.logicop_enable &&
         !key->blend.rt[0].blend_enable
         ? TRUE : FALSE;

   variant->potentially_opaque =
         no_kill &&
         !key->blend.logicop_enable &&
         key->blend.rt[0].blend_enable &&
         key->blend.rt[0].rgb_func == PIPE_BLEND_ADD &&
         key->blend.rt[0].rgb_dst_factor == PIPE_BLENDFACTOR_INV_SRC_ALPHA &&
         key->blend.rt[0].alpha_func == key->blend.rt[0].rgb_func &&
         key->blend.rt[0].alpha_dst_factor == key->blend.rt[0].rgb_dst_factor &&
         shader->base.type == PIPE_SHADER_IR_TGSI &&
         /*
          * FIXME: for NIR, all of the fields of info.xxx (except info.base)
          * are zeros, hence shader analysis (here and elsewhere) using these
          * bits cannot work and will silently fail (cbuf is the only pointer
          * field, hence causing a crash).
          */
         shader->info.cbuf[0][3].file != TGSI_FILE_NULL
         ? TRUE : FALSE;

   /* We only care about opaque blits for now */
   if (variant->opaque &&
       (shader->kind == LP_FS_KIND_BLIT_RGBA ||
        shader->kind == LP_FS_KIND_BLIT_RGB1)) {
      const struct lp_sampler_static_state *samp0 =
         lp_fs_variant_key_sampler_idx(key, 0);
      assert(samp0);

      const enum pipe_format texture_format = samp0->texture_state.format;
      const enum pipe_texture_target target = samp0->texture_state.target;
      const unsigned min_img_filter = samp0->sampler_state.min_img_filter;
      const unsigned mag_img_filter = samp0->sampler_state.mag_img_filter;

      unsigned min_mip_filter;
      if (samp0->texture_state.level_zero_only) {
         min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
      } else {
         min_mip_filter = samp0->sampler_state.min_mip_filter;
      }

      if (target == PIPE_TEXTURE_2D &&
          min_img_filter == PIPE_TEX_FILTER_NEAREST &&
          mag_img_filter == PIPE_TEX_FILTER_NEAREST &&
          min_mip_filter == PIPE_TEX_MIPFILTER_NONE &&
          ((texture_format &&
            util_is_format_compatible(util_format_description(texture_format),
                                      cbuf0_format_desc)) ||
           (shader->kind == LP_FS_KIND_BLIT_RGB1 &&
            (texture_format == PIPE_FORMAT_B8G8R8A8_UNORM ||
             texture_format == PIPE_FORMAT_B8G8R8X8_UNORM) &&
            (key->cbuf_format[0] == PIPE_FORMAT_B8G8R8A8_UNORM ||
             key->cbuf_format[0] == PIPE_FORMAT_B8G8R8X8_UNORM)))) {
         variant->blit = 1;
      }
   }

   memcpy(&variant->key, key, sizeof *key);

   llvmpipe_fs_variant_fastpath(variant);

   lp_jit_init_types(variant);

   if (variant->jit_function[RAST_EDGE_TEST] == NULL)
      generate_fragment(lp, shader, variant, RAST_EDGE_TEST);

   /*
    * Compile everything
    */

   gallivm_compile_module(variant->gallivm);

   variant->nr_instrs += lp_build_count_ir_module(variant->gallivm->module);

   if (variant->function[RAST_EDGE_TEST]) {
      variant->jit_function[RAST_EDGE_TEST] = (lp_jit_frag_func)
            gallivm_jit_function(variant->gallivm,
                                 variant->function[RAST_EDGE_TEST]);
   }

   if (variant->function[RAST_WHOLE]) {
      variant->jit_function[RAST_WHOLE] = (lp_jit_frag_func)
         gallivm_jit_function(variant->gallivm,
                              variant->function[RAST_WHOLE]);
   } else if (!variant->jit_function[RAST_WHOLE]) {
      variant->jit_function[RAST_WHOLE] = (lp_jit_frag_func)
         variant->jit_function[RAST_EDGE_TEST];
   }

   if (linear_pipeline) {
      if (variant->linear_function) {
         variant->jit_linear_llvm = (lp_jit_linear_llvm_func)
            gallivm_jit_function(variant->gallivm, variant->linear_function);
      }
/* * This must be done after LLVM compilation, as it will call the JIT'ed * code to determine active inputs.
       */
      lp_linear_check_variant(variant);
   }

   if (needs_caching) {
      lp_disk_cache_insert_shader(screen, &cached, ir_sha1_cache_key);
   }

   gallivm_free_ir(variant->gallivm);

   return variant;
}

```
### 编译模块

```c
/**
 * Compile a module.
 * This does IR optimization on all functions in the module.
 */
void
gallivm_compile_module(struct gallivm_state *gallivm)
{
   int64_t time_begin = 0;

   assert(!gallivm->compiled);

   if (gallivm->builder) {
      LLVMDisposeBuilder(gallivm->builder);
      gallivm->builder = NULL;
   }

   LLVMSetDataLayout(gallivm->module, "");
   assert(!gallivm->engine);
   if (!init_gallivm_engine(gallivm)) {
            //这里创建gallivm->engine, code
            ret = lp_build_create_jit_compiler_for_module(&gallivm->engine, &gallivm->code,...);
                [-> out]

      assert(0);
   }
   assert(gallivm->engine);

    /**
     * 将模块的 DataLayout 设置为空字符串将导致 ExecutionEngine 从其目标机器复制到模块的 DataLayout 字符串。
     * 从 LLVM 3.8 开始，要求模块和执行引擎具有相同的 DataLayout。
     *
     * 我们必须确保在运行优化 passes 之后执行此操作，因为这些 passes 需要正确的 datalayout 字符串。
     * 例如，如果这些优化 passes 看到一个空的 datalayout，它们将假定这是一个小端目标，并进行破坏大端机器的优化。
     *
     * TODO：这只是一个临时解决方案。正确的解决方案是让 gallivm_init_state() 创建一个 TargetMachine，
     * 并从那里提取 DataLayout。目前，llvmpipe 使用的 TargetMachine 是由 lp_build_create_jit_compiler_for_module()
     * 中的 EngineBuilder 隐式创建的。
     */
 skip_cached:

   ++gallivm->compiled;

   lp_init_printf_hook(gallivm);
   LLVMAddGlobalMapping(gallivm->engine, gallivm->debug_printf_hook, debug_printf);

   lp_init_clock_hook(gallivm);
   LLVMAddGlobalMapping(gallivm->engine, gallivm->get_time_hook, os_time_get_nano);

   lp_build_coro_add_malloc_hooks(gallivm);

}

```

## 创建gallivm的jit执行引擎和code

```
/**
 * Same as LLVMCreateJITCompilerForModule, but:
 * - allows using MCJIT and enabling AVX feature where available.
 * - set target options
 *
 * See also:
 * - llvm/lib/ExecutionEngine/ExecutionEngineBindings.cpp
 * - llvm/tools/lli/lli.cpp
 * - http://markmail.org/message/ttkuhvgj4cxxy2on#query:+page:1+mid:aju2dggerju3ivd3+state:results
 */
extern "C"
LLVMBool
lp_build_create_jit_compiler_for_module(LLVMExecutionEngineRef *OutJIT,
                                        lp_generated_code **OutCode,
                                        struct lp_cached_code *cache_out,
                                        LLVMModuleRef M,
                                        LLVMMCJITMemoryManagerRef CMM,
                                        unsigned OptLevel,
                                        char **OutError)
{
   using namespace llvm;

   std::string Error;
   EngineBuilder builder(std::unique_ptr<Module>(unwrap(M)));

   /**
    * LLVM 3.1+ haven't more "extern unsigned llvm::StackAlignmentOverride" and
    * friends for configuring code generation options, like stack alignment.
    */
   TargetOptions options;
#if DETECT_ARCH_X86 && LLVM_VERSION_MAJOR < 13
   options.StackAlignmentOverride = 4;
#endif

   builder.setEngineKind(EngineKind::JIT)
          .setErrorStr(&Error)
          .setTargetOptions(options)
          .setOptLevel((CodeGenOpt::Level)OptLevel);

   llvm::SmallVector<std::string, 16> MAttrs;

#if 
    ...
#elif DETECT_ARCH_X86 || DETECT_ARCH_X86_64
   /*
    * Because we can override cpu caps with environment variables,
    * so we do not use llvm::sys::getHostCPUFeatures to detect cpu features
    * but using util_get_cpu_caps() instead.
    */
   MAttrs.push_back(util_get_cpu_caps()->has_sse    ? "+sse"    : "-sse"   );
   MAttrs.push_back(util_get_cpu_caps()->has_sse4_2 ? "+sse4.2" : "-sse4.2");
   /*
    * AVX feature is not automatically detected from CPUID by the X86 target
    * yet, because the old (yet default) JIT engine is not capable of
    * emitting the opcodes. On newer llvm versions it is and at least some
    * versions (tested with 3.3) will emit avx opcodes without this anyway.
    */
   MAttrs.push_back(util_get_cpu_caps()->has_avx  ? "+avx"  : "-avx");
   MAttrs.push_back(util_get_cpu_caps()->has_avx512vl ? "+avx512vl"  : "-avx512vl");
   ...

#endif
   builder.setMAttrs(MAttrs);

   StringRef MCPU = llvm::sys::getHostCPUName();

   builder.setMCPU(MCPU);

   ShaderMemoryManager *MM = NULL;
   BaseMemoryManager* JMM = reinterpret_cast<BaseMemoryManager*>(CMM);
   MM = new ShaderMemoryManager(JMM);
   *OutCode = MM->getGeneratedCode();

   builder.setMCJITMemoryManager(std::unique_ptr<RTDyldMemoryManager>(MM));
   MM = NULL; // ownership taken by std::unique_ptr

   ExecutionEngine *JIT;

   JIT = builder.create();

#if LLVM_USE_INTEL_JITEVENTS
   JITEventListener *JEL = JITEventListener::createIntelJITEventListener();
   JIT->RegisterJITEventListener(JEL);
#endif
   if (JIT) {
      *OutJIT = wrap(JIT);
      return 0;
   }
}

```
# 设置三角形bins

根据剔除模式设置相应的三角形处理接口
```
void
lp_setup_choose_triangle(struct lp_setup_context *setup)
{
   if (setup->rasterizer_discard) {
      setup->triangle = triangle_noop;
      return;
   }
   switch (setup->cullmode) {
   case PIPE_FACE_NONE:
      setup->triangle = triangle_both;
      break;
   case PIPE_FACE_BACK:
      setup->triangle = setup->ccw_is_frontface ? triangle_ccw : triangle_cw;
      break;
   case PIPE_FACE_FRONT:
      setup->triangle = setup->ccw_is_frontface ? triangle_cw : triangle_ccw;
      break;
   default:
      setup->triangle = triangle_noop;
      break;
   }
}

```

## triangle_both

```
/**
 * Draw triangle whether it's CW or CCW.
 */
static void
triangle_both(struct lp_setup_context *setup,
              const float (*v0)[4],
              const float (*v1)[4],
              const float (*v2)[4])
{
   alignas(16) struct fixed_position position;
   struct llvmpipe_context *lp_context = llvmpipe_context(setup->pipe);

   if (lp_context->active_statistics_queries) {
      lp_context->pipeline_statistics.c_primitives++;
   }
   int8_t area_sign = calc_fixed_position(setup, &position, v0, v1, v2);

   if (area_sign > 0) {
      retry_triangle_ccw(setup, &position, v0, v1, v2,
                         setup->ccw_is_frontface);
            if (!do_triangle_ccw(setup, position, v0, v1, v2, front)) {

   } else if (area_sign < 0) {
        ...
   }
}
```

### 三角形光栅化 do_triangle_ccw

```c
/**
 * 执行三角形光栅化的基本设置，并确定哪些帧缓冲瓦片被触及。
 * 将三角形放入场景的与之重叠的瓦片的 bin 中。
 */
static boolean
do_triangle_ccw(struct lp_setup_context *setup,
                struct fixed_position *position,
                const float (*v0)[4],
                const float (*v1)[4],
                const float (*v2)[4],
                boolean frontfacing)
{
   struct lp_scene *scene = setup->scene;

   const float (*pv)[4];
   if (setup->flatshade_first) {
      pv = v0;
   } else {
      pv = v2;
   }

   unsigned viewport_index = 0;
   if (setup->viewport_index_slot > 0) {
      unsigned *udata = (unsigned*)pv[setup->viewport_index_slot];
      viewport_index = lp_clamp_viewport_idx(*udata);
   }

   unsigned layer = 0;
   if (setup->layer_slot > 0) {
      layer = *(unsigned*)pv[setup->layer_slot];
      layer = MIN2(layer, scene->fb_max_layer);
   }

   /* 边界矩形（以像素为单位） */
   struct u_rect bbox;
   {
      /* 是的，这是必要的，以准确计算带有我们支持的两种填充约定的边界框。
       * GL（通常）最终需要底部左侧的填充约定，这需要稍微不同的舍入。
       */
      int adj = (setup->bottom_edge_rule != 0) ? 1 : 0;

      /* 包含 x0，不包含 x1 */
      bbox.x0 =  MIN3(position->x[0], position->x[1],
                      position->x[2]) >> FIXED_ORDER;
      bbox.x1 = (MAX3(position->x[0], position->x[1],
                      position->x[2]) - 1) >> FIXED_ORDER;

      /* 包含 / 不包含，具体取决于 adj（底部左侧或顶部右侧） */
      bbox.y0 = (MIN3(position->y[0], position->y[1],
                      position->y[2]) + adj) >> FIXED_ORDER;
      bbox.y1 = (MAX3(position->y[0], position->y[1],
                      position->y[2]) - 1 + adj) >> FIXED_ORDER;
   }

   if (!u_rect_test_intersection(&setup->draw_regions[viewport_index], &bbox)) {}

   int max_szorig = ((bbox.x1 - (bbox.x0 & ~3)) |
                     (bbox.y1 - (bbox.y0 & ~3)));
   boolean use_32bits = max_szorig <= MAX_FIXED_LENGTH32;

   /* 可以安全地丢弃负区域，但需要保留关于三角形是否延伸到屏幕边界之间的信息。
    * 参见 lp_setup_bin_triangle() 中的 trimmed_box。
    */
   bbox.x0 = MAX2(bbox.x0, 0);
   bbox.y0 = MAX2(bbox.y0, 0);

   int nr_planes = 3;

   /*
    * 确定我们需要多少个裁剪平面，即如果三角形的边界框完全位于该边界框内部，则放弃裁剪边缘。
    */
   const struct u_rect *scissor = &setup->draw_regions[viewport_index];
   boolean s_planes[4];
   scissor_planes_needed(s_planes, &bbox, scissor);
   nr_planes += s_planes[0] + s_planes[1] + s_planes[2] + s_planes[3];

   const struct lp_setup_variant_key *key = &setup->setup.variant->key;


   // 从lp_scene.data 获取内存空间保存输入属性
   // 输入属性封装在lp_rast_triangle中
   struct lp_rast_triangle *tri =
      lp_setup_alloc_triangle(scene, key->num_inputs, nr_planes);

   LP_COUNT(nr_tris);

   /* 设置参数插值器：
    */
    //调用lp_jit_setup_triangle 获取插值参数的重心坐标
    // 将数据保存在tri->input中
   setup->setup.variant->jit_function(v0, v1, v2,
                                      frontfacing,
                                      GET_A0(&tri->inputs),
                                      GET_DADX(&tri->inputs),
                                      GET_DADY(&tri->inputs),
                                      &setup->setup.variant->key);

   tri->inputs.frontfacing = frontfacing;
   tri->inputs.disable = FALSE;
   tri->inputs.is_blit = FALSE;
   tri->inputs.layer = layer;
   tri->inputs.viewport_index = viewport_index;
   tri->inputs.view_index = setup->view_index;

   struct lp_rast_plane *plane = GET_PLANES(tri);


/**
 * 设置顶点坐标和相关属性的 SSE 汇编优化部分。
 * 注意：这段代码依赖于 SSE 指令集，用于加速矢量化计算。
 */
#if DETECT_ARCH_SSE
    ...
#endif
   if (nr_planes > 3) {
      lp_setup_add_scissor_planes(scissor, &plane[3],
                                  s_planes, setup->multisample);
   }

   return lp_setup_bin_triangle(setup, tri, use_32bits,
                                check_opaque(setup, v0, v1, v2),
                                &bbox, nr_planes, viewport_index);

}

```
*  在该函数内部首先是分配了数据块和用于保存输入属性数据
* 之后加入bin命令到lp_scene， 用于光栅化


####  获取存储空间data_block流程

```
/**
 * Alloc space for a new triangle plus the input.a0/dadx/dady arrays
 * immediately after it.
 * The memory is allocated from the per-scene pool, not per-tile.
 * \param num_inputs  number of fragment shader inputs
 * \return pointer to triangle space
 */
struct lp_rast_triangle *
lp_setup_alloc_triangle(struct lp_scene *scene,
                        unsigned nr_inputs,
                        unsigned nr_planes)
{
   // add 1 for XYZW position
   unsigned input_array_sz = (nr_inputs + 1) * sizeof(float[4]);
   unsigned plane_sz = nr_planes * sizeof(struct lp_rast_plane);

   STATIC_ASSERT(sizeof(struct lp_rast_plane) % 8 == 0);

    // 由于使用的是lp_rast_triangle 根据输入大小设置分配总数大小
   const unsigned tri_size  = sizeof(struct lp_rast_triangle)
      + 3 * input_array_sz +   // 3 = da + dadx + dady
      + plane_sz;

    // 这里分配的是一个uint32*的数据指针o
    //采用opaque策略用lp_rast_triangle结构引用
   struct lp_rast_triangle *tri = lp_scene_alloc_aligned(scene, tri_size, 16);
        struct data_block_list *list = &scene->data;
        struct data_block *block = list->head;

        assert(block != NULL);

        // 当使用超过分配大小重新分配一个data_block
        //加入到data_block_list的首部
        if (block->used + size + alignment - 1 > DATA_BLOCK_SIZE) {
           block = lp_scene_new_data_block(scene);
                struct data_block *block = MALLOC_STRUCT(data_block);
                scene->scene_size += sizeof *block;
                block->used = 0;
                block->next = scene->data.head;
                scene->data.head = block;
        }


        {
           ubyte *data = block->data + block->used;
           unsigned offset = (((uintptr_t)data + alignment - 1) & ~(alignment - 1)) - (uintptr_t)data;
           block->used += offset + size;
           return data + offset;
        }



   tri->inputs.stride = input_array_sz;


   return tri;
}




`
```
### 安装bin命令到lp_scene

该接口找出所有的被三角形覆盖的tile， 设置bin_cmd

```c

boolean
lp_setup_bin_triangle(struct lp_setup_context *setup,
                      struct lp_rast_triangle *tri,
                      boolean use_32bits,
                      boolean opaque,
                      const struct u_rect *bbox,
                      int nr_planes,
                      unsigned viewport_index)
{
   struct lp_scene *scene = setup->scene;
   unsigned cmd;

   /* What is the largest power-of-two boundary this triangle crosses:
    */
   const int dx = floor_pot((bbox->x0 ^ bbox->x1) |
                            (bbox->y0 ^ bbox->y1));

   /* The largest dimension of the rasterized area of the triangle
    * (aligned to a 4x4 grid), rounded down to the nearest power of two:
    */
   const int max_sz = ((bbox->x1 - (bbox->x0 & ~3)) |
                       (bbox->y1 - (bbox->y0 & ~3)));
   const int sz = floor_pot(max_sz);

   /*
    * NOTE: It is important to use the original bounding box
    * which might contain negative values here, because if the
    * plane math may overflow or not with the 32bit rasterization
    * functions depends on the original extent of the triangle.
    */

   /* Now apply scissor, etc to the bounding box.  Could do this
    * earlier, but it confuses the logic for tri-16 and would force
    * the rasterizer to also respect scissor, etc, just for the rare
    * cases where a small triangle extends beyond the scissor.
    */
   struct u_rect trimmed_box = *bbox;
   u_rect_find_intersection(&setup->draw_regions[viewport_index],
                            &trimmed_box);

   /* Determine which tile(s) intersect the triangle's bounding box
    */
   if (dx < TILE_SIZE) {
        ...
   } else {
      struct lp_rast_plane *plane = GET_PLANES(tri);
      int64_t c[MAX_PLANES];
      int64_t ei[MAX_PLANES];

      int64_t eo[MAX_PLANES];
      int64_t xstep[MAX_PLANES];
      int64_t ystep[MAX_PLANES];

      const int ix0 = trimmed_box.x0 / TILE_SIZE;
      const int iy0 = trimmed_box.y0 / TILE_SIZE;
      const int ix1 = trimmed_box.x1 / TILE_SIZE;
      const int iy1 = trimmed_box.y1 / TILE_SIZE;

      for (int i = 0; i < nr_planes; i++) {
         c[i] = (plane[i].c +
                 IMUL64(plane[i].dcdy, iy0) * TILE_SIZE -
                 IMUL64(plane[i].dcdx, ix0) * TILE_SIZE);

         ei[i] = (plane[i].dcdy -
                  plane[i].dcdx -
                  (int64_t)plane[i].eo) << TILE_ORDER;

         eo[i] = (int64_t)plane[i].eo << TILE_ORDER;
         xstep[i] = -(((int64_t)plane[i].dcdx) << TILE_ORDER);
         ystep[i] = ((int64_t)plane[i].dcdy) << TILE_ORDER;
      }

      tri->inputs.is_blit = lp_setup_is_blit(setup, &tri->inputs);

      /* 测试与三角形相交的瓦片大小的块。
       * 丢弃完全在tri之外的块。如果块完全包含在tri中，那么bin一个lp_rast_shade_tile命令。
       * 否则，bin一个lp_rast_triangle命令。
       */
      for (int y = iy0; y <= iy1; y++) {
         boolean in = FALSE;  /* are we inside the triangle? */
         int64_t cx[MAX_PLANES];

         for (int i = 0; i < nr_planes; i++)
            cx[i] = c[i];

         for (int x = ix0; x <= ix1; x++) {
            int out = 0, partial = 0;

            for (int i = 0; i < nr_planes; i++) {
               int64_t planeout = cx[i] + eo[i];
               int64_t planepartial = cx[i] + ei[i] - 1;
               out |= (int) (planeout >> 63);
               partial |= ((int) (planepartial >> 63)) & (1<<i);
            }

            if (out) {
               /* do nothing */
               if (in)
                  break;  /* exiting triangle, all done with this row */
               LP_COUNT(nr_empty_64);
            } else if (partial) {
               /* Not trivially accepted by at least one plane -
                * rasterize/shade partial tile
                */
               int count = util_bitcount(partial);
               in = TRUE;

               if (setup->multisample)
                  cmd = lp_rast_ms_tri_tab[count];
               else
                  cmd = use_32bits ? lp_rast_32_tri_tab[count] : lp_rast_tri_tab[count];
                // 处理tile部分在三角形内部的庆奎昂
               if (!lp_scene_bin_cmd_with_state(scene, x, y,
                                                setup->fs.stored, cmd,
                                                lp_rast_arg_triangle(tri, partial)))
                  goto fail;

               LP_COUNT(nr_partially_covered_64);
            } else {
               /* triangle covers the whole tile- shade whole tile */
               LP_COUNT(nr_fully_covered_64);
               in = TRUE;
               // 处理tile在三角形内部的情况
               if (!lp_setup_whole_tile(setup, &tri->inputs, x, y, opaque))
                  goto fail;
            }

            /* Iterate cx values across the region: */
            for (int i = 0; i < nr_planes; i++)
               cx[i] += xstep[i];
         }

         /* Iterate c values down the region: */
         for (int i = 0; i < nr_planes; i++)
            c[i] += ystep[i];
      }
   }

   return TRUE;

}
```
* 根据tile block在三角形的范围分别处理其中当tile block完全在三角形内部时实则内部也是调用lp_scene_bin_cmd_with_state进行处理

#### 覆盖整个tile block

```
/**
 * 矩形覆盖整个瓷砖-着色整个瓷砖。
 * XXX 此文件中没有矩形/三角形依赖性-与lp_setup_tri.c中的相同代码共享
 * \param tx, ty  瓦片在瓦片中的位置，而不是像素
 */
boolean lp_setup_whole_tile(struct lp_setup_context *setup,
                            const struct lp_rast_shader_inputs *inputs,
                            int tx, int ty, boolean opaque)
{
   struct lp_scene *scene = setup->scene;

   LP_COUNT(nr_fully_covered_64);

   /* 如果变体是不透明的且剪裁不影响瓦片 */
   if (opaque) {
      /* 有几件事情阻止此优化的工作：
       * - 对于分层渲染，我们无法确定这是否覆盖了与先前渲染相同的层
       *   （或在清除的情况下，它们实际上总是覆盖所有层，因此优化是不可能的）。
       *   需要使用fb_max_layer而不是setup->layer_slot来确定这一点，因为即使
       *   当前没有分配插槽，以前的渲染也可能已经使用了一个。
       * - 如果场景中有任何Begin/End查询命令，那么这些都将被删除，这是非常错误的。
       *   此外，如果查询只是活动的，我们也无法进行优化，因为为了获得
       *   准确的查询结果，我们不幸需要执行渲染命令。
       */
      if (!scene->fb.zsbuf && scene->fb_max_layer == 0 &&
          !scene->had_queries) {
         /*
          * 所有先前的渲染都将被覆盖，因此重置bin。
          */
         lp_scene_bin_reset(scene, tx, ty);
      }

      if (inputs->is_blit) {
         LP_COUNT(nr_blit_64);
         return lp_scene_bin_cmd_with_state(scene, tx, ty,
                                            setup->fs.stored,
                                            LP_RAST_OP_BLIT,
                                            lp_rast_arg_inputs(inputs));
      } else {
         LP_COUNT(nr_shade_opaque_64);
         return lp_scene_bin_cmd_with_state(scene, tx, ty,
                                            setup->fs.stored,
                                            LP_RAST_OP_SHADE_TILE_OPAQUE,
                                            lp_rast_arg_inputs(inputs));
      }
   } else {
      LP_COUNT(nr_shade_64);
      return lp_scene_bin_cmd_with_state(scene, tx, ty,
                                         setup->fs.stored,
                                         LP_RAST_OP_SHADE_TILE,
                                         lp_rast_arg_inputs(inputs));
   }
}


```


#### bin数据的初始化

```c
static void
lp_setup_get_empty_scene(struct lp_setup_context *setup)
{
   assert(setup->scene == NULL);
   unsigned i;

   /* try and find a scene that isn't being used */
   for (i = 0; i < setup->num_active_scenes; i++) {
      if (setup->scenes[i]->fence) {
         if (lp_fence_signalled(setup->scenes[i]->fence)) {
            lp_scene_end_rasterization(setup->scenes[i]);
            break;
         }
      } else {
         break;
      }
   }

   if (setup->num_active_scenes + 1 > MAX_SCENES) {
      i = lp_setup_wait_empty_scene(setup);
   } else if (i == setup->num_active_scenes) {
      /* allocate a new scene */
      struct lp_scene *scene = lp_scene_create(setup);
      if (!scene) {
         /* block and reuse scenes */
         i = lp_setup_wait_empty_scene(setup);
      } else {
         LP_DBG(DEBUG_SETUP, "allocated scene: %d\n", setup->num_active_scenes);
         setup->scenes[setup->num_active_scenes] = scene;
         i = setup->num_active_scenes;
         setup->num_active_scenes++;
      }
   }

   setup->scene = setup->scenes[i];
   setup->scene->permit_linear_rasterizer = setup->permit_linear_rasterizer;
   lp_scene_begin_binning(setup->scene, &setup->fb);
}





``
#### 产生cmd_bin到lp_scene;

```c
/* Add a command to bin[x][y].
 */
static inline boolean
lp_scene_bin_command(struct lp_scene *scene,
                     unsigned x, unsigned y,
                     enum lp_rast_op cmd,
                     union lp_rast_cmd_arg arg)
{
   struct cmd_bin *bin = lp_scene_get_bin(scene, x, y);
   struct cmd_block *tail = bin->tail;

   assert(x < scene->tiles_x);
   assert(y < scene->tiles_y);
   assert(cmd < LP_RAST_OP_MAX);

   if (tail == NULL || tail->count == CMD_BLOCK_MAX) {
      tail = lp_scene_new_cmd_block(scene, bin);

      assert(tail->count == 0);
   }

   {
      unsigned i = tail->count;
      tail->cmd[i] = cmd & LP_RAST_OP_MASK;
      tail->arg[i] = arg;
      tail->count++;
   }

   return TRUE;
}


```

# llvmpipe状态处理

在draw的时候先处理驱动状态
```c

/**
 * Handle state changes.
 * Called just prior to drawing anything (pipe::draw_arrays(), etc).
 *
 * Hopefully this will remain quite simple, otherwise need to pull in
 * something like the gallium frontend mechanism.
 */
void
llvmpipe_update_derived(struct llvmpipe_context *llvmpipe)
{
   struct llvmpipe_screen *lp_screen = llvmpipe_screen(llvmpipe->pipe.screen);

   /* Check for updated textures.
    */
   if (llvmpipe->tex_timestamp != lp_screen->timestamp) {
      llvmpipe->tex_timestamp = lp_screen->timestamp;
      llvmpipe->dirty |= LP_NEW_SAMPLER_VIEW;
   }

   /* This needs LP_NEW_RASTERIZER because of draw_prepare_shader_outputs(). */
   if (llvmpipe->dirty & (LP_NEW_RASTERIZER |
                          LP_NEW_FS |
                          LP_NEW_GS |
                          LP_NEW_TCS |
                          LP_NEW_TES |
                          LP_NEW_VS))
      compute_vertex_info(llvmpipe);

   if (llvmpipe->dirty & (LP_NEW_FS |
                          LP_NEW_FRAMEBUFFER |
                          LP_NEW_BLEND |
                          LP_NEW_SCISSOR |
                          LP_NEW_DEPTH_STENCIL_ALPHA |
                          LP_NEW_RASTERIZER |
                          LP_NEW_SAMPLER |
                          LP_NEW_SAMPLER_VIEW |
                          LP_NEW_OCCLUSION_QUERY))
      llvmpipe_update_fs(llvmpipe);

   if (llvmpipe->dirty & (LP_NEW_FS |
                          LP_NEW_FRAMEBUFFER |
                          LP_NEW_RASTERIZER |
                          LP_NEW_SAMPLE_MASK |
                          LP_NEW_DEPTH_STENCIL_ALPHA)) {
      boolean discard =
         llvmpipe->rasterizer ? llvmpipe->rasterizer->rasterizer_discard : FALSE;
      lp_setup_set_rasterizer_discard(llvmpipe->setup, discard);
   }

    //更新lp_setup_variant 函数,实则更新lp_jit_setup_triangle
   if (llvmpipe->dirty & (LP_NEW_FS |
                          LP_NEW_FRAMEBUFFER |
                          LP_NEW_RASTERIZER))
      llvmpipe_update_setup(llvmpipe);

   if (llvmpipe->dirty & LP_NEW_SAMPLE_MASK)
      lp_setup_set_sample_mask(llvmpipe->setup, llvmpipe->sample_mask);

   if (llvmpipe->dirty & LP_NEW_BLEND_COLOR)
      lp_setup_set_blend_color(llvmpipe->setup,
                               &llvmpipe->blend_color);

   if (llvmpipe->dirty & LP_NEW_SCISSOR)
      lp_setup_set_scissors(llvmpipe->setup, llvmpipe->scissors);

   if (llvmpipe->dirty & LP_NEW_DEPTH_STENCIL_ALPHA) {
      lp_setup_set_alpha_ref_value(llvmpipe->setup,
                                   llvmpipe->depth_stencil->alpha_ref_value);
      lp_setup_set_stencil_ref_values(llvmpipe->setup,
                                      llvmpipe->stencil_ref.ref_value);
   }

   if (llvmpipe->dirty & LP_NEW_FS_CONSTANTS)
      lp_setup_set_fs_constants(llvmpipe->setup,
                                ARRAY_SIZE(llvmpipe->constants[PIPE_SHADER_FRAGMENT]),
                                llvmpipe->constants[PIPE_SHADER_FRAGMENT]);

   if (llvmpipe->dirty & LP_NEW_FS_SSBOS)
      lp_setup_set_fs_ssbos(llvmpipe->setup,
                            ARRAY_SIZE(llvmpipe->ssbos[PIPE_SHADER_FRAGMENT]),
                            llvmpipe->ssbos[PIPE_SHADER_FRAGMENT], llvmpipe->fs_ssbo_write_mask);

   if (llvmpipe->dirty & LP_NEW_FS_IMAGES)
      lp_setup_set_fs_images(llvmpipe->setup,
                             ARRAY_SIZE(llvmpipe->images[PIPE_SHADER_FRAGMENT]),
                             llvmpipe->images[PIPE_SHADER_FRAGMENT]);

   if (llvmpipe->dirty & (LP_NEW_SAMPLER_VIEW))
      lp_setup_set_fragment_sampler_views(llvmpipe->setup,
                                          llvmpipe->num_sampler_views[PIPE_SHADER_FRAGMENT],
                                          llvmpipe->sampler_views[PIPE_SHADER_FRAGMENT]);

   if (llvmpipe->dirty & (LP_NEW_SAMPLER))
      lp_setup_set_fragment_sampler_state(llvmpipe->setup,
                                          llvmpipe->num_samplers[PIPE_SHADER_FRAGMENT],
                                          llvmpipe->samplers[PIPE_SHADER_FRAGMENT]);

   if (llvmpipe->dirty & LP_NEW_VIEWPORT) {
      /*
       * Update setup and fragment's view of the active viewport state.
       *
       * XXX TODO: It is possible to only loop over the active viewports
       *           instead of all viewports (PIPE_MAX_VIEWPORTS).
       */
      lp_setup_set_viewports(llvmpipe->setup,
                             PIPE_MAX_VIEWPORTS,
                             llvmpipe->viewports);
   }

   llvmpipe_update_derived_clear(llvmpipe);

   llvmpipe->dirty = 0;
}

```
# 设置变量lp_setup_variant生成

```c
 * Update fragment/vertex shader linkage state.  This is called just
 * prior to drawing something when some fragment-related state has
 * changed.
 */
void
llvmpipe_update_setup(struct llvmpipe_context *lp)
{
   struct lp_setup_variant_key *key = &lp->setup_variant.key;
   struct lp_setup_variant *variant = NULL;
   struct lp_setup_variant_list_item *li;

   if (variant) {
      list_move_to(&variant->list_item_global.list, &lp->setup_variants_list.list);
   } else {

      
      variant = generate_setup_variant(key, lp);
      if (variant) {
         list_add(&variant->list_item_global.list, &lp->setup_variants_list.list);
         lp->nr_setup_variants++;
      }
   }

   lp_setup_set_setup_variant(lp->setup, variant);
}


```
## 插值系数重心坐标计算函数setup_variant_\* LLVM IR生成

```c
/**
 * Generate the runtime callable function for the coefficient calculation.
 *
 */
static struct lp_setup_variant *
generate_setup_variant(struct lp_setup_variant_key *key,
                       struct llvmpipe_context *lp)
{
   int64_t t0 = 0, t1;

   struct lp_setup_variant *variant = CALLOC_STRUCT(lp_setup_variant);

   variant->no = setup_no++;

   char func_name[64];
   snprintf(func_name, sizeof(func_name), "setup_variant_%u",
            variant->no);

   struct gallivm_state *gallivm;
   variant->gallivm = gallivm = gallivm_create(func_name, lp->context, NULL);

   LLVMBuilderRef builder = gallivm->builder;

   memcpy(&variant->key, key, key->size);
   variant->list_item_global.base = variant;

   /* Currently always deal with full 4-wide vertex attributes from
    * the vertices.
    */

   LLVMTypeRef vec4f_type =
      LLVMVectorType(LLVMFloatTypeInContext(gallivm->context), 4);

   LLVMTypeRef arg_types[8];
   arg_types[0] = LLVMPointerType(vec4f_type, 0);        /* v0 */
   arg_types[1] = LLVMPointerType(vec4f_type, 0);        /* v1 */
   arg_types[2] = LLVMPointerType(vec4f_type, 0);        /* v2 */
   arg_types[3] = LLVMInt32TypeInContext(gallivm->context); /* facing */
   arg_types[4] = LLVMPointerType(vec4f_type, 0);	/* a0, aligned */
   arg_types[5] = LLVMPointerType(vec4f_type, 0);	/* dadx, aligned */
   arg_types[6] = LLVMPointerType(vec4f_type, 0);	/* dady, aligned */
   arg_types[7] = LLVMPointerType(vec4f_type, 0);	/* key (placeholder) */

   LLVMTypeRef func_type =
      LLVMFunctionType(LLVMVoidTypeInContext(gallivm->context),
                       arg_types, ARRAY_SIZE(arg_types), 0);

   variant->function = LLVMAddFunction(gallivm->module, func_name, func_type);
   if (!variant->function)
      goto fail;

   LLVMSetFunctionCallConv(variant->function, LLVMCCallConv);


    // v0,v1v2 a0 dadx daxy 都表示数组根据slot索引属性
   struct lp_setup_args args;
   args.vec4f_type = vec4f_type;
   args.v0       = LLVMGetParam(variant->function, 0);
   args.v1       = LLVMGetParam(variant->function, 1);
   args.v2       = LLVMGetParam(variant->function, 2);
   args.facing   = LLVMGetParam(variant->function, 3);
   args.a0       = LLVMGetParam(variant->function, 4);
   args.dadx     = LLVMGetParam(variant->function, 5);
   args.dady     = LLVMGetParam(variant->function, 6);
   args.key      = LLVMGetParam(variant->function, 7);



    // out表示输出参数
   lp_build_name(args.v0, "in_v0");
   lp_build_name(args.v1, "in_v1");
   lp_build_name(args.v2, "in_v2");
   lp_build_name(args.facing, "in_facing");
   lp_build_name(args.a0, "out_a0");
   lp_build_name(args.dadx, "out_dadx");
   lp_build_name(args.dady, "out_dady");
   lp_build_name(args.key, "key");

   /*
    * Function body
    */
   LLVMBasicBlockRef block =
      LLVMAppendBasicBlockInContext(gallivm->context,
                                    variant->function, "entry");
   LLVMPositionBuilderAtEnd(builder, block);

   set_noalias(builder, variant->function, arg_types, ARRAY_SIZE(arg_types));



    
   // 计算slot0 gl_Position 的参数
   init_args(gallivm, &variant->key, &args);
       /* The internal position input is in slot zero:
       load_attribute(gallivm, args, key, 0, attr_pos);
       calc_coef4(gallivm, args, attr_pos[0], attr_pos[1], attr_pos[2], coeffs);
       store_coef(gallivm, args, 0, coeffs[0], coeffs[1], coeffs[2]);


    // 计算其他属性插值的重心坐标
   emit_tri_coef(gallivm, &variant->key, &args);
       LVMValueRef attribs[3];

       /* setup interpolation for all the remaining attributes */
       for (unsigned slot = 0; slot < key->num_inputs; slot++) {
          switch (key->inputs[slot].interp) {
          case LP_INTERP_CONSTANT:
            ...
          case LP_INTERP_LINEAR:
            ...
          case LP_INTERP_PERSPECTIVE:
             load_attribute(gallivm, args, key, key->inputs[slot].src_index, attribs);
             apply_perspective_corr(gallivm, args, slot+1, attribs);
             emit_linear_coef(gallivm, args, slot+1, attribs);
             break;

          case LP_INTERP_POSITION:
             /*
              * The generated pixel interpolators will pick up the coeffs from
              * slot 0.
              */
             break;

          case LP_INTERP_FACING:
             emit_facing_coef(gallivm, args, slot+1);
             break;
          }
       }


   LLVMBuildRetVoid(builder);

   gallivm_verify_function(gallivm, variant->function);

    // 编译代码加入jit执行模块
   gallivm_compile_module(gallivm);


   //设置jit_function用于globalmap 
   variant->jit_function = (lp_jit_setup_triangle)
      gallivm_jit_function(gallivm, variant->function);
   if (!variant->jit_function)
      goto fail;

   return variant;
}



```
# 光栅化流程


## 创建光栅化线程

llvmpipe中默认线程数为2
```
llvmpipe_create_context(struct pipe_screen *screen, void *priv,
  llvmpipe_screen_late_init(lp_screen)
     screen->rast = lp_rast_create(screen->num_threads);
        create_rast_threads(rast);
           /* NOTE: if num_threads is zero, we won't use any threads */
           for (unsigned i = 0; i < rast->num_threads; i++) {
              util_semaphore_init(&rast->tasks[i].work_ready, 0);
              util_semaphore_init(&rast->tasks[i].work_done, 0);
              if (thrd_success != u_thread_create(rast->threads + i, thread_function,
                                                    (void *) &rast->tasks[i])) {
                 rast->num_threads = i; /* previous thread is max */
                 break;
              }
           }

/**
 * This is the thread's main entrypoint.
 * It's a simple loop:
 *   1. wait for work
 *   2. do work
 *   3. signal that we're done
 */
static int
thread_function(void *init_data)
{
   struct lp_rasterizer_task *task = (struct lp_rasterizer_task *) init_data;
   struct lp_rasterizer *rast = task->rast;
   boolean debug = false;
   char thread_name[16];

   /* Make sure that denorms are treated like zeros. This is
    * the behavior required by D3D10. OpenGL doesn't care.
    */
   unsigned fpstate = util_fpstate_get();
   util_fpstate_set_denorms_to_zero(fpstate);

   while (1) {
      if (task->thread_index == 0) {
         /* thread[0]:
          *  - get next scene to rasterize
          *  - map the framebuffer surfaces
          */
            // 光栅化初始化
         lp_rast_begin(rast, lp_scene_dequeue(rast->full_scenes, TRUE));
      }

      /* Wait for all threads to get here so that threads[1+] don't
       * get a null rast->curr_scene pointer.
       */
      util_barrier_wait(&rast->barrier);


     // 光栅化
      rasterize_scene(task, rast->curr_scene);

      /* wait for all threads to finish with this scene */
      util_barrier_wait(&rast->barrier);

      /* XXX: shouldn't be necessary:
       */
      if (task->thread_index == 0) {
         lp_rast_end(rast);
      }

   }

   return 0;
}

```

### 光栅化任务入列


通过glFlush-> llvmpipe_flush将setup好的模块分发给各个线程

```c

/**
 * \param fence  if non-null, returns pointer to a fence which can be waited on
 */
void
llvmpipe_flush(struct pipe_context *pipe,
               struct pipe_fence_handle **fence,
               const char *reason)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   struct llvmpipe_screen *screen = llvmpipe_screen(pipe->screen);

   draw_flush(llvmpipe->draw);

   /* ask the setup module to flush */
   lp_setup_flush(llvmpipe->setup, reason);
        set_scene_state(setup, SETUP_FLUSHED, reason);
           /* wait for a free/empty scene
            */
           if (old_state == SETUP_FLUSHED)
              lp_setup_get_empty_scene(setup);


           case SETUP_FLUSHED:
              if (old_state == SETUP_CLEARED)
                 if (!execute_clears(setup))
                    goto fail;
              lp_setup_rasterize_scene(setup);
              assert(setup->scene == NULL);
              break;


   mtx_lock(&screen->rast_mutex);
   lp_rast_fence(screen->rast, (struct lp_fence **)fence);
   mtx_unlock(&screen->rast_mutex);

   if (fence && (!*fence))
      *fence = (struct pipe_fence_handle *)lp_fence_create(0);
}


/** Rasterize all scene's bins */
static void
lp_setup_rasterize_scene(struct lp_setup_context *setup)
{
   struct lp_scene *scene = setup->scene;
   struct llvmpipe_screen *screen = llvmpipe_screen(scene->pipe->screen);

   scene->num_active_queries = setup->active_binned_queries;
   memcpy(scene->active_queries, setup->active_queries,
          scene->num_active_queries * sizeof(scene->active_queries[0]));

   mtx_lock(&screen->rast_mutex);
   lp_rast_queue_scene(screen->rast, scene);

      // 将screen加入环形队列rast->full_scenes
      lp_scene_enqueue(rast->full_scenes, scene);
            queue->scenes[queue->tail++ % SCENE_QUEUE_SIZE] = scene;
        
      /* signal the threads that there's work to do */
      for (unsigned i = 0; i < rast->num_threads; i++) {
         util_semaphore_signal(&rast->tasks[i].work_ready);
      }

   mtx_unlock(&screen->rast_mutex);

   lp_setup_reset(setup);

   LP_DBG(DEBUG_SETUP, "%s done \n", __func__);
}

```

### 光栅化


#### 循环遍历所需要的光栅化分箱处理

```c

/**
 * Rasterize/execute all bins within a scene.
 * Called per thread.
 */
static void
rasterize_scene(struct lp_rasterizer_task *task,
                struct lp_scene *scene)
{
   task->scene = scene;

   if (!task->rast->no_rast) {
      /* loop over scene bins, rasterize each */
      struct cmd_bin *bin;
      int i, j;

      assert(scene);
      while ((bin = lp_scene_bin_iter_next(scene, &i, &j))) {
         if (!is_empty_bin(bin))
            rasterize_bin(task, bin, i, j);
      }
   }

   if (scene->fence) {
      lp_fence_signal(scene->fence);
   }

   task->scene = NULL;
}
````

# 光栅化分箱bin

```
/**
 * Rasterize commands for a single bin.
 * \param x, y  position of the bin's tile in the framebuffer
 * Must be called between lp_rast_begin() and lp_rast_end().
 * Called per thread.
 */
static void
rasterize_bin(struct lp_rasterizer_task *task,
              const struct cmd_bin *bin, int x, int y)
{
   struct lp_bin_info info = lp_characterize_bin(bin);

   lp_rast_tile_begin(task, bin, x, y);

   if (LP_DEBUG & DEBUG_NO_FASTPATH) {
      debug_rasterize_bin(task, bin);
   } else if (info.type & LP_RAST_FLAGS_BLIT) {
      blit_rasterize_bin(task, bin);
   } else if (task->scene->permit_linear_rasterizer &&
            !(LP_PERF & PERF_NO_RAST_LINEAR) &&
            (info.type & LP_RAST_FLAGS_RECT)) {
      lp_linear_rasterize_bin(task, bin);
   } else {
      tri_rasterize_bin(task, bin, x, y);
   }

   lp_rast_tile_end(task);

}
```
* 分为




setup->num_active_scenes + 1 

setup->num_active_scenes + 1  > MAX_SCENS ) {
    i = lp_setup_wait_empty_scene(setup);
} else 
{ 
    struct lp_scene \*scene = lp_scene_create(setup);
    if (!scene) {
        i = lp_setup_wait_empty(scen
    }

}
    

static unsigned 
lp_setup_wait_empty_scnee(struct lp_struct_context *setup)
{
    if (setup->scens[0]->fence)  {
        lp_fence_wait(setup->scenes[0]->fence);
        lp_scene_end_rasterization(setup->scenes[0]);
    }
    return 0;
}


lp_setup_get_empty_scene(structr lp_setup_context \*setup)
{

    assert(setup->scene == NULL);
    unsigned i;

    for (i = 0; i < setup->num_active_scenes ; i++) {
        if (setup->scenes[i]->fence) {
            lp_scene_end_rasterization(setup_>scens[i]);

       } 
    }
    llvmpipe(
}





# 出图流程


## glx 

### glXSwapBuffers


```

_GLX_PUBLIC void
glXSwapBuffers(Display * dpy, GLXDrawable drawable)
{
#ifdef GLX_USE_APPLEGL
#else
   struct glx_context *gc;
   GLXContextTag tag;
   CARD8 opcode;
   xcb_connection_t *c;

   gc = __glXGetCurrentContext();

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
   {
      __GLXDRIdrawable *pdraw = GetGLXDRIDrawable(dpy, drawable);

      if (pdraw != NULL) {
         Bool flush = gc != &dummyContext && drawable == gc->currentDrawable;

         if (pdraw->psc->driScreen->swapBuffers(pdraw, 0, 0, 0, flush) == -1)
            [-> driswSwapBuffers]
             __glXSendError(dpy, GLXBadCurrentWindow, 0, X_GLXSwapBuffers, false);
         return;
      }
   }
#endif

#endif /* GLX_USE_APPLEGL */
}

```

### driswSwapBuffers

```
static int64_t
driswSwapBuffers(__GLXDRIdrawable * pdraw,
                 int64_t target_msc, int64_t divisor, int64_t remainder,
                 Bool flush)
{
   struct drisw_drawable *pdp = (struct drisw_drawable *) pdraw;
   struct drisw_screen *psc = (struct drisw_screen *) pdp->base.psc;

   (void) target_msc;
   (void) divisor;
   (void) remainder;

   if (flush) {
      glFlush();
   }

   if (psc->kopper)
       return psc->kopper->swapBuffers (pdp->driDrawable, 0);

   psc->core->swapBuffers(pdp->driDrawable);
        [-> driSwapBuffers]
            drawable->swap_buffers(drawable);
            [-> drisw_swap_buffers]
       


   return 0;
}
```


### swrastPutImageShm2(__DRIdrawable * draw, int op,


```c
static void
swrastPutImageShm2(__DRIdrawable * draw, int op,
                   int x, int y,
                   int w, int h, int stride,
                   int shmid, char *shmaddr, unsigned offset,
                   void *loaderPrivate)
{
   struct drisw_drawable *pdp = loaderPrivate;

   if (!pdp)
      return;

   pdp->shminfo.shmaddr = shmaddr;
   swrastXPutImage(draw, op, x, 0, x, y, w, h, stride, shmid,
                   shmaddr + offset, loaderPrivate);
       struct drisw_drawable *pdp = loaderPrivate;
       __GLXDRIdrawable *pdraw = &(pdp->base);
       Display *dpy = pdraw->psc->dpy;
       Drawable drawable;
       XImage *ximage;
       GC gc = pdp->gc;
    
       if (!pdp->ximage || shmid != pdp->shminfo.shmid) {
          if (!XCreateDrawable(pdp, shmid, dpy))
             return;
       }
    
       drawable = pdraw->xDrawable;
       ximage = pdp->ximage;
       ximage->bytes_per_line = stride ? stride : bytes_per_line(w * ximage->bits_per_pixel, 32);
       ximage->data = data;
    
       ximage->width = ximage->bytes_per_line / ((ximage->bits_per_pixel + 7)/ 8);
       ximage->height = h;
    
       if (pdp->shminfo.shmid >= 0) {
          XShmPutImage(dpy, drawable, gc, ximage, srcx, srcy, x, y, w, h, False);
          XSync(dpy, False);
       } else {
          XPutImage(dpy, drawable, gc, ximage, srcx, srcy, x, y, w, h);
       }
       ximage->data = NULL;

}


static Bool
XCreateDrawable(struct drisw_drawable * pdp, int shmid, Display * dpy)
{

   if (!xshm_error && shmid >= 0) {
      pdp->shminfo.shmid = shmid;
      pdp->ximage = XShmCreateImage(dpy,
                                    NULL,
                                    pdp->xDepth,
                                    ZPixmap,              /* format */
                                    NULL,                 /* data */
                                    &pdp->shminfo,        /* shminfo */
}



```

* create
## dri


### drisw_swap_buffers

```

/*
 * Backend functions for pipe_frontend_drawable and swap_buffers.
 */

static void
drisw_swap_buffers(struct dri_drawable *drawable)
{
   struct dri_context *ctx = dri_get_current();
   struct dri_screen *screen = drawable->screen;
   struct pipe_resource *ptex;

   /* Wait for glthread to finish because we can't use pipe_context from
    * multiple threads.
    */
   _mesa_glthread_finish(ctx->st->ctx);

   ptex = drawable->textures[ST_ATTACHMENT_BACK_LEFT];

   if (ptex) {
      struct pipe_fence_handle *fence = NULL;
      if (ctx->pp)
         pp_run(ctx->pp, ptex, ptex, drawable->textures[ST_ATTACHMENT_DEPTH_STENCIL]);

      if (ctx->hud)
         hud_run(ctx->hud, ctx->st->cso_context, ptex);

      st_context_flush(ctx->st, ST_FLUSH_FRONT, &fence, NULL, NULL);

      if (drawable->stvis.samples > 1) {
         /* Resolve the back buffer. */
         dri_pipe_blit(ctx->st->pipe,
                       drawable->textures[ST_ATTACHMENT_BACK_LEFT],
                       drawable->msaa_textures[ST_ATTACHMENT_BACK_LEFT]);
      }

      screen->base.screen->fence_finish(screen->base.screen, ctx->st->pipe,
                                        fence, PIPE_TIMEOUT_INFINITE);
      screen->base.screen->fence_reference(screen->base.screen, &fence, NULL);
      drisw_copy_to_front(ctx->st->pipe, drawable, ptex);
           drisw_present_texture(pipe, drawable, ptex,sub_box= NULL);
                struct dri_screen *screen = drawable->screen;
                screen->base.screen->flush_frontbuffer(screen->base.screen, pipe, ptex, 0, 0, drawable, sub_box);
                  [-> llvmpipe_flush_frontbuffer]

           drisw_invalidate_drawable(drawable);



      /* TODO: remove this if the framebuffer state doesn't change. */
      st_context_invalidate_state(ctx->st, ST_INVALIDATE_FB_STATE);
   }
}
```





## llvmpipe 



### llvmpipe_flush_frontbuffer

```
static void
llvmpipe_flush_frontbuffer(struct pipe_screen *_screen,
                           struct pipe_context *_pipe,
                           struct pipe_resource *resource,
                           unsigned level, unsigned layer,
                           void *context_private,
                           struct pipe_box *sub_box)
{
   struct llvmpipe_screen *screen = llvmpipe_screen(_screen);
   struct sw_winsys *winsys = screen->winsys;
   struct llvmpipe_resource *texture = llvmpipe_resource(resource);

   assert(texture->dt);

   if (texture->dt) {
      if (_pipe)
         llvmpipe_flush_resource(_pipe, resource, 0, true, true,
                                 false, reason="frontbuffer");
                llvmpipe_finish(pipe, reason);
                    llvmpipe_flush(pipe, &fence, reason);

      winsys->displaytarget_display(winsys, texture->dt,
                                    context_private, sub_box);
            [-> sw_winsys  dri_sw_displaytarget_display]
   }
}

```

## sw_winsys


### dri_sw_displaytarget_display

```c
static void
dri_sw_displaytarget_display(struct sw_winsys *ws,
                             struct sw_displaytarget *dt,
                             void *context_private,
                             struct pipe_box *box)
{
   struct dri_sw_winsys *dri_sw_ws = dri_sw_winsys(ws);
   struct dri_sw_displaytarget *dri_sw_dt = dri_sw_displaytarget(dt);
   struct dri_drawable *dri_drawable = (struct dri_drawable *)context_private;
   unsigned width, height, x = 0, y = 0;
   unsigned blsize = util_format_get_blocksize(dri_sw_dt->format);
   unsigned offset = 0;
   unsigned offset_x = 0;
   char *data = dri_sw_dt->data;
   bool is_shm = dri_sw_dt->shmid != -1;
   /* Set the width to 'stride / cpp'.
    *
    * PutImage correctly clips to the width of the dst drawable.
    */
   if (box) {
      offset = dri_sw_dt->stride * box->y;
      offset_x = box->x * blsize;
      data += offset;
      /* don't add x offset for shm, the put_image_shm will deal with it */
      if (!is_shm)
         data += offset_x;
      x = box->x;
      y = box->y;
      width = box->width;
      height = box->height;
   } else {
      width = dri_sw_dt->stride / blsize;
      height = dri_sw_dt->height;
   }

   if (is_shm) {
      dri_sw_ws->lf->put_image_shm(dri_drawable, dri_sw_dt->shmid, dri_sw_dt->data, offset, offset_x,
                                   x, y, width, height, dri_sw_dt->stride);

            [-> dri drisw_put_image_shm]
                put_image_shm(drawable, shmid, shmaddr, offset, offset_x, x, y, width, height, stride);
                   const __DRIswrastLoaderExtension *loader = drawable->screen->swrast_loader;

                   /* if we have the newer interface, don't have to add the offset_x here. */
                   if (loader->base.version > 4 && loader->putImageShm2)
                     loader->putImageShm2(opaque_dri_drawable(drawable), __DRI_SWRAST_IMAGE_OP_SWAP,
                                          x, y, width, height, stride,
                                          shmid, shmaddr, offset, drawable->loaderPrivate);
                     [-> glx swrastPutImageShm2]

                   else
                     loader->putImageShm(opaque_dri_drawable(drawable), __DRI_SWRAST_IMAGE_OP_SWAP,
                                         x, y, width, height, stride,
                                         shmid, shmaddr, offset + offset_x, drawable->loaderPrivate);

                    
                
      return;
   }

   if (box)
      dri_sw_ws->lf->put_image2(dri_drawable, data,
                                x, y, width, height, dri_sw_dt->stride);
   else
      dri_sw_ws->lf->put_image(dri_drawable, data, width, height);
}

```

# 计算三角形固定位置



# 计算像素块着色 

```c
/**
 * Compute shading for a 4x4 block of pixels inside a triangle.
 * This is a bin command called during bin processing.
 * \param x  X position of quad in window coords
 * \param y  Y position of quad in window coords
 */
void
lp_rast_shade_quads_mask_sample(struct lp_rasterizer_task *task,
                                const struct lp_rast_shader_inputs *inputs,
                                unsigned x, unsigned y,
                                uint64_t mask)
{
   const struct lp_rast_state *state = task->state;
   const struct lp_fragment_shader_variant *variant = state->variant;
   const struct lp_scene *scene = task->scene;

   assert(state);

   /* Sanity checks */
   assert(x < scene->tiles_x * TILE_SIZE);
   assert(y < scene->tiles_y * TILE_SIZE);
   assert(x % TILE_VECTOR_WIDTH == 0);
   assert(y % TILE_VECTOR_HEIGHT == 0);

   assert((x % 4) == 0);
   assert((y % 4) == 0);

   /* color buffer */
   uint8_t *color[PIPE_MAX_COLOR_BUFS];
   unsigned stride[PIPE_MAX_COLOR_BUFS];
   unsigned sample_stride[PIPE_MAX_COLOR_BUFS];
   for (unsigned i = 0; i < scene->fb.nr_cbufs; i++) {
      if (scene->fb.cbufs[i]) {
         stride[i] = scene->cbufs[i].stride;
         sample_stride[i] = scene->cbufs[i].sample_stride;
         color[i] = lp_rast_get_color_block_pointer(task, i, x, y,
                                                    inputs->layer + inputs->view_index);
      } else {
         stride[i] = 0;
         sample_stride[i] = 0;
         color[i] = NULL;
      }
   }

   /* depth buffer */
   uint8_t *depth = NULL;
   unsigned depth_stride = 0;
   unsigned depth_sample_stride = 0;
   if (scene->zsbuf.map) {
      depth_stride = scene->zsbuf.stride;
      depth_sample_stride = scene->zsbuf.sample_stride;
      depth = lp_rast_get_depth_block_pointer(task, x, y, inputs->layer + inputs->view_index);
   }

   assert(lp_check_alignment(state->jit_context.u8_blend_color, 16));

   /*
    * The rasterizer may produce fragments outside our
    * allocated 4x4 blocks hence need to filter them out here.
    */
   if ((x % TILE_SIZE) < task->width && (y % TILE_SIZE) < task->height) {
      /* Propagate non-interpolated raster state. */
      task->thread_data.raster_state.viewport_index = inputs->viewport_index;
      task->thread_data.raster_state.view_index = inputs->view_index;

      /* run shader on 4x4 block */
      BEGIN_JIT_CALL(state, task);
      // 调用fs LLVM IR 产生像素颜色
      variant->jit_function[RAST_EDGE_TEST](&state->jit_context,
                                            x, y,
                                            inputs->frontfacing,
                                            GET_A0(inputs),
                                            GET_DADX(inputs),
                                            GET_DADY(inputs),
                                            color,
                                            depth,
                                            mask,
                                            &task->thread_data,
                                            stride,
                                            depth_stride,
                                            sample_stride,
                                            depth_sample_stride);
      END_JIT_CALL();
   }
}


```

# 图元装配 PrimAssembler


```c
/*
 * Primitive assembler breaks up adjacency primitives and assembles
 * the base primitives they represent, e.g. vertices forming
 * PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY
 * become vertices forming PIPE_PRIM_TRIANGLES
 * This is needed because specification says that the adjacency
 * primitives are only visible in the geometry shader so we need
 * to get rid of them so that the rest of the pipeline can
 * process the inputs.
 */
void
draw_prim_assembler_run(struct draw_context *draw,
                        const struct draw_prim_info *input_prims,
                        const struct draw_vertex_info *input_verts,
                        struct draw_prim_info *output_prims,
                        struct draw_vertex_info *output_verts)
{
   struct draw_assembler *asmblr = draw->ia;
   unsigned start, i;
   unsigned assembled_prim = (input_prims->prim == PIPE_PRIM_QUADS ||
                              input_prims->prim == PIPE_PRIM_QUAD_STRIP) ?
      PIPE_PRIM_QUADS : u_reduced_prim(input_prims->prim);
   unsigned max_primitives = u_decomposed_prims_for_vertices(
      input_prims->prim, input_prims->count);
   unsigned max_verts = u_vertices_per_prim(assembled_prim) * max_primitives;

   asmblr->output_prims = output_prims;
   asmblr->output_verts = output_verts;
   asmblr->input_prims = input_prims;
   asmblr->input_verts = input_verts;
   asmblr->needs_primid = needs_primid(asmblr->draw);
   asmblr->num_prims = 0;

   output_prims->linear = TRUE;
   output_prims->elts = NULL;
   output_prims->start = 0;
   output_prims->prim = assembled_prim;
   output_prims->flags = 0x0;
   output_prims->primitive_lengths = MALLOC(sizeof(unsigned));
   output_prims->primitive_lengths[0] = 0;
   output_prims->primitive_count = 1;

   output_verts->vertex_size = input_verts->vertex_size;
   output_verts->stride = input_verts->stride;
   output_verts->verts = (struct vertex_header*)MALLOC(
      input_verts->vertex_size * max_verts + DRAW_EXTRA_VERTICES_PADDING);
   output_verts->count = 0;


   for (start = i = 0; i < input_prims->primitive_count;
        start += input_prims->primitive_lengths[i], i++) {
      unsigned count = input_prims->primitive_lengths[i];
      if (input_prims->linear) {

        // draw_prim_assembler.c #define FUNC assembler_run_linear
         assembler_run_linear(asmblr, input_prims, input_verts,
                              start, count);
      } else {
         assembler_run_elts(asmblr, input_prims, input_verts,
                            start, count);
      }
   }

   output_prims->count = output_verts->count;
}


```

##  assembler_run_linear/ assembler_run_elts 
```c
static void
FUNC(FUNC_VARS)
{
   unsigned idx[6], i;
   ushort flags;
   LOCAL_VARS

   FUNC_ENTER;

   /* prim, prim_flags, count, and last_vertex_last should have been defined */
   if (0) {
      debug_printf("%s: prim 0x%x, prim_flags 0x%x, count %d, last_vertex_last %d\n",
            __func__, prim, prim_flags, count, last_vertex_last);
   }

   switch (prim) {
   case PIPE_PRIM_POINTS:
      for (i = 0; i < count; i++) {
         idx[0] = GET_ELT(i);
         POINT(idx[0]);
      }
      break;

   case PIPE_PRIM_LINES:
      flags = DRAW_PIPE_RESET_STIPPLE;
      for (i = 0; i + 1 < count; i += 2) {
         idx[0] = GET_ELT(i);
         idx[1] = GET_ELT(i + 1);
         LINE(flags, idx[0], idx[1]);
      }
      break;

   case PIPE_PRIM_LINE_LOOP:
   case PIPE_PRIM_LINE_STRIP:
      if (count >= 2) {
         flags = (prim_flags & DRAW_SPLIT_BEFORE) ? 0 : DRAW_PIPE_RESET_STIPPLE;
         idx[1] = GET_ELT(0);
         idx[2] = idx[1];

         for (i = 1; i < count; i++, flags = 0) {
            idx[0] = idx[1];
            idx[1] = GET_ELT(i);
            LINE(flags, idx[0], idx[1]);
         }
         /* close the loop */
         if (prim == PIPE_PRIM_LINE_LOOP && !prim_flags)
            LINE(flags, idx[1], idx[2]);
      }
      break;

   case PIPE_PRIM_TRIANGLES:
      flags = DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL;
      for (i = 0; i + 2 < count; i += 3) {
         idx[0] = GET_ELT(i);
         idx[1] = GET_ELT(i + 1);
         idx[2] = GET_ELT(i + 2);
         TRIANGLE(flags, idx[0], idx[1], idx[2]);
      }
      break;

   case PIPE_PRIM_TRIANGLE_STRIP:
      if (count >= 3) {
         flags = DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL;
         idx[1] = GET_ELT(0);
         idx[2] = GET_ELT(1);

         if (last_vertex_last) {
            for (i = 0; i + 2 < count; i++) {
               idx[0] = idx[1];
               idx[1] = idx[2];
               idx[2] = GET_ELT(i + 2);
               /* always emit idx[2] last */
               if (i & 1)
                  TRIANGLE(flags, idx[1], idx[0], idx[2]);
               else
                  TRIANGLE(flags, idx[0], idx[1], idx[2]);
            }
         }
         else {
            for (i = 0; i + 2 < count; i++) {
               idx[0] = idx[1];
               idx[1] = idx[2];
               idx[2] = GET_ELT(i + 2);
               /* always emit idx[0] first */
               if (i & 1)
                  TRIANGLE(flags, idx[0], idx[2], idx[1]);
               else
                  TRIANGLE(flags, idx[0], idx[1], idx[2]);
            }
         }
      }
      break;

   case PIPE_PRIM_TRIANGLE_FAN:
      if (count >= 3) {
         flags = DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL;
         idx[0] = GET_ELT(0);
         idx[2] = GET_ELT(1);

         /* idx[0] is neither the first nor the last vertex */
         if (last_vertex_last) {
            for (i = 0; i + 2 < count; i++) {
               idx[1] = idx[2];
               idx[2] = GET_ELT(i + 2);
               /* always emit idx[2] last */
               TRIANGLE(flags, idx[0], idx[1], idx[2]);
            }
         }
         else {
            for (i = 0; i + 2 < count; i++) {
               idx[1] = idx[2];
               idx[2] = GET_ELT(i + 2);
               /* always emit idx[1] first */
               TRIANGLE(flags, idx[1], idx[2], idx[0]);
            }
         }
      }
      break;

   case PIPE_PRIM_QUADS:
      if (last_vertex_last) {
         for (i = 0; i + 3 < count; i += 4) {
            idx[0] = GET_ELT(i);
            idx[1] = GET_ELT(i + 1);
            idx[2] = GET_ELT(i + 2);
            idx[3] = GET_ELT(i + 3);
#ifdef PASS_QUADS
            QUAD(0, idx[0], idx[1],
                  idx[2], idx[3]);
#else
            flags = DRAW_PIPE_RESET_STIPPLE |
                    DRAW_PIPE_EDGE_FLAG_0 |
                    DRAW_PIPE_EDGE_FLAG_2;
            /* always emit idx[3] last */
            TRIANGLE(flags, idx[0], idx[1], idx[3]);

            flags = DRAW_PIPE_EDGE_FLAG_0 |
                    DRAW_PIPE_EDGE_FLAG_1;
            TRIANGLE(flags, idx[1], idx[2], idx[3]);
#endif
         }
      }
      else {
         for (i = 0; i + 3 < count; i += 4) {
            idx[0] = GET_ELT(i);
            idx[1] = GET_ELT(i + 1);
            idx[2] = GET_ELT(i + 2);
            idx[3] = GET_ELT(i + 3);
#ifdef PASS_QUADS
            QUAD(0, idx[0], idx[1],
                  idx[2], idx[3]);
#else
            flags = DRAW_PIPE_RESET_STIPPLE |
                    DRAW_PIPE_EDGE_FLAG_0 |
                    DRAW_PIPE_EDGE_FLAG_1;
            /* always emit idx[3] / idx[0] first */
            if (quads_flatshade_last)
               TRIANGLE(flags, idx[3], idx[0], idx[1]);
            else
               TRIANGLE(flags, idx[0], idx[1], idx[2]);

            flags = DRAW_PIPE_EDGE_FLAG_1 |
                    DRAW_PIPE_EDGE_FLAG_2;
            if (quads_flatshade_last)
               TRIANGLE(flags, idx[3], idx[1], idx[2]);
            else
               TRIANGLE(flags, idx[0], idx[2], idx[3]);
#endif
         }
      }
      break;

   case PIPE_PRIM_QUAD_STRIP:
      if (count >= 4) {
         idx[2] = GET_ELT(0);
         idx[3] = GET_ELT(1);

         if (last_vertex_last) {
            for (i = 0; i + 3 < count; i += 2) {
               idx[0] = idx[2];
               idx[1] = idx[3];
               idx[2] = GET_ELT(i + 2);
               idx[3] = GET_ELT(i + 3);

#ifdef PASS_QUADS
               QUAD(0, idx[2], idx[0],
                    idx[1], idx[3]);
#else
               /* always emit idx[3] last */
               flags = DRAW_PIPE_RESET_STIPPLE |
                       DRAW_PIPE_EDGE_FLAG_0 |
                       DRAW_PIPE_EDGE_FLAG_2;
               TRIANGLE(flags, idx[2], idx[0], idx[3]);

               flags = DRAW_PIPE_EDGE_FLAG_0 |
                       DRAW_PIPE_EDGE_FLAG_1;
               TRIANGLE(flags, idx[0], idx[1], idx[3]);
#endif
            }
         }
         else {
            for (i = 0; i + 3 < count; i += 2) {
               idx[0] = idx[2];
               idx[1] = idx[3];
               idx[2] = GET_ELT(i + 2);
               idx[3] = GET_ELT(i + 3);

#ifdef PASS_QUADS
               QUAD(0, idx[3], idx[2],
                    idx[0], idx[1]);
#else
               flags = DRAW_PIPE_RESET_STIPPLE |
                       DRAW_PIPE_EDGE_FLAG_0 |
                       DRAW_PIPE_EDGE_FLAG_1;
               /* always emit idx[3] / idx[0 first */
               if (quads_flatshade_last)
                  TRIANGLE(flags, idx[3], idx[2], idx[0]);
               else
                  TRIANGLE(flags, idx[0], idx[3], idx[2]);

               flags = DRAW_PIPE_EDGE_FLAG_1 |
                       DRAW_PIPE_EDGE_FLAG_2;
               if (quads_flatshade_last)
                  TRIANGLE(flags, idx[3], idx[0], idx[1]);
               else
                  TRIANGLE(flags, idx[0], idx[1], idx[3]);
#endif
            }
         }
      }
      break;

   case PIPE_PRIM_POLYGON:
      if (count >= 3) {
         ushort edge_next, edge_finish;

         if (last_vertex_last) {
            flags = (DRAW_PIPE_RESET_STIPPLE |
                     DRAW_PIPE_EDGE_FLAG_0);
            if (!(prim_flags & DRAW_SPLIT_BEFORE))
               flags |= DRAW_PIPE_EDGE_FLAG_2;

            edge_next = DRAW_PIPE_EDGE_FLAG_0;
            edge_finish =
               (prim_flags & DRAW_SPLIT_AFTER) ? 0 : DRAW_PIPE_EDGE_FLAG_1;
         }
         else {
            flags = (DRAW_PIPE_RESET_STIPPLE |
                     DRAW_PIPE_EDGE_FLAG_1);
            if (!(prim_flags & DRAW_SPLIT_BEFORE))
               flags |= DRAW_PIPE_EDGE_FLAG_0;

            edge_next = DRAW_PIPE_EDGE_FLAG_1;
            edge_finish =
               (prim_flags & DRAW_SPLIT_AFTER) ? 0 : DRAW_PIPE_EDGE_FLAG_2;
         }

         idx[0] = GET_ELT(0);
         idx[2] = GET_ELT(1);

         for (i = 0; i + 2 < count; i++, flags = edge_next) {
            idx[1] = idx[2];
            idx[2] = GET_ELT(i + 2);

            if (i + 3 == count)
               flags |= edge_finish;

            /* idx[0] is both the first and the last vertex */
            if (last_vertex_last)
               TRIANGLE(flags, idx[1], idx[2], idx[0]);
            else
               TRIANGLE(flags, idx[0], idx[1], idx[2]);
         }
      }
      break;

   case PIPE_PRIM_LINES_ADJACENCY:
      flags = DRAW_PIPE_RESET_STIPPLE;
      for (i = 0; i + 3 < count; i += 4) {
         idx[0] = GET_ELT(i);
         idx[1] = GET_ELT(i + 1);
         idx[2] = GET_ELT(i + 2);
         idx[3] = GET_ELT(i + 3);
         LINE_ADJ(flags, idx[0], idx[1], idx[2], idx[3]);
      }
      break;

   case PIPE_PRIM_LINE_STRIP_ADJACENCY:
      if (count >= 4) {
         flags = (prim_flags & DRAW_SPLIT_BEFORE) ? 0 : DRAW_PIPE_RESET_STIPPLE;
         idx[1] = GET_ELT(0);
         idx[2] = GET_ELT(1);
         idx[3] = GET_ELT(2);

         for (i = 1; i + 2 < count; i++, flags = 0) {
            idx[0] = idx[1];
            idx[1] = idx[2];
            idx[2] = idx[3];
            idx[3] = GET_ELT(i + 2);
            LINE_ADJ(flags, idx[0], idx[1], idx[2], idx[3]);
         }
      }
      break;

   case PIPE_PRIM_TRIANGLES_ADJACENCY:
      flags = DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL;
      for (i = 0; i + 5 < count; i += 6) {
         idx[0] = GET_ELT(i);
         idx[1] = GET_ELT(i + 1);
         idx[2] = GET_ELT(i + 2);
         idx[3] = GET_ELT(i + 3);
         idx[4] = GET_ELT(i + 4);
         idx[5] = GET_ELT(i + 5);
         TRIANGLE_ADJ(flags, idx[0], idx[1], idx[2], idx[3], idx[4], idx[5]);
      }
      break;

   case PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY:
      if (count >= 6) {
         flags = DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL;
         idx[0] = GET_ELT(1);
         idx[2] = GET_ELT(0);
         idx[4] = GET_ELT(2);
         idx[3] = GET_ELT(4);

         /*
          * The vertices of the i-th triangle are stored in
          * idx[0,2,4] = { 2*i, 2*i+2, 2*i+4 };
          *
          * The adjacent vertices are stored in
          * idx[1,3,5] = { 2*i-2, 2*i+6, 2*i+3 }.
          *
          * However, there are two exceptions:
          *
          * For the first triangle, idx[1] = 1;
          * For the  last triangle, idx[3] = 2*i+5.
          */
         if (last_vertex_last) {
            for (i = 0; i + 5 < count; i += 2) {
               idx[1] = idx[0];

               idx[0] = idx[2];
               idx[2] = idx[4];
               idx[4] = idx[3];

               idx[3] = GET_ELT(i + ((i + 7 < count) ? 6 : 5));
               idx[5] = GET_ELT(i + 3);

               /*
                * alternate the first two vertices (idx[0] and idx[2]) and the
                * corresponding adjacent vertices (idx[3] and idx[5]) to have
                * the correct orientation
                */
               if (i & 2) {
                  TRIANGLE_ADJ(flags,
                        idx[2], idx[1], idx[0], idx[5], idx[4], idx[3]);
               }
               else {
                  TRIANGLE_ADJ(flags,
                        idx[0], idx[1], idx[2], idx[3], idx[4], idx[5]);
               }
            }
         }
         else {
            for (i = 0; i + 5 < count; i += 2) {
               idx[1] = idx[0];

               idx[0] = idx[2];
               idx[2] = idx[4];
               idx[4] = idx[3];

               idx[3] = GET_ELT(i + ((i + 7 < count) ? 6 : 5));
               idx[5] = GET_ELT(i + 3);

               /*
                * alternate the last two vertices (idx[2] and idx[4]) and the
                * corresponding adjacent vertices (idx[1] and idx[5]) to have
                * the correct orientation
                */
               if (i & 2) {
                  TRIANGLE_ADJ(flags,
                        idx[0], idx[5], idx[4], idx[3], idx[2], idx[1]);
               }
               else {
                  TRIANGLE_ADJ(flags,
                        idx[0], idx[1], idx[2], idx[3], idx[4], idx[5]);
               }
            }
         }
      }
      break;

   default:
      assert(0);
      break;
   }

   FUNC_EXIT;
}


#define FUNC_VARS                               \
   struct draw_assembler *asmblr,               \
   const struct draw_prim_info *input_prims,    \
   const struct draw_vertex_info *input_verts,  \
   unsigned start,                              \
   unsigned count

#define FUNC_ENTER                                                \
   /* declare more local vars */                                  \
   const enum pipe_prim_type prim = input_prims->prim;            \
   const unsigned prim_flags = input_prims->flags;                \
   const boolean last_vertex_last = !asmblr->draw->rasterizer->flatshade_first;  \
   switch (prim) {                                                  \
   case PIPE_PRIM_POLYGON:                                          \
      assert(!"unexpected primitive type in prim assembler"); \
      return;                                                       \
   default:                                                         \
      break;                                                        \
   }


#define PASS_QUADS
#define POINT(i0)                             prim_point(asmblr, i0)
#define LINE(flags, i0, i1)                   prim_line(asmblr, i0, i1)
#define TRIANGLE(flags, i0, i1, i2)           prim_tri(asmblr, i0, i1, i2)
#define QUAD(flags, i0, i1, i2, i3)           prim_quad(asmblr, i0, i1, i2, i3)

#include "draw_decompose_tmp.h"



static void
prim_tri(struct draw_assembler *asmblr,
         unsigned i0, unsigned i1, unsigned i2)
{
   unsigned indices[3];

   if (asmblr->needs_primid) {
      inject_primid(asmblr, i0, asmblr->primid);
      inject_primid(asmblr, i1, asmblr->primid);
      inject_primid(asmblr, i2, asmblr->primid++);
   }
   indices[0] = i0;
   indices[1] = i1;
   indices[2] = i2;

   add_prim(asmblr, 3);
   copy_verts(asmblr, indices, 3);
}


```

## 重要结构及纂









