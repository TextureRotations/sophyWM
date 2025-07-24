#define _POSIX_C_SOURCE 200112L

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

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
Display *dpy;
Window root;

void grab_keys(void);
void spawn(Arg *a);
void kill(Arg *a);

#include "config.h"

void grab_keys(void) {
    for (unsigned int i = 0; i < sizeof(keys)/sizeof(keys[0]); i++) {
        KeyCode keycode = XKeysymToKeycode(dpy, keys[i].key);
        XGrabKey(dpy, keycode, keys[i].modifier, root, True,
                GrabModeAsync, GrabModeAsync);
    }
}

void kill(Arg *a) {
    (void)a;
    Window focused;
    int revert;
    XGetInputFocus(dpy, &focused, &revert);
    if (focused != None && focused != root)
        XKillClient(dpy, focused);
}

void spawn(Arg *a) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        return;
    }

    if (pid == 0) { // Child process
        // Set X11 environment (Arch-specific defaults)
        if (!getenv("DISPLAY")) setenv("DISPLAY", ":0", 1);
        if (!getenv("XAUTHORITY")) {
            setenv("XAUTHORITY", "/home/oy/.Xauthority", 1); // ← Change this!
        }

        execvp(a->v[0], a->v);
        perror("execvp failed");
        _exit(1);
    }

    fprintf(stderr, "Spawned PID %d: %s\n", pid, a->v[0]);
}

int main(void) {
    if (!(dpy = XOpenDisplay(0))) exit(1);

    root = DefaultRootWindow(dpy);
    
    XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask);
    XDefineCursor(dpy, root, XCreateFontCursor(dpy, 1));  // Default cursor value is 68
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
        case CreateNotify:
        case DestroyNotify:
        case MapRequest:
        case ConfigureRequest:
            break;
        default:
            break;
        }
    }

    XCloseDisplay(dpy);
}
