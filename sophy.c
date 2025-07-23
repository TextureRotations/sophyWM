#include <X11/X.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>

#include "sophy.h" 

XButtonEvent mouse;
Display      *dpy;
Window       root;

#include "config.h"

int main(void) {
    if (!(dpy = XOpenDisplay(0))) exit(1);

    root = DefaultRootWindow(dpy);
    
    XSelectInput(dpy, root, SubstructureRedirectMask);
	XDefineCursor(dpy, root, XCreateFontCursor(dpy, 1)); // 68 is default cursor value
    XSync(dpy, 0);

    XEvent e;
    for (;;) {
        XNextEvent(dpy, &e);
        switch (e.type) {
        default:
            puts("Unexpected event.");
            break;
        }
        XSync(dpy, 0);
    }

    XCloseDisplay(dpy);
}
