


__glXInitialize _(Display *dpy)

{
   struct glx_display *dpyPriv, *d;
   dpyPriv = calloc(1, sizeof *dpyPriv);

    // initialize glx extension
   codes = XInitExtension(dpy, __glXExtensionName);

    // add pro function for event
    XESetWireToEvent(dpy, dpyPriv->codes.first_event + i, __glXWireToEvent);

    // set close proc function 
   XESetCloseDisplay(dpy, dpyPriv->codes.extension, __glXCloseDisplay);
   XESetErrorString (dpy, dpyPriv->codes.extension, __glXErrorString);
   
   // first try dri3  , then dri2 
   dpyPriv->dri3Display = dri3_create_display(dpy);
}



dri3_create_display(Display * dpy)
{
   xcb_connection_t                     *c = XGetXCBConnection(dpy);

}


## environment variables

LIBGL_DRI3_DISABLE
LIBGL_DIR2_DISABLE

# XCB 

## X Connection 
the major notion of using XCB  is the X  connection . It is analogous to the Xlibh Display . When we open a connection to an X server . the library 
returns 

## Requests and replise : the Xlib killers

## The Graphic Context 

In order to avoid the need to supply of hundreds of prameters to each drawing function , a graphical context structure is used.  We set the various drawing options in this structure:


