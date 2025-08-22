#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

typedef struct client {
    Window w;
} client;

static Window  cur;   // <— store focused window directly
static Display *dpy;
static Window  root;

typedef struct Arg {
    char **v;
} Arg;

typedef struct KeyEvent {
    unsigned int modifier;
    unsigned int key;
    void (*func)(Arg *a);
    Arg arg;
} KeyEvent;

void kill(Arg *a);
void spawn(Arg *a);
void grabkeys(void);
void focus(client *a);
void keypress(XEvent *e);
void maprequest(XEvent *e);
void enternotify(XEvent *e);
void buttonpress(XEvent *e);

#include "config.h"

void kill(Arg *a) {
    fprintf(stderr, "call 1\n");

    if (!cur || cur == root) return;
    XKillClient(dpy, cur);
}

void spawn(Arg *a) {
    if (fork() == 0) {
        setsid();
        execvp(a->v[0], a->v);
        exit(0);
    }
}

void grabkeys(void) {
    KeyCode code;

    XUngrabKey(dpy, AnyKey, AnyModifier, root);

    for (unsigned int i = 0; i < sizeof(keys)/sizeof(*keys); i++) {
        code = XKeysymToKeycode(dpy, keys[i].key);
        if (code) {
            XGrabKey(dpy,
                     code,
                     keys[i].modifier,
                     root,
                     True,              
                     GrabModeAsync,     
					 GrabModeAsync);
        }
    }
}

void focus(client *c) {
    if (!c) return;
    cur = c->w;

    XSetInputFocus(dpy, cur, RevertToParent, CurrentTime);

    char *window_name = 0;
    if (XFetchName(dpy, cur, &window_name) && window_name) {
        fprintf(stderr, "Focused to: %s\n", window_name);
        XFree(window_name);
    }
}

void keypress(XEvent *e) {
    KeySym keysym = XkbKeycodeToKeysym(dpy, e->xkey.keycode, 0, 0);

    unsigned int cleanmask = e->xkey.state & ~(LockMask | Mod2Mask);

    for (unsigned int i = 0; i < sizeof(keys)/sizeof(*keys); ++i)
        if (keys[i].key == keysym &&
            keys[i].modifier == cleanmask)
            keys[i].func(&keys[i].arg);
}

void maprequest(XEvent *e) {
    XMapRequestEvent *ev = &e->xmaprequest;
    XSelectInput(dpy, ev->window, EnterWindowMask | FocusChangeMask);
    XMapWindow(dpy, ev->window);

    client c = { .w = ev->window };
    focus(&c);
}

void enternotify(XEvent *e) {
    XCrossingEvent *ev = &e->xcrossing;

    if (ev->window == root) return;

    client c = { .w = ev->window };
    focus(&c);
}

void buttonpress(XEvent *e) {
    Window w = e->xbutton.subwindow;
    if (w == None) return;

    XRaiseWindow(dpy, w);
}

static void (*eventhandler[])(XEvent *e) = {
    [KeyPress]    = keypress,
    [ButtonPress] = buttonpress,
    [EnterNotify] = enternotify,
    [MapRequest]  = maprequest,
};

int main(void) {
    if (!(dpy = XOpenDisplay(0))) {
        fprintf(stderr, "error opening display\n");
        return 1;
    }
    root = DefaultRootWindow(dpy);

    grabkeys();

    XSelectInput(dpy, root,
                 SubstructureNotifyMask | SubstructureRedirectMask |
                 ButtonPressMask);

    XDefineCursor(dpy, root, XCreateFontCursor(dpy, 2));

    XEvent event;
    for (;;) {
        XNextEvent(dpy, &event);
        if (eventhandler[event.type])
            eventhandler[event.type](&event);
    }
}

