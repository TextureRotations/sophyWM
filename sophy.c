#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <stdio.h>
#include <unistd.h>

static Display *dpy;
static Window root;

typedef struct Arg {
    char **v;
} Arg;

typedef struct KeyEvent {
    int modifier;
    int key;
    void (*func)(Arg *a);
    Arg arg;
} KeyEvent;

void spawn(Arg *a);
void keypress(XEvent *event);

#include "config.h"

void spawn(Arg *a) {
    fprintf(stderr, "haiii :P\n");
}

void keypress(XEvent *e) {
    KeySym keysym = XkbKeycodeToKeysym(dpy, e->xkey.keycode, 0, 0);
    for (unsigned int i = 0; i < sizeof(keys)/sizeof(*keys); ++i)
        if (keys[i].key == keysym &&
        	keys[i].modifier == e->xkey.state)
            keys[i].func(&keys[i].arg);
}

static void (*eventhandler[])(XEvent *event) = {
    [KeyPress] = keypress,
};

int main(void) {
    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "error opening display\n");
        return 1;
    }
    root = DefaultRootWindow(dpy);
    XSelectInput(dpy, root, KeyPressMask);

    XEvent event;
    while (!XNextEvent(dpy, &event)) {
        if (eventhandler[event.type])
            eventhandler[event.type](&event);
    }
}

