#include <X11/Xlib.h>
#include <X11/keysym.h>
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

static client *cur = NULL;
static Display *dpy;
static Window root;
void focus(client *c);
void grab_keys(void);
void kill(Arg *a);
void spawn(Arg *a);

#include "config.h"

void focus(client *c) {
    if (!c) return;
    cur = c;
    XRaiseWindow(dpy, cur->w);
    XSetInputFocus(dpy, cur->w, RevertToParent, CurrentTime);

    char *window_name = NULL;
    if (XFetchName(dpy, cur->w, &window_name) && window_name) {
        fprintf(stderr, "Focused: %s\n", window_name);
        XFree(window_name);
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
        fprintf(stderr, "execvp failed\n");
        exit(1);
    }
}

void grab_keys(void) {
    for (unsigned int i = 0; i < sizeof(keys)/sizeof(keys[0]); i++) {
        KeyCode keycode = XKeysymToKeycode(dpy, keys[i].key);
        XGrabKey(dpy, keycode, keys[i].modifier, root, True,
                 GrabModeAsync, GrabModeAsync);
    }
}

int main(void) {
    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "Failed to open display\n");
        return 1;
    }
    root = DefaultRootWindow(dpy);

    // Select events on root window
    XSelectInput(dpy, root,
                 SubstructureRedirectMask |
                 SubstructureNotifyMask |
                 StructureNotifyMask |
                 EnterWindowMask |
                 ButtonPressMask |
                 ButtonReleaseMask |
                 PointerMotionMask);

    XDefineCursor(dpy, root, XCreateFontCursor(dpy, 68));
    grab_keys();
    XSync(dpy, False);

    XEvent ev;
    int moving = 0;
    int drag_start_x = 0, drag_start_y = 0;
    int win_start_x = 0, win_start_y = 0;

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
                XSelectInput(dpy, w, EnterWindowMask | FocusChangeMask | ButtonPressMask);

                client *c = malloc(sizeof(client));
                if (!c) {
                    fprintf(stderr, "malloc failed\n");
                    break;
                }

                c->w = w;
                c->wx = wa.x;
                c->wy = wa.y;
                c->ww = wa.width;
                c->wh = wa.height;

                focus(c);
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

                focus(c);
                break;
            }

            case ButtonPress: {
                XButtonEvent *e = &ev.xbutton;

                // Only start moving if focused window, left click + MOD key pressed
                if (cur && e->window == cur->w &&
                    (e->state & Mod4Mask) && e->button == Button1) {

                    moving = 1;
                    drag_start_x = e->x_root;
                    drag_start_y = e->y_root;

                    // Get current window position
                    Window dummy;
                    int wx, wy;
                    unsigned int bw, depth, w, h;
                    XGetGeometry(dpy, cur->w, &dummy, &wx, &wy, &w, &h, &bw, &depth);

                    win_start_x = wx;
                    win_start_y = wy;

                    // Grab the pointer for moving
                    XGrabPointer(dpy, root, True,
                                 PointerMotionMask | ButtonReleaseMask,
                                 GrabModeAsync, GrabModeAsync,
                                 None, None, CurrentTime);
                }
                break;
            }

            case MotionNotify: {
                if (!moving) break;
                XMotionEvent *e = &ev.xmotion;

                int dx = e->x_root - drag_start_x;
                int dy = e->y_root - drag_start_y;

                int new_x = win_start_x + dx;
                int new_y = win_start_y + dy;

                XMoveWindow(dpy, cur->w, new_x, new_y);
                break;
            }

            case ButtonRelease: {
                if (!moving) break;
                XButtonEvent *e = &ev.xbutton;

                if (e->button == Button1) {
                    moving = 0;
                    XUngrabPointer(dpy, CurrentTime);
                }
                break;
            }

            default:
                break;
        }
    }

    XCloseDisplay(dpy);
    return 0;
}

