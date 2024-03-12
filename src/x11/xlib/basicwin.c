/* The program will perform these options :
 *
 *  Connect the client to an X server with XOpenDisplay() , and exit gracefully if the connection could not be made.
 *  get information about the physical screen, and use it to calculate the desired size of the window.
 *  create a window with XCreateSimpleWindow()
 *  Set standard properties for the window manager
 *  Select the types of events if needs to receive.
 *  Load the font to be used for printing tex
 *  Creat a graphics context to control the action of drawing requests.
 *  Display the window with XMapWindow.
 *  Loop for events
 *  Respond the the Expose event resulting from mapping the window by calling routines  to draw text and graphics. If the window is too samll to perform its intended function . it will display an appropriate message.
 *  Receive ConfigureNotify events., indicating that the window has been resided by the window manager. The new window size is provided in the event structures.
 *  Keep handling events until a KeyPress or ButtonPress event arribes, then close the display connection and exits.
*/



#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>


#include <stdio.h>
#include <stdlib.h>

#include "bitmaps/icon_bitmap"


int main(int argc, char *argv[])
{
    
    Window win;
    unsigned int width , height;
    int x = 0, y = 0;
    unsigned int border_width = 4;
    unsigned int display_width , display_height; 
    char *window_name = "Basic Window Program";
    char *icon_name = "basicwin";
    Pixmap icon_pixmap;

    XSizeHints *size_hints;
    XClassHint *class_hints;
    XTextProperty windowName, iconName;
    XWMHints* wm_hints;

    Display *display;
    
    int screen_num;
    Screen *screen_ptr;
    GC gc;
    XFontStruct *font_info;
    char *progname = argv[0];
    char *display_name = NULL;

    if ((display = XOpenDisplay(display_name)) == NULL) {

        fprintf( stderr, "%s: cannot connect to X server %s  \n",
                progname, XDisplayName(display_name));
        exit(-1);
        
    }
    //screen_num = XDefaultScreen(display);
    screen_num = DefaultScreen(display);
    screen_ptr = DefaultScreenOfDisplay(display);
    unsigned int screen_count = ScreenCount(display);

    //Display(display);
    display_width = DisplayWidth(display, screen_num); 
    display_height = DisplayHeight(display, screen_num);
    width = display_width / 3;
    height = display_height / 3;
    x = y = 0;

    win = XCreateSimpleWindow( display, RootWindow(display, screen_num), x, y , width, height, border_width, 
            BlackPixel(display, screen_num), WhitePixel(display, screen_num));

    icon_pixmap = XCreateBitmapFromData(display, win, icon_bitmap_bits, icon_bitmap_width, icon_bitmap_height);
   
    if (!(size_hints = XAllocSizeHints())) {
        fprintf(stderr, "%s : failure allocating size memmory" , progname);
        exit(0);
        
    }

    if (!(wm_hints = XAllocWMHints())) {
        fprintf(stderr, "%s : failure allocating vm memmory" , progname);
        exit(0);
    }
    if (!(class_hints = XAllocClassHint())) {
        fprintf(stderr, "%s : failure allocating class  memmory" , progname);
        exit(0);
    }

//    size_hints->flags = PPosition; //| PSize | PMinSize;
    size_hints->flags = PMaxSize | PMinSize; //| PSize | PMinSize;
    size_hints->min_width = 300;
    size_hints->min_height = 200;
    size_hints->max_width = 300;
    size_hints->max_height = 200;
        
    
    //XTextProperty windowName, iconName;
    if(XStringListToTextProperty(&window_name, 1, &windowName) == 0) {
        exit(0);
    }
    wm_hints->initial_state = NormalState;
    wm_hints->input = True;
    wm_hints->icon_pixmap = icon_pixmap;
    wm_hints->flags = StateHint | IconPixmapHint; //| InputHint;

    class_hints->res_name = progname;
    class_hints->res_class = "BasicWin";
    
    //XSetWMProperties(display, win, &windowName, &iconName,
    //        argv, argc, size_hints, wm_hints, class_hints
    //        );


    XSetWMProperties(display, win, &windowName, NULL,
            argv, argc, size_hints, wm_hints, class_hints
            );

   // XSetWMProperties(display, win, &windowName, NULL,
            //NULL, 0, size_hints, NULL, NULL);
    XSelectInput(display, win, ExposureMask | KeyPressMask| ButtonPressMask | StructureNotifyMask);

    XMapWindow(display, win);

    
    return 0;

}
