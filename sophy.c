#include <X11/X.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>

#include "sophy.h" 

XButtonEvent mouse;
Display      *dpy;
Window       root;

typredef struct KeyEvent {
	unsigned int modifier;
	unsigned int key;
	void (*func)(Arg *a);
	Arg arg;
} KeyEvent;

void spawn(Arg, *a);

#include "config.h"

void killclient(Arg *a) {
	XKillClient(info.dsp, info.focused);
}

void spawn(Arg *a) {
	if (fork() == 0) {
		if (execvp(a->v[0], (char**)a->v) == -1) {
			fprintf(stderr, "nigger");
			exit(EXIT_FAILURE);
		}
	}
}

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

/* +-----------------------------------------------------------------------+
   | TODO														     _ o x |
   +-----------------------------------------------------------------------+
   | 1. take refernces of spawning a window form dfpwm					   |
   | 2. add an ability to kill focused window  							   |
   | 3. add an ability to move windows around							   |
   | 4. make focused/unfocused indicators, borders or titlebars 		   |
   | 5. add multiple workspaces                                            |
   | 6. add statusbar with some modules like: clock, workspaces, volume	   |
   +-----------------------------------------------------------------------+ */
