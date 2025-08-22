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
static int resizing = 0;

// moving
static int drag_start_x, drag_start_y;
static int win_start_x, win_start_y;
static Window drag_win;

// resizing
static int resize_start_x, resize_start_y;
static int win_start_width, win_start_height;
static Window resize_win;

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
        code = XKeysymToKeycode(dpy, keys[i].key); // THIS
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
    if (moving) {
        int dx = e->xmotion.x_root - drag_start_x;
        int dy = e->xmotion.y_root - drag_start_y;

        XMoveWindow(dpy, drag_win,
                    win_start_x + dx,
                    win_start_y + dy);
    }

    if (resizing) {
        int dx = e->xmotion.x_root - resize_start_x;
        int dy = e->xmotion.y_root - resize_start_y;

        int new_width = win_start_width + dx;
        int new_height = win_start_height + dy;

        if (new_width < 50) new_width = 50;
        if (new_height < 50) new_height = 50;

        XResizeWindow(dpy, resize_win, new_width, new_height);
    }
}

void buttonpress(XEvent *e) {
    Window w = e->xbutton.subwindow;
    if (!w || w == root) return;

    XRaiseWindow(dpy, w);

    if (e->xbutton.state & Mod4Mask) {
        if (e->xbutton.button == Button1) {
            // move
            moving = 1;
            drag_win = w;
            drag_start_x = e->xbutton.x_root;
            drag_start_y = e->xbutton.y_root;

            XWindowAttributes attr;
            XGetWindowAttributes(dpy, w, &attr);
            win_start_x = attr.x;
            win_start_y = attr.y;

            XGrabPointer(dpy, w, False,
                         PointerMotionMask | ButtonReleaseMask,
                         GrabModeAsync, GrabModeAsync,
                         None, None, CurrentTime);

        } else if (e->xbutton.button == Button3) {
            // resize
            resizing = 1;
            resize_win = w;
            resize_start_x = e->xbutton.x_root;
            resize_start_y = e->xbutton.y_root;

            XWindowAttributes attr;
            XGetWindowAttributes(dpy, w, &attr);
            win_start_width = attr.width;
            win_start_height = attr.height;

            XGrabPointer(dpy, w, False,
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

    if (resizing) {
        resizing = 0;
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
    [KeyPress]         = keypress,
    [ButtonPress]      = buttonpress,
    [EnterNotify]      = enternotify,
    [MapRequest]       = maprequest,
    [DestroyNotify]    = destroynotify,
    [ConfigureRequest] = configurerequest,
    [MotionNotify]     = motionnotify,
    [ButtonRelease]    = buttonrelease,
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

