
# gbm_create_device

struct gbm_dri_device {
   struct gbm_device base;
      dri->base.fd = fd;
      dri->base.bo_create = gbm_dri_bo_create;
      dri->base.bo_import = gbm_dri_bo_import;
      dri->base.bo_map = gbm_dri_bo_map;
      dri->base.bo_unmap = gbm_dri_bo_unmap;
      dri->base.is_format_supported = gbm_dri_is_format_supported;
      dri->base.get_format_modifier_plane_count =
         gbm_dri_get_format_modifier_plane_count;
      dri->base.bo_write = gbm_dri_bo_write;
      dri->base.bo_get_fd = gbm_dri_bo_get_fd;
      dri->base.bo_get_planes = gbm_dri_bo_get_planes;
      dri->base.bo_get_handle = gbm_dri_bo_get_handle_for_plane;
      dri->base.bo_get_stride = gbm_dri_bo_get_stride;
      dri->base.bo_get_offset = gbm_dri_bo_get_offset;
      dri->base.bo_get_modifier = gbm_dri_bo_get_modifier;
      dri->base.bo_destroy = gbm_dri_bo_destroy;
      dri->base.destroy = dri_destroy;
      dri->base.surface_create = gbm_dri_surface_create;
      dri->base.surface_destroy = gbm_dri_surface_destroy;

      dri->base.name = "drm";

   ///driver_name = loader_get_driver_for_fd(dri->base.fd);
   char *driver_name; /* Name of the DRI module, without the _dri suffix */


   __DRIscreen *screen = driCreateNewScreen2(...);

         psp->driver = globalDriverAPI = galliumdrm_driver_api ;

        const __DRIextension **extensions = dri_screen_extensions;

         const __DRIswrastLoaderExtension *swrast_loader  = swrast_loader_extension ;
         struct {
             const __DRIdri2LoaderExtension *loader = dri2_loader_extension ;
             const __DRIimageLookupExtension *image = image_lookup_extension;
             const __DRIuseInvalidateExtension *useInvalidate = use_invalidate  ;
             const __DRIbackgroundCallableExtension *backgroundCallable;
         } dri2;

         struct {
             const __DRIimageLoaderExtension *loader = image_loader_extension ;
         } image;

         struct {
            const __DRImutableRenderBufferLoaderExtension *loader;
         } mutableRenderBuffer;

        void *driverPrivate = dri_screen ;
        void *loaderPrivate = gbm_dri_device;
        psp->extensions = emptyExtensionList;
        psp->fd = fd;
        psp->myNum = scrn;



   const __DRIconfig   **driver_configs = dri_init_screen_helper() ;
   const __DRIcoreExtension   *core = driCoreExtension;
   const __DRIdri2Extension   *dri2 =  driDRI2Extension ;

   const __DRI2fenceExtension *fence = dri2FenceExtension

   const __DRIimageExtension  *image = dri2ImageExtension 
   const __DRIswrastExtension *swrast;
   const __DRI2flushExtension *flush = dri2FlushExtension 

   //dri->loader_extensions = gbm_dri_screen_extensions;
   const __DRIextension **loader_extensions;

   //dri->driver_extensions = galliumdrm_driver_extensions ;
   const __DRIextension **driver_extensions;

   const struct gbm_dri_visual *visual_table = gbm_dri_visuals_table;

   //dri->num_visuals = ARRAY_SIZE(gbm_dri_visuals_table);
   int num_visuals = ;
};




struct dri_screen
{
   /* st_api */
   struct st_manager base;
        screen = pipe_screen 
        get_egl_image = dri_get_egl_image
        get_param = dri_get_param;
        set_background_context = dri_set_background_context;
        
   struct st_api *st_api = st_gl_api;


   /* dri */
   __DRIscreen *sPriv = psp;


   struct st_config_options options;

   /* drm */
   int fd = psp->fd ;
   boolean can_share_buffer = true;

   struct pipe_loader_device *dev;

   /* gallium */
   boolean d_depth_bits_last;
   boolean sd_depth_bits_last;
   boolean auto_fake_front;
   boolean has_reset_status_query;
   enum pipe_texture_target target;

   boolean swrast_no_present;

   /* hooks filled in by dri2 & drisw */
   //screen->lookup_egl_image = dri2_lookup_egl_image;
   __DRIimage * (*lookup_egl_image)(struct dri_screen *ctx, void *handle);

};


