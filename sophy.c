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
 	Window w;
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

void focus(void);
void move(Arg *a);
void grab_keys(void);
void kill(Arg *a);
void spawn(Arg *a);

#include "config.h"

void focus(void) {

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
	fprintf(stderr, "kill executed\n");
}

void spawn(Arg *a) {
    if (fork() == 0) {
        if (dpy)
            close(ConnectionNumber(dpy)); // close X connection in child

        setsid();

        // Make sure DISPLAY is still available in child
        char *display_env = getenv("DISPLAY");
        if (!display_env) {
            fprintf(stderr, "DISPLAY not set\n");
            exit(1);
        }

        execvp(a->v[0], a->v);
        perror("execvp failed");
        exit(1);
    }
}

int main(void) {
    if (!(dpy = XOpenDisplay(NULL))) exit(1);

    root = DefaultRootWindow(dpy);

    XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask | StructureNotifyMask);
    XDefineCursor(dpy, root, XCreateFontCursor(dpy, 68));
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
        case MapRequest:
            XMapWindow(dpy, ev.xmaprequest.window);
            break;
        }
    }

    XCloseDisplay(dpy);
    return 0;
}
