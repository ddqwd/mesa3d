
 **weston is a server**

#  useful links 

## main page 
https://gitlab.freedesktop.org/wayland/weston

## official doc
https://wayland.pages.freedesktop.org/weston/

### client api 
https://wayland.freedesktop.org/docs/html/apb.html#Client-classwl__display

###  server api 
https://wayland.freedesktop.org/docs/html/apc.html#Server-structwl__display

## books 
https://wayland-book.com/introduction/high-level-design.html


# analysis source code by debugging

## analysis exe
entrypoint : 
frontend/executable.c : main -> frontend/main.c: wet_main() 

1. westong_log_ctx_create
2. designate log file by option -a  
3. wl_display_create
4. load_configuration
加载默认weston_init,可以通过config选项指定
5. weston_compositor_create 
6. load_backends
可以用backend 指定
多backends 通过backends指定
backends有
```c
    WESTON_BACKEND_DRM,
	WESTON_BACKEND_HEADLESS,
	WESTON_BACKEND_PIPEWIRE,
	WESTON_BACKEND_RDP,
	WESTON_BACKEND_VNC,
	WESTON_BACKEND_WAYLAND,
	WESTON_BACKEND_X11,
```
drm:
main.c | load_drm_backend 

1. parse options assoricated with drm in weston.ini 
```c
		{ WESTON_OPTION_STRING, "seat", 0, &config.seat_id },
		{ WESTON_OPTION_STRING, "drm-device", 0, &config.specific_device },
		{ WESTON_OPTION_STRING, "additional-devices", 0, &config.additional_devices},
		{ WESTON_OPTION_BOOLEAN, "current-mode", 0, &wet->drm_use_current_mode },
		{ WESTON_OPTION_BOOLEAN, "use-pixman", 0, &force_pixman },
		{ WESTON_OPTION_BOOLEAN, "continue-without-input", false, &without_input }
```

2. weston_load_module and 获取weston_backend_init符号

加载模块就是dlopen下面对应的库

backend_map[] = {
	[WESTON_BACKEND_DRM] =		"drm-backend.so",
	[WESTON_BACKEND_HEADLESS] =	"headless-backend.so",
	[WESTON_BACKEND_PIPEWIRE] =	"pipewire-backend.so",
	[WESTON_BACKEND_RDP] =		"rdp-backend.so",
	[WESTON_BACKEND_VNC] =		"vnc-backend.so",
	[WESTON_BACKEND_WAYLAND] =	"wayland-backend.so",
	[WESTON_BACKEND_X11] =		"x11-backend.so",
};

3. backend_init 即 weston_backend_init

### 分析drm中 weston_backend_init

1. according environment XDT_SEAT to load seat config  

2. call `udev_new` to create udev

3.  open drm_device

4. init_kms_caps 
获取kms属性

5. 根据renderer 初始化pixman或者egl 

6. weston_setup_vt_switch_bindings

7. drm_backend_create_crtc_list

8. create_sprites 

9. udev_input_init

10. drm_backend_discover_connectors

11. 





headless 
6. 

## analysis libweston 






# drm-backend

// drm.c
weston_backend_init()
	b = drm_backend_create(compositor, &config);




# env 

## server env 

WAYLAND_DEBUG=true  | wl_display_create

// load drm-backend.so PATH
WESTON_MODULE_MAP

// designated seat
drm.c | weston_backend_init |  XDG_SET


//
kms.c | init_kms_caps | 
WESTON_DISABLE_ATOMIC
WESTON_DISABLE_GBM_MODIFIERS
WESTON_FORCE_RENDERER
//
XDG_RUNTIME_DIR 







# steps for client  using wayland to program
分析demo-clients 

1. create wl_display
wl_dispay_connect

2. 

3. 