typedef struct __DRIextensionRec		__DRIextension;
struct __DRIextensionRec {
    const char *name;
    int version;
};

static struct gbm_device *
dri_device_create(int fd)
   dri = calloc(1, sizeof *dri);
   dri.base.fd = ...
   ret = dri_screen_create(dri);
        driver_name = loader_get_driver_for_fd(dri->base.fd);
        return dri_screen_create_dri2(dri, driver_name);
                ret = dri_load_driver(dri);
                        const __DRIextension **extensions;
                        extensions = dri_open_driver(dri);
                            get_extensions_name = loader_get_extensions_name(dri->driver_name);
                                //#define __DRI_DRIVER_GET_EXTENSIONS "__driDriverGetExtensions"
                                if (asprintf(&name, "%s_%s", __DRI_DRIVER_GET_EXTENSIONS, driver_name) < 0)
                            get_extensions = dlsym(dri->driver, get_extensions_name);
                            extensions = get_extensions();

                        // core dri2
                        if (dri_bind_extensions(dri, gbm_dri_device_extensions, extensions) < 0) {
                        dri->driver_extensions = extensions;

                dri->screen = dri->dri2->createNewScreen2(0, dri->base.fd,
                                                dri->loader_extensions,
                                                dri->driver_extensions,
                                                &dri->driver_configs, dri);
                    __DRIscreen *psp;
                    psp = calloc(1, sizeof(*psp));
                    psp->driver = globalDriverAPI;

                    //#define __DRI_DRIVER_VTABLE "DRI_DriverVtable"
                    if (strcmp(driver_extensions[i]->name, __DRI_DRIVER_VTABLE) == 0) {
                        psp->driver =
                            ((__DRIDriverVtableExtension *)driver_extensions[i])->vtable;
                    }


                    setupLoaderExtensions(psp, extensions);
                    *driver_configs = psp->driver->InitScreen(psp);
                        -> dri2_init_screen



                //-> driGetExtensions 
                // dri_screen_extensions
                extensions = dri->core->getExtensions(dri->screen);

                // dri2FenceExtension
                // dri2ImageExtension
                // dri2FlushExtension
                if (dri_bind_extensions(dri, dri_core_extensions, extensions) < 0) {

                dri->lookup_image = NULL;
                dri->lookup_user_data = NULL;


dri2_init_screen(__DRIscreen * sPriv)
   const __DRIconfig **configs;
   struct dri_screen *screen;
   struct pipe_screen *pscreen = NULL;
   const struct drm_conf_ret *throttle_ret;
   const struct drm_conf_ret *dmabuf_ret;

   screen = CALLOC_STRUCT(dri_screen);
   if (!screen)
      return NULL;

   screen->sPriv = sPriv;
   screen->fd = sPriv->fd;
   sPriv->driverPrivate = (void *)screen;

   if (pipe_loader_drm_probe_fd(&screen->dev, screen->fd)) {
      dri_init_options(screen);

      pscreen = pipe_loader_create_screen(screen->dev);
            pipe_loader_drm_create_screen,
                pipe_radeonsi_create_screen(int fd, const struct pipe_screen_config *config)
   }


   sPriv->extensions = dri_screen_extensions;

   configs = dri_init_screen_helper(screen, pscreen);

   screen->can_share_buffer = true;
   screen->auto_fake_front = dri_with_format(sPriv);
   screen->broken_invalidate = !sPriv->dri2.useInvalidate;
   screen->lookup_egl_image = dri2_lookup_egl_image;

   return configs;




# eglGetPlatformDisplay

struct _egl_display: 
   _EGLPlatformType Platform; /**< The type of the platform display */
   void *PlatformDisplay;     /**< A pointer to the platform display */

_eglGetPlatformDisplayCommon(EGLenum platform, void *native_display,
      dpy = _eglGetGbmDisplay((struct gbm_device*) native_display,
                              attrib_list);
            return _eglFindDisplay(_EGL_PLATFORM_DRM, native_display);
                 /* create a new display */
                   if (!dpy) {
                      dpy = calloc(1, sizeof(_EGLDisplay));
                      dpy->Platform = plat;
                      dpy->PlatformDisplay = plat_dpy;
                      dpy->Next = _eglGlobal.DisplayList;
                      _eglGlobal.DisplayList = dpy;
                   }

   
# eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor)

struct _egl_driver
{
   _EGLAPI API;  /**< EGL API dispatch table */
};

      if (!_eglMatchDriver(disp))
            _EGLDriver *best_drv;
            best_drv = _eglMatchAndInitialize(dpy);
                _eglGetDriver()
                    _eglDriver = calloc(1, sizeof(*_eglDriver));
                    _eglInitDriverFallbacks(_eglDriver);
                           /* the driver has to implement these */
                           drv->API.Initialize = NULL;
                           drv->API.Terminate = NULL;
                           drv->API.GetConfigs = _eglGetConfigs;

                    _eglInitDriver(_eglDriver);
                       dri2_drv->API.Initialize = dri2_initialize;
                       dri2_drv->API.Terminate = dri2_terminate;
                       dri2_drv->API.CreateContext = dri2_create_context;
 
                _eglDriver->API.Initialize(_eglDriver, dpy)
                    dri2_initialize







struct _egl_display:
   void *DriverData;          /**< Driver private data */
   _EGLDevice *Device;        /**< Device backing the display */


struct _egl_device {
   _EGLDevice *Next;

   const char *extensions;

   EGLBoolean MESA_device_software;
   EGLBoolean EXT_device_drm;

#ifdef HAVE_LIBDRM
   drmDevicePtr device;
#endif
};


struct dri2_egl_display
{

   const struct dri2_egl_display_vtbl *vtbl = &dri2_drm_display_vtbl;

   __DRIscreen              *dri_screen = gbm_dri->screen;
   const __DRIconfig       **driver_configs = gbm_dri->driver_configs;

   const __DRIcoreExtension       *core = gbm_dri->core;
   const __DRIimageDriverExtension *image_driver;
   const __DRIdri2Extension       *dri2 = gbm_dri->dri2;
   const __DRIswrastExtension     *swrast = gbm_dri->swrast;

   const __DRI2flushExtension     *flush  = dri2FlushExtension ;
   const __DRItexBufferExtension  *tex_buffer = driTexBufferExtension;
   const __DRIimageExtension      *image = dri2ImageExtension ;

   struct gbm_dri_device    *gbm_dri;
        lookup_image = dri2_lookup_egl_image
        dri2_dpy->gbm_dri->lookup_user_data = disp;
        dri2_dpy->gbm_dri->get_buffers = dri2_drm_get_buffers;
        dri2_dpy->gbm_dri->flush_front_buffer = dri2_drm_flush_front_buffer;
        dri2_dpy->gbm_dri->get_buffers_with_format = dri2_drm_get_buffers_with_format;
        dri2_dpy->gbm_dri->image_get_buffers = dri2_drm_image_get_buffers;
        dri2_dpy->gbm_dri->swrast_put_image2 = swrast_put_image2;
        dri2_dpy->gbm_dri->swrast_get_image = swrast_get_image;

        dri2_dpy->gbm_dri->base.surface_lock_front_buffer = lock_front_buffer;
        dri2_dpy->gbm_dri->base.surface_release_buffer = release_buffer;
        dri2_dpy->gbm_dri->base.surface_has_free_buffers = has_free_buffers;


   char                     *driver_name = gbm_dri->driver_name;


dri2_initialize(_EGLDriver *drv, _EGLDisplay *disp)
    // disp-> DriverData = dri2_egl_display
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   if (dri2_dpy is null):
   case _EGL_PLATFORM_DRM:
      ret = dri2_initialize_drm(drv, disp);
            dri2_dpy = calloc(1, sizeof *dri2_dpy);
            dri2_dpy->fd = -1;
            disp->DriverData = (void *) dri2_dpy;
            dri2_dpy->fd = fcntl(gbm_device_get_fd(gbm), F_DUPFD_CLOEXEC, 3);

            dev = _eglAddDevice(dri2_dpy->fd, false);
            disp->Device = dev;
            ...

            // flush texbuffer image
            if (!dri2_setup_extensions(disp)) {


            // fill disp->Extensions.EXT_image_dma_buf_import_modifiers = EGL_TRUE;
            dri2_setup_screen(disp);


            dri2_dpy->vtbl = &dri2_drm_display_vtbl;


# eglCreateContext


struct dri2_egl_context
{
   _EGLContext   base;
   __DRIcontext *dri_context;
        void *driverPrivate = dri2_egl_context;
        void *loaderPrivate;

        // binded when eglMakeCurrent is called, 
        context->driDrawablePriv = NULL;
        context->driReadablePriv = NULL;
        __DRIscreen *driScreenPriv = dri2_dpy->dri_screen;
};




context = drv->API.CreateContext(drv, disp, conf, share, attrib_list);
dri2_create_context
   dri2_ctx = malloc(sizeof *dri2_ctx);
   if (!_eglInitContext(&dri2_ctx->base, disp, conf, attrib_list))

   if (!dri2_fill_context_attribs(dri2_ctx, dri2_dpy, ctx_attribs,
     (dri2_dpy->dri2) {
      if (dri2_dpy->dri2->base.version >= 3) {
		// driDRI2Extension 
		// driCreateNewContext
         dri2_ctx->dri_context =
            dri2_dpy->dri2->createContextAttribs(dri2_dpy->dri_screen,
                                                 api,
                                                 dri_config,
                                                 shared,
                                                 num_attribs / 2,
                                                 ctx_attribs,
                                                 & error,
                                                 dri2_ctx);
                    driCreateContextAttribs
                        __DRIcontext *context;
                       context = calloc(1, sizeof *context);
                       context->loaderPrivate = data;
                   
                       context->driScreenPriv = screen;
                       context->driDrawablePriv = NULL;
                       context->driReadablePriv = NULL;
                   
                   	// galliumdrm_driver_api 
                   	// dri_create_context
                       if (!screen->driver->CreateContext(mesa_api, modes, context,
                                                          &ctx_config, error, shareCtx)) {
                       }



         dri2_create_context_attribs_error(error);



GLboolean
dri_create_context(gl_api api, const struct gl_config * visual, __DRIcontext * cPriv,
                   const struct __DriverContextConfig *ctx_config,
                   unsigned *error,
                   void *sharedContextPrivate)
{
 
   struct dri_context *ctx = NULL;

   ctx = CALLOC_STRUCT(dri_context);

   cPriv->driverPrivate = ctx;
   ctx->cPriv = cPriv;
   ctx->sPriv = sPriv;

   if (driQueryOptionb(&screen->dev->option_cache, "mesa_no_error"))
      attribs.flags |= ST_CONTEXT_FLAG_NO_ERROR;

   attribs.options = screen->options;
   dri_fill_st_visual(&attribs.visual, screen, visual);

   // st_gl_api 
   //st_api_create_context
   ctx->st = stapi->create_context(stapi, &screen->base, &attribs, &ctx_err,
				   st_share);

        struct st_context *st;
        struct pipe_context *pipe;

        //  pipe_screen si_pipe_create_context
        pipe = smapi->screen->context_create(smapi->screen, NULL, ctx_flags);

        st = st_create_context(api, pipe, mode_ptr, shared_ctx,
                          &attribs->options, no_error);
            struct gl_context *ctx;

            st_init_driver_functions(pipe->screen, &funcs);

            ctx = calloc(1, sizeof(struct gl_context));

        st_create_context_priv(struct gl_context *ctx, struct pipe_context *pipe,


# gbm_surface_create(gbm, wid, hei, format, flags)

struct gbm_dri_surface {
   struct gbm_surface base;
        struct gbm_device *gbm = gbm
        uint32_t width = width ;
        uint32_t height = height;
        uint32_t format = gbm_format_canonicalize(format);
        uint32_t flags = flags ;
        struct {
           uint64_t *modifiers; surf->base.modifiers = calloc(count, sizeof(*modifiers));
           unsigned count;
        };


   void *dri_private= dri2_surf ;
};


gbm_dri_surface_create
   surf = calloc(1, sizeof *surf);

# eglCreatePlatformWindowSurface

struct dri2_egl_surface
{
   _EGLSurface          base;
        EGLint Width, Height = gbm_surface->wid, hei ;
   __DRIdrawable       *dri_drawable;
        void *driverPrivate = dri_drawable;

-------------------------------------------------------
struct dri_drawable
{
   struct st_framebuffer_iface base;
        _framebuffer_iface */
       drawable->base.visual = &drawable->stvis;
       drawable->base.flush_front = dri_st_framebuffer_flush_front;
       drawable->base.validate = dri_st_framebuffer_validate;
       drawable->base.flush_swapbuffers = dri_st_framebuffer_flush_swapbuffers;
       drawable->base.st_manager_private = (void *) drawable;
       drawable->base.ID = p_atomic_inc_return(&drifb_ID);
       drawable->base.state_manager = &screen->base;

   struct st_visual stvis;

   struct dri_screen *screen = egl_dpy->dri_screen ;

   /* dri */
   __DRIdrawable *dPriv;
   __DRIscreen *sPriv;

   struct pipe_resource *textures[ST_ATTACHMENT_COUNT];
   struct pipe_resource *msaa_textures[ST_ATTACHMENT_COUNT];
   unsigned int texture_mask, texture_stamp;

   struct pipe_fence_handle *swap_fences[DRI_SWAP_FENCES_MAX];
   unsigned int cur_fences;
   unsigned int head;
   unsigned int tail;
   unsigned int desired_fences;
   boolean flushing; /* prevents recursion in dri_flush */

   /* used only by DRISW */
   struct pipe_surface *drisw_surface;

   (void*)allocate_textures = dri2_allocate_textures;
   (void*)flush_frontbuffer = dri2_flush_frontbuffer;
   (void*)update_tex_buffer = dri2_update_tex_buffer;
   (void*)flush_swapbuffers = dri2_flush_swapbuffers;

};

----------------------------------------------------------------
        void *loaderPrivate = data = gbm_surface;
        __DRIscreen *driScreenPriv = screen;

        // binded when eglMakeCurrent is called 
        __DRIcontext *driContextPriv = NULL;
        pdraw->refcount = 0;
        pdraw->lastStamp = 0;
        pdraw->w = 0;
        pdraw->h = 0;



   __DRIbuffer          buffers[5];
   bool                 have_fake_front;

#ifdef HAVE_X11_PLATFORM
   xcb_drawable_t       drawable;
   xcb_xfixes_region_t  region;
   int                  depth;
   int                  bytes_per_pixel;
   xcb_gcontext_t       gc;
   xcb_gcontext_t       swapgc;
#endif

#ifdef HAVE_WAYLAND_PLATFORM
   struct wl_egl_window  *wl_win;
   int                    dx;
   int                    dy;
   struct wl_event_queue *wl_queue;
   struct wl_surface     *wl_surface_wrapper;
   struct wl_display     *wl_dpy_wrapper;
   struct wl_drm         *wl_drm_wrapper;
   struct wl_callback    *throttle_callback;
   int                    format;
#endif

#ifdef HAVE_DRM_PLATFORM
   struct gbm_dri_surface *gbm_surf = gbm_surf;
        dri_private = dri2_surf
#endif

   /* EGL-owned buffers */
   __DRIbuffer           *local_buffers[__DRI_BUFFER_COUNT];

#if defined(HAVE_WAYLAND_PLATFORM) || defined(HAVE_DRM_PLATFORM)
   struct {
#ifdef HAVE_DRM_PLATFORM
      struct gbm_bo       *bo = gbm_bo_create();
#endif
      bool                locked;
      int                 age;
   } color_buffers[4], *back, *current;
#endif

#ifdef HAVE_ANDROID_PLATFORM
   struct ANativeWindow *window;
   struct ANativeWindowBuffer *buffer;
   __DRIimage *dri_image_back;
   __DRIimage *dri_image_front;

   /* Used to record all the buffers created by ANativeWindow and their ages.
    * Usually Android uses at most triple buffers in ANativeWindow
    * so hardcode the number of color_buffers to 3.
    */
   struct {
      struct ANativeWindowBuffer *buffer;
      int age;
   } color_buffers[3], *back;
#endif

#if defined(HAVE_SURFACELESS_PLATFORM)
      __DRIimage           *front;
      unsigned int         visual;
#endif
   int out_fence_fd;
   EGLBoolean enable_out_fence;
};




eglCreatePlatformWindowSurface(EGLDisplay dpy, EGLConfig config,
                               void *native_window,
                               const EGLAttrib *attrib_list)

   surface = _eglCreateWindowSurfaceCommon(disp, config, native_window,
                                           int_attribs);
        _EGLSurface *surf;
        //dri2_create_window_surface
        surf = drv->API.CreateWindowSurface(drv, disp, conf, native_window,
                                       attrib_list);
            // dri2_drm_display_vtbl dri2_drm_create_window_surface
            return dri2_dpy->vtbl->create_window_surface(drv, dpy, conf, native_window,
                                                attrib_list);

static _EGLSurface *
dri2_drm_create_window_surface(_EGLDriver *drv, _EGLDisplay *disp,
                               _EGLConfig *conf, void *native_surface,
                               const EGLint *attrib_list)
{

   struct dri2_egl_surface *dri2_surf;

   dri2_surf = calloc(1, sizeof *dri2_surf);

    // intialize _EGLSurface
   if (!dri2_init_surface(&dri2_surf->base, disp, EGL_WINDOW_BIT, conf,
                          attrib_list, false))

   config = dri2_get_dri_config(dri2_conf, EGL_WINDOW_BIT,
                                dri2_surf->base.GLColorspace);

   surf = gbm_dri_surface(surface);
   dri2_surf->gbm_surf = surf;
   dri2_surf->base.Width =  surf->base.width;
   dri2_surf->base.Height = surf->base.height;
   surf->dri_private = dri2_surf;

   if (dri2_dpy->dri2) {
		// driDRI2Extension 
		//driCreateNewDrawable
      dri2_surf->dri_drawable =
         dri2_dpy->dri2->createNewDrawable(dri2_dpy->dri_screen, config,
                                           dri2_surf->gbm_surf);

                __DRIdrawable *pdraw;
   
                pdraw = malloc(sizeof *pdraw);

                pdraw->loaderPrivate = data;
            
                pdraw->driScreenPriv = screen;
                pdraw->driContextPriv = NULL;
                pdraw->refcount = 0;
                pdraw->lastStamp = 0;
                pdraw->w = 0;
                pdraw->h = 0;
            
                dri_get_drawable(pdraw);
            
	            //galliumdrm_driver_api 
	            //dri2_create_buffer
                if (!screen->driver->CreateBuffer(screen, pdraw, &config->modes,
                                                  GL_FALSE)) {
                }
            

static boolean
dri2_create_buffer(__DRIscreen * sPriv,
                   __DRIdrawable * dPriv,
                   const struct gl_config * visual, boolean isPixmap)
{

   struct dri_drawable *drawable = NULL;
   if (!dri_create_buffer(sPriv, dPriv, visual, isPixmap))
        struct dri_screen *screen = sPriv->driverPrivate;
        drawable = CALLOC_STRUCT(dri_drawable);
   struct dri_drawable *drawable = NULL;
   drawable = dPriv->driverPrivate;

   drawable->allocate_textures = dri2_allocate_textures;
   drawable->flush_frontbuffer = dri2_flush_frontbuffer;
   drawable->update_tex_buffer = dri2_update_tex_buffer;
   drawable->flush_swapbuffers = dri2_flush_swapbuffers;



# eglMakeCurrent

eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read,
               EGLContext ctx)
    // error checked 
    ...

    // dri2_make_current
   ret = drv->API.MakeCurrent(drv, disp, draw_surf, read_surf, context);
        __DRIdrawable *ddraw, *rdraw;
        ddraw = (dsurf) ? dri2_dpy->vtbl->get_dri_drawable(dsurf) : NULL;
        rdraw = (rsurf) ? dri2_dpy->vtbl->get_dri_drawable(rsurf) : NULL;

       	// driCoreExtension
    	//driBindContext
        if (!unbind && !dri2_dpy->core->bindContext(cctx, ddraw, rdraw)) {
            /* Bind the drawable to the context */
            pcp->driDrawablePriv = pdp;
            pcp->driReadablePriv = prp;
        	pdp->driContextPriv = pcp;

	            //galliumdrm_driver_api 
				//dri_make_current
            return pcp->driScreenPriv->driver->MakeCurrent(pcp, pdp, prp);

        }

GLboolean
dri_make_current(__DRIcontext * cPriv,
		 __DRIdrawable * driDrawPriv,
		 __DRIdrawable * driReadPriv)
{
   /* dri_util.c ensures cPriv is not null */
   struct dri_context *ctx = dri_context(cPriv);
   struct dri_drawable *draw = dri_drawable(driDrawPriv);
   struct dri_drawable *read = dri_drawable(driReadPriv);

   ++ctx->bind_count;

   if (!draw && !read)
      return ctx->stapi->make_current(ctx->stapi, ctx->st, NULL, NULL);
   else if (!draw || !read)
      return GL_FALSE;

   if (ctx->dPriv != driDrawPriv) {
      ctx->dPriv = driDrawPriv;
      draw->texture_stamp = driDrawPriv->lastStamp - 1;
   }
   if (ctx->rPriv != driReadPriv) {
      ctx->rPriv = driReadPriv;
      read->texture_stamp = driReadPriv->lastStamp - 1;
   }

// st_api_make_current
   ctx->stapi->make_current(ctx->stapi, ctx->st, &draw->base, &read->base);

   return GL_TRUE;
}





------------------------------------------- st_api_make_current begin 




static boolean
st_api_make_current(struct st_api *stapi, struct st_context_iface *stctxi,
                    struct st_framebuffer_iface *stdrawi,
                    struct st_framebuffer_iface *streadi)
{
   struct st_context *st = (struct st_context *) stctxi;
   struct st_framebuffer *stdraw, *stread;
   boolean ret;

   if (st) {
      /* reuse or create the draw fb */
      stdraw = st_framebuffer_reuse_or_create(st,
            st->ctx->WinSysDrawBuffer, stdrawi);
            cur = st_framebuffer_create(st, stfbi);
      if (streadi != stdrawi) {
         /* do the same for the read fb */
         stread = st_framebuffer_reuse_or_create(st,
               st->ctx->WinSysReadBuffer, streadi);
      }
      else {
         stread = NULL;
         /* reuse the draw fb for the read fb */
         if (stdraw)
            st_framebuffer_reference(&stread, stdraw);
      }

      if (stdraw && stread) {
         st_framebuffer_validate(stdraw, st);

                // dri_st_framebuffer_validate
                if (!stfb->iface->validate(&st->iface, stfb->iface, stfb->statts,
                    // dri2_allocate_textures
                         if (image) {
                            if (!dri_image_drawable_get_buffers(drawable, &images,
                                                                statts, statts_count))
                                    dri_drawable_get_format(drawable, statts[i], &pf, &bind);
                                    //image_loader_extension 
                                    // image_get_buffers
                                    return (*sPriv->image.loader->getBuffers) (dPriv, image_format,
                                                                        (uint32_t *) &drawable->base.stamp,
                                                                        dPriv->loaderPrivate, buffer_mask,
                                                                        images);
                               return;
                         }


         if (stread != stdraw)
            st_framebuffer_validate(stread, st);

         ret = _mesa_make_current(st->ctx, &stdraw->Base, &stread->Base);

         st->draw_stamp = stdraw->stamp - 1;
         st->read_stamp = stread->stamp - 1;
         st_context_validate(st, stdraw, stread);
      }
      else {
         struct gl_framebuffer *incomplete = _mesa_get_incomplete_framebuffer();
         ret = _mesa_make_current(st->ctx, incomplete, incomplete);
      }

      st_framebuffer_reference(&stdraw, NULL);
      st_framebuffer_reference(&stread, NULL);

      /* Purge the context's winsys_buffers list in case any
       * of the referenced drawables no longer exist.
       */
      st_framebuffers_purge(st);
   }
   else {
      GET_CURRENT_CONTEXT(ctx);

      ret = _mesa_make_current(NULL, NULL, NULL);

      if (ctx)
         st_framebuffers_purge(ctx->st);
   }

   return ret;
}

static int
image_get_buffers(__DRIdrawable *driDrawable,
                  unsigned int format,
                  uint32_t *stamp,
                  void *loaderPrivate,
                  uint32_t buffer_mask,
                  struct __DRIimageList *buffers)
{
   struct gbm_dri_surface *surf = loaderPrivate;
   struct gbm_dri_device *dri = gbm_dri_device(surf->base.gbm);

   if (dri->image_get_buffers == NULL)
      return 0;

// dri2_drm_image_get_buffers
   return dri->image_get_buffers(driDrawable, format, stamp,
                                 surf->dri_private, buffer_mask, buffers);
       struct dri2_egl_surface *dri2_surf = loaderPrivate;
       struct gbm_dri_bo *bo;
    
       if (get_back_bo(dri2_surf) < 0)
                 dri2_surf->back->bo = gbm_bo_create(&dri2_dpy->gbm_dri->base,
                                                     surf->base.width,
                                                     surf->base.height,
                                                     surf->base.format,
                                                     surf->base.flags);
          return 0;
    
       bo = gbm_dri_bo(dri2_surf->back->bo);
       buffers->image_mask = __DRI_IMAGE_BUFFER_BACK;
       buffers->back = bo->image;
    
       return 1;

}












# gbm_bo_create


struct gbm_dri_bo {
   struct gbm_bo base;
       bo->base.gbm = gbm;
       bo->base.width = width;
       bo->base.height = height;
       bo->base.format = gbm_format_canonicalize(format);



   __DRIimage *image;
        struct pipe_resource *texture = si_alloc_resouce_create
        unsigned level = 0;
        unsigned layer = 0 ;
        uint32_t dri_format;
        uint32_t dri_components;
        unsigned use  = __DRI_IMAGE_USE_SCANOUT | ..  ;

        void *loader_private = gbm_dri_bo ;

        /**
         * Provided by EGL_EXT_image_dma_buf_import.
         */
        enum __DRIYUVColorSpace yuv_color_space;
        enum __DRISampleRange sample_range;
        enum __DRIChromaSiting horizontal_siting;
        enum __DRIChromaSiting vertical_siting;


        

   /* Used for cursors and the swrast front BO */
   uint32_t handle, size;
   void *map;
};



		// dri2ImageExtension 
		//dri2_create_image
      bo->image = dri->image->createImage(dri->screen, width, height,
                                          dri_format, dri_use, bo);
           struct dri_screen *screen = dri_screen(_screen);
           __DRIimage *img;
           struct pipe_resource templ;
           unsigned tex_usage;
           enum pipe_format pf;
        
           /* createImageWithModifiers doesn't supply usage, and we should not get
            * here with both modifiers and a usage flag.
            */
           assert(!(use && (modifiers != NULL)));
        
           tex_usage = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;
        
           if (use & __DRI_IMAGE_USE_SCANOUT)
              tex_usage |= PIPE_BIND_SCANOUT;
           if (use & __DRI_IMAGE_USE_SHARE)
              tex_usage |= PIPE_BIND_SHARED;
           if (use & __DRI_IMAGE_USE_LINEAR)
              tex_usage |= PIPE_BIND_LINEAR;
           if (use & __DRI_IMAGE_USE_CURSOR) {
              if (width != 64 || height != 64)
                 return NULL;
              tex_usage |= PIPE_BIND_CURSOR;
           }
        
           pf = dri2_format_to_pipe_format (format);
           if (pf == PIPE_FORMAT_NONE)
              return NULL;
        
           img = CALLOC_STRUCT(__DRIimageRec);
           if (!img)
              return NULL;
        
           memset(&templ, 0, sizeof(templ));
           templ.bind = tex_usage;
           templ.format = pf;
           templ.target = PIPE_TEXTURE_2D;
           templ.last_level = 0;
           templ.width0 = width;
           templ.height0 = height;
           templ.depth0 = 1;
           templ.array_size = 1;
        
           if (modifiers)
              img->texture =
                 screen->base.screen
                    ->resource_create_with_modifiers(screen->base.screen,
                                                     &templ,
                                                     modifiers,
                                                     count);
           else
              img->texture =
                 screen->base.screen->resource_create(screen->base.screen, &templ);
        
           img->level = 0;
           img->layer = 0;
           img->dri_format = format;
           img->dri_components = 0;
           img->use = use;
        
           img->loader_private = loaderPrivate;
           return img;








----------------------------------------


# eglCreateImage

   img = drv->API.CreateImageKHR(drv, disp, context, target,
                                 buffer, attr_list);

        struct dri2_egl_display *dri2_dpy = dri2_egl_display(dpy);

        // dri2_drm_display_vtbl
        //dri2_drm_create_image_khr
        return dri2_dpy->vtbl->create_image(drv, dpy, ctx, target, buffer,
            #ifdef HAVE_LIBDRM
               case EGL_DRM_BUFFER_MESA:
                  return dri2_create_image_mesa_drm_buffer(disp, ctx, buffer, attr_list);
               case EGL_LINUX_DMA_BUF_EXT:
                  return dri2_create_image_dma_buf(disp, ctx, buffer, attr_list);
            #endif
