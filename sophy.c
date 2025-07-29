/* TODO: 1. focus
 *		 2. kill
 *		 3. move
 */

#include <X11/keysym.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <X11/Xutil.h>

typedef struct Arg {
    char **v;
} Arg;

typedef struct KeyEvent {
    unsigned int modifier;
    KeySym key;
    void (*func)(Arg *a);
    Arg arg;
} KeyEvent;

XButtonEvent mouse;
Display      *dpy;
Window       root;

void focus(Arg *a);
void move(Arg *a);
void grab_keys(void);
void kill(Arg *a);
void spawn(Arg *a);

#include "config.h"

void focus(Arg *a) {

}

void move(Arg *a) {
	
}

void grab_keys(void) {
    for (unsigned int i = 0; i < sizeof(keys)/sizeof(keys[0]); i++) {
        KeyCode keycode = XKeysymToKeycode(dpy, keys[i].key);
        XGrabKey(dpy, keycode, keys[i].modifier, root, True,
                GrabModeAsync, GrabModeAsync);
    }
}

void kill(Arg *a) {
	
}

void spawn(Arg *a) {
	int pid = fork();
	fprintf(stderr, "%d\n", pid);
		
	execvp((char*)a->v[0], (char**)a->v);
	fprintf(stderr, "event called\n");
}

int main(void) {
    if (!(dpy = XOpenDisplay(0))) exit(1);

    root = DefaultRootWindow(dpy);
    
    XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask);
    XDefineCursor(dpy, root, XCreateFontCursor(dpy, 68));  // Default cursor value is 68
    grab_keys();
    XSync(dpy, False);

    XEvent ev;
    while (1) {
        XNextEvent(dpy, &ev);
        switch (ev.type) {
        case KeyPress: {
            XKeyEvent *e = &ev.xkey;
            for (unsigned int i = 0; i < sizeof(keys)/sizeof(keys[0]); i++) {
                KeyCode keycode = XKeysymToKeycode(dpy, keys[i].key);
                if (e->keycode == keycode && 
                    (e->state & keys[i].modifier) == keys[i].modifier) {
                    keys[i].func(&keys[i].arg);
                }
            }
			break;
		}
		/*case ConfigureRequest:
			break;
		default:
			break; */
        }
    }
    XCloseDisplay(dpy);
}
