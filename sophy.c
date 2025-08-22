#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "sophy.h"

static Window  cur;
static Display *dpy;
static Window  root;

static int moving = 0;
static int drag_start_x, drag_start_y;
static int win_start_x, win_start_y;
static Window drag_win;

#include "config.h"

void kill(Arg *a) {
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
		// fprintf(stderr, "keys[%d].key = %lu\n", i, keys[i].key);
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

void motionnotify(XEvent *e) {
    if (!moving) return;

    int dx = e->xmotion.x_root - drag_start_x;
    int dy = e->xmotion.y_root - drag_start_y;

    XMoveWindow(dpy, drag_win,
                win_start_x + dx,
                win_start_y + dy);
}

void buttonpress(XEvent *e) {
    Window w = e->xbutton.subwindow;
    if (!w || w == root) return;

    if (e->xbutton.button == Button1) {
        XRaiseWindow(dpy, w);

        if (e->xbutton.state & Mod4Mask) {
            moving = 1;
            drag_win = w;
            drag_start_x = e->xbutton.x_root;
            drag_start_y = e->xbutton.y_root;

            XWindowAttributes attr;
            XGetWindowAttributes(dpy, w, &attr);
            win_start_x = attr.x;
            win_start_y = attr.y;

            XGrabPointer(dpy, root, False,
                         PointerMotionMask | ButtonReleaseMask,
                         GrabModeAsync, GrabModeAsync,
                         None, None, CurrentTime);
        }
    }
}

void buttonrelease(XEvent *e) {
    if (moving) {
        moving = 0;
        XUngrabPointer(dpy, CurrentTime);
    }
}

void destroynotify(XEvent *e) {
    XDestroyWindowEvent *ev = &e->xdestroywindow;

    if (ev->window == cur) {
        fprintf(stderr, "window 0x%lx destroyed, clearing cur\n", cur);
        cur = 0;
    }
}

void configurerequest(XEvent *e) {
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    XWindowChanges wc;

    wc.x = ev->x;
    wc.y = ev->y;
    wc.width = ev->width;
    wc.height = ev->height;
    wc.border_width = ev->border_width;
    wc.sibling = ev->above;
    wc.stack_mode = ev->detail;

    XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
}

static void (*eventhandler[])(XEvent *e) = {
    [KeyPress]    = keypress,
    [ButtonPress] = buttonpress,
    [EnterNotify] = enternotify,
    [MapRequest]  = maprequest,
	[DestroyNotify] = destroynotify,
	[ConfigureRequest] = configurerequest,
	[MotionNotify]   = motionnotify,
	[ButtonRelease]  = buttonrelease,
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
    			 ButtonPressMask | KeyReleaseMask);

    XDefineCursor(dpy, root, XCreateFontCursor(dpy, 2));

    XEvent event;
    for (;;) {
        XNextEvent(dpy, &event);
        if (eventhandler[event.type])
            eventhandler[event.type](&event);
    }
}

