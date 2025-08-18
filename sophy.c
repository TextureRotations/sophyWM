#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

typedef struct client {
	Window w;
} client;

static client  *cur;
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

void spawn(Arg *a);
void kill(Arg *a);
void focus(client *c);
void keypress(XEvent *e);
void buttonpress(XEvent *e);

#include "config.h"

void spawn(Arg *a) {
    if (fork() == 0) {
		setsid();
		execvp(a->v[0], a->v);
		exit(0);
	}
}

void kill(Arg *a) {
	if (!cur) return;
	if (cur->w == root) return;
	
	XKillClient(dpy, cur->w);	
}

void focus(client *c) {
    if (!c) return;
    cur = c;

    XSetInputFocus(dpy, cur->w, RevertToParent, CurrentTime);

	char *window_name = 0;
	if (XFetchName(dpy, cur->w, &window_name) && window_name) {
		fprintf(stderr, "Focused to: %s\n", window_name);
		XFree(window_name);
	}
}

void keypress(XEvent *e) {
    KeySym keysym = XkbKeycodeToKeysym(dpy, e->xkey.keycode, 0, 0); // converts numeric keycode into a keysym
	
	unsigned int cleanmask = e->xkey.state & ~(LockMask | Mod2Mask);

    for (unsigned int i = 0; i < sizeof(keys)/sizeof(*keys); ++i) // calculates how many keybindings exists
        if (keys[i].key == keysym && 
			keys[i].modifier == cleanmask)
            keys[i].func(&keys[i].arg); // if pressed keys are  matching any existing binding it calls associated with that binding function
}

void buttonpress(XEvent *e) {
	fprintf(stderr, "mouse button clicked\n");

	Window w = e->xbutton.subwindow;
    if (w == None) return;

	client c = { .w = w };
	focus(&c);
    XRaiseWindow(dpy, w);
    // XSetInputFocus(dpy, w, RevertToParent, CurrentTime);
}

static void (*eventhandler[])(XEvent *e) = {
    [KeyPress] 	  = keypress,
	[ButtonPress] = buttonpress,
};

int main(void) {
    if (!(dpy = XOpenDisplay(0))) {
        fprintf(stderr, "error opening display\n");
        return 1;
    }
    root = DefaultRootWindow(dpy);
    XSelectInput(dpy, root,
				 // SubstructureNotifyMask |
				 // SubstructureRedirectMask |
				 ButtonPressMask |
				 KeyPressMask);

	XDefineCursor(dpy, root, XCreateFontCursor(dpy, 2)); // 24 - circle | 126 - star 

    XEvent event;
    for (;;) {
		XNextEvent(dpy, &event);
        if (eventhandler[event.type])
            eventhandler[event.type](&event);
    }
}
