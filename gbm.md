
```



/drisw.c

/* This is the table of extensions that the loader will dlsym() for. */
const __DRIextension *galliumsw_driver_extensions[] = {
    &driCoreExtension.base,
    &mesaCoreExtension.base,
    &driSWRastExtension.base,
    &driSWCopySubBufferExtension.base,
    &gallium_config_options.base,
    NULL
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



const __DRIswrastExtension driSWRastExtension = {
    .base = { __DRI_SWRAST, 4 },

    .createNewScreen            = driSWRastCreateNewScreen,
    .createNewDrawable          = driCreateNewDrawable,
    .createNewContextForAPI     = driCreateNewContextForAPI,
    .createContextAttribs       = driCreateContextAttribs,
    .createNewScreen2           = driSWRastCreateNewScreen2,
};


/** DRI2 interface */
const __DRIdri2Extension driDRI2Extension = {
    .base = { __DRI_DRI2, 4 },

    .createNewScreen            = dri2CreateNewScreen,
    .createNewDrawable          = driCreateNewDrawable,
    .createNewContext           = driCreateNewContext,
    .getAPIMask                 = driGetAPIMask,
    .createNewContextForAPI     = driCreateNewContextForAPI,
    .allocateBuffer             = dri2AllocateBuffer,
    .releaseBuffer              = dri2ReleaseBuffer,
    .createContextAttribs       = driCreateContextAttribs,
    .createNewScreen2           = driCreateNewScreen2,
};




struct gbm_dri_device {

static struct dri_extension_match gbm_dri_device_extensions[] = {
   { __DRI_CORE, 1, offsetof(struct gbm_dri_device, core), false },
   { __DRI_MESA, 1, offsetof(struct gbm_dri_device, mesa), false },
   { __DRI_DRI2, 4, offsetof(struct gbm_dri_device, dri2), false },
};



static const __DRIextension *gbm_dri_screen_extensions[] = {
   &image_lookup_extension.base,
   &use_invalidate.base,
   &dri2_loader_extension.base,
   &image_loader_extension.base,
   &swrast_loader_extension.base,
   &kopper_loader_extension.base,
   NULL,
};


   dri->driver_extensions = extensions;
   dri->loader_extensions = gbm_dri_screen_extensions;
   dri->screen = dri->mesa->createNewScreen(0, swrast ? -1 : dri->base.v0.fd,
                                            dri->loader_extensions,
                                            dri->driver_extensions,
                                            &dri->driver_configs, dri);


----------------------
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
----------------------------
    screen->loaderPrivate = data;

    /* This will be filled in by mesa->initScreen(). */
    screen->extensions = emptyExtensionList;
    screen->fd = fd;
    screen->myNum = scrn;

 



   dri2_dpy->gbm_dri->lookup_image = dri2_lookup_egl_image;
   dri2_dpy->gbm_dri->validate_image = dri2_validate_egl_image;
   dri2_dpy->gbm_dri->lookup_image_validated = dri2_lookup_egl_image_validated;
   dri2_dpy->gbm_dri->lookup_user_data = disp;

   dri2_dpy->gbm_dri->get_buffers = dri2_drm_get_buffers;
   dri2_dpy->gbm_dri->flush_front_buffer = dri2_drm_flush_front_buffer;
   dri2_dpy->gbm_dri->get_buffers_with_format = dri2_drm_get_buffers_with_format;
   dri2_dpy->gbm_dri->image_get_buffers = dri2_drm_image_get_buffers;
   dri2_dpy->gbm_dri->swrast_put_image2 = swrast_put_image2;
   dri2_dpy->gbm_dri->swrast_get_image = swrast_get_image;

   dri2_dpy->gbm_dri->base.v0.surface_lock_front_buffer = lock_front_buffer;
   dri2_dpy->gbm_dri->base.v0.surface_release_buffer = release_buffer;
   dri2_dpy->gbm_dri->base.v0.surface_has_free_buffers = has_free_buffers;

```


```

static const __DRIconfig **
drisw_init_screen(struct dri_screen *screen)
{
   const __DRIswrastLoaderExtension *loader = screen->swrast_loader;
   const __DRIconfig **configs;
   struct pipe_screen *pscreen = NULL;
   const struct drisw_loader_funcs *lf = &drisw_lf;

#ifdef HAVE_DRISW_KMS
   if (screen->fd != -1)
      success = pipe_loader_sw_probe_kms(&screen->dev, screen->fd);
#endif
   if (!success)
      success = pipe_loader_sw_probe_dri(&screen->dev, lf);
   if (success) {
      pscreen = pipe_loader_create_screen(screen->dev);
      dri_init_options(screen);
   }


    screen->extensions = drisw_screen_extensions;
   screen->lookup_egl_image = dri2_lookup_egl_image;

   const __DRIimageLookupExtension *image = screen->dri2.image;
   if (image &&
       image->base.version >= 2 &&
       image->validateEGLImage &&
       image->lookupEGLImageValidated) {
      screen->validate_egl_image = dri2_validate_egl_image;
      screen->lookup_egl_image_validated = dri2_lookup_egl_image_validated;
   }

   screen->create_drawable = drisw_create_drawable;

}
```


