#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct Arg {
    char **v;
} Arg;

typedef struct KeyEvent {
    unsigned int modifier;
    KeySym key;
    void (*func)(Arg *a);
    Arg arg;
} KeyEvent;

typedef struct client {
	unsigned int ww, wh;
	int wx, wy;
	Window w;
} client;

static client 		*cur;
XButtonEvent 		mouse;
Display      		*dpy;
Window       		root;

void focus(client *c);
void move(Arg *a);
void grab_keys(void);
void kill(Arg *a);
void spawn(Arg *a);

#include "config.h"

void focus(client *c) {
	if (!c) return;
    cur = c;
  
    XRaiseWindow(dpy, cur->w);
    XSetInputFocus(dpy, cur->w, RevertToParent, CurrentTime);
	
	//print window name in console
	char *window_name = NULL;
	if (XFetchName(dpy, cur->w, &window_name) && window_name) {
		fprintf(stderr, "Focused to: %s\n", window_name);
		XFree(window_name);
	}
}

void move(Arg *a) {
    if (!cur) return;
  
    Screen *scr = DefaultScreenOfDisplay(dpy);
    int sw = scr->width;
    int sh = scr->height;
  
    int x = (sw - cur->ww) / 2;
	int y = (sh - cur->wh) / 2;
  
	XMoveWindow(dpy, cur->w, x, y);
}

void grab_keys(void) {
	for (unsigned int i = 0; i < sizeof(keys)/sizeof(keys[0]); i++) {
        KeyCode keycode = XKeysymToKeycode(dpy, keys[i].key);
        XGrabKey(dpy, keycode, keys[i].modifier, root, True,
                GrabModeAsync, GrabModeAsync);
    }
}

void kill(Arg *a) {
	if (cur) XKillClient(dpy, cur->w);
}

void spawn(Arg *a) {
    if (fork() == 0) {
        if (dpy)
            close(ConnectionNumber(dpy));

        setsid();
        execvp(a->v[0], a->v);
        fprintf(stderr, "execvp failedi\n");
        exit(1);
    }
}

int main(void) {
    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "Failed to open display\n");
        exit(1);
    }

    root = DefaultRootWindow(dpy);

    XSelectInput(dpy, root,
        SubstructureRedirectMask |
        SubstructureNotifyMask |
        StructureNotifyMask |
        EnterWindowMask);

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

        case MapRequest: {
            Window w = ev.xmaprequest.window;

            XWindowAttributes wa;
            if (!XGetWindowAttributes(dpy, w, &wa) || wa.override_redirect)
                break;

            XMapWindow(dpy, w);

            XSelectInput(dpy, w, EnterWindowMask | FocusChangeMask);

            client *c = malloc(sizeof(client));
            if (!c) {
                fprintf(stderr, "Failed to allocate memory for client\n");
                break;
            }

            c->w = w;
            c->wx = wa.x;
            c->wy = wa.y;
            c->ww = wa.width;
            c->wh = wa.height;

            focus(c); // automaticaly focus newly opened window
            break;
        }

        case EnterNotify: {
            Window w = ev.xcrossing.window;

            if (cur && cur->w == w) break;

            XWindowAttributes wa;
            if (!XGetWindowAttributes(dpy, w, &wa)) break;

            client *c = malloc(sizeof(client));
            if (!c) break;

            c->w = w;
            c->wx = wa.x;
            c->wy = wa.y;
            c->ww = wa.width;
            c->wh = wa.height;

            focus(c);  // focus the window under the mouse cursor
            break;
        }
		
        default:
            break;
        }
    }

    XCloseDisplay(dpy); // 2 possibly unnecessary lines
    return 0;
}