static const __DRIextension *drisw_screen_extensions[] = {
   &driTexBufferExtension.base,
   &dri2RendererQueryExtension.base,
   &dri2ConfigQueryExtension.base,
   &dri2FenceExtension.base,
   &driSWImageExtension.base,
   &dri2FlushControlExtension.base,
   NULL
};

static const struct dri_extension_match optional_core_extensions[] = {
   { __DRI2_CONFIG_QUERY, 1, offsetof(struct dri2_egl_display, config), true },
   { __DRI2_FENCE, 2, offsetof(struct dri2_egl_display, fence), true },
   { __DRI2_BUFFER_DAMAGE, 1, offsetof(struct dri2_egl_display, buffer_damage), true },
   { __DRI2_INTEROP, 1, offsetof(struct dri2_egl_display, interop), true },
   { __DRI_IMAGE, 6, offsetof(struct dri2_egl_display, image), true },
   { __DRI2_FLUSH_CONTROL, 1, offsetof(struct dri2_egl_display, flush_control), true },
   { __DRI2_BLOB, 1, offsetof(struct dri2_egl_display, blob), true },
   { __DRI_MUTABLE_RENDER_BUFFER_DRIVER, 1, offsetof(struct dri2_egl_display, mutable_render_buffer), true },
   { __DRI_KOPPER, 1, offsetof(struct dri2_egl_display, kopper), true },
};



static const struct dri_extension_match dri2_core_extensions[] = {
   { __DRI2_FLUSH, 1, offsetof(struct dri2_egl_display, flush), false },
   { __DRI_TEX_BUFFER, 2, offsetof(struct dri2_egl_display, tex_buffer), false },
   { __DRI_IMAGE, 6, offsetof(struct dri2_egl_display, image), false },
};


static const struct dri_extension_match swrast_core_extensions[] = {
   { __DRI_TEX_BUFFER, 2, offsetof(struct dri2_egl_display, tex_buffer), false },
};


static const struct dri2_egl_display_vtbl dri2_drm_display_vtbl = {
   .authenticate = dri2_drm_authenticate,
   .create_window_surface = dri2_drm_create_window_surface,
   .create_pixmap_surface = dri2_drm_create_pixmap_surface,
   .destroy_surface = dri2_drm_destroy_surface,
   .create_image = dri2_drm_create_image_khr,
   .swap_buffers = dri2_drm_swap_buffers,
   .query_buffer_age = dri2_drm_query_buffer_age,
   .get_dri_drawable = dri2_surface_get_dri_drawable,
};


/**
 * This is the driver specific part of the createNewScreen entry point.
 *
 * Returns the struct gl_config supported by this driver.
 */
static const __DRIconfig **
dri2_init_screen(struct dri_screen *screen)
{
   const __DRIconfig **configs;
   struct pipe_screen *pscreen = NULL;

   (void) mtx_init(&screen->opencl_func_mutex, mtx_plain);

   if (pipe_loader_drm_probe_fd(&screen->dev, screen->fd)) {
      pscreen = pipe_loader_create_screen(screen->dev);
      dri_init_options(screen);
   }

   if (!pscreen)
       goto release_pipe;

   screen->throttle = pscreen->get_param(pscreen, PIPE_CAP_THROTTLE);

   dri2_init_screen_extensions(screen, pscreen, false);

   if (pscreen->get_param(pscreen, PIPE_CAP_DEVICE_PROTECTED_CONTEXT))
      screen->has_protected_context = true;

   configs = dri_init_screen_helper(screen, pscreen);
   if (!configs)
      goto destroy_screen;

   screen->can_share_buffer = true;
   screen->auto_fake_front = dri_with_format(screen);
   screen->lookup_egl_image = dri2_lookup_egl_image;

   const __DRIimageLookupExtension *loader = screen->dri2.image;
   if (loader &&
       loader->base.version >= 2 &&
       loader->validateEGLImage &&
       loader->lookupEGLImageValidated) {
      screen->validate_egl_image = dri2_validate_egl_image;
      screen->lookup_egl_image_validated = dri2_lookup_egl_image_validated;
   }

   screen->create_drawable = dri2_create_drawable;
   screen->allocate_buffer = dri2_allocate_buffer;
   screen->release_buffer = dri2_release_buffer;

   return configs;

destroy_screen:
   dri_destroy_screen_helper(screen);

release_pipe:
   if (screen->dev)
      pipe_loader_release(&screen->dev, 1);

   return NULL;
}


