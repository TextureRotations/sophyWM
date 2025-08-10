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
    Window w; // window id
} client;

static client *cur = NULL; // current focused client
static Display *dpy;       // connections handle
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
    if (!cur) return;
    if (cur->w == root) return;

    XKillClient(dpy, cur->w);
}

void spawn(Arg *a) {
    if (fork() == 0) {
        if (dpy)
            close(ConnectionNumber(dpy));
        setsid();
        execvp(a->v[0], a->v);
        fprintf(stderr, "Execution failed\n");
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
    int moving = 0, resizing = 0;
    int drag_start_x = 0, drag_start_y = 0;
    int win_start_x = 0, win_start_y = 0;
    unsigned int win_start_w = 0, win_start_h = 0;
    client *moving_client = NULL;
    client *resizing_client = NULL;

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
                if (moving || resizing) break;

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

                // Moving
                if (cur && e->window == cur->w &&
                    (e->state & Mod4Mask) && e->button == Button1) {

                    moving = 1;
                    moving_client = cur;

                    drag_start_x = e->x_root;
                    drag_start_y = e->y_root;

                    Window dummy;
                    unsigned int bw, depth;
                    XGetGeometry(dpy, moving_client->w, &dummy,
                                 &win_start_x, &win_start_y,
                                 &win_start_w, &win_start_h, &bw, &depth);

                    XGrabPointer(dpy, root, True,
                                 PointerMotionMask | ButtonReleaseMask,
                                 GrabModeAsync, GrabModeAsync,
                                 None, None, CurrentTime);
                }

                // Resizing
                if (cur && e->window == cur->w &&
                    (e->state & Mod4Mask) && e->button == Button3) {

                    resizing = 1;
                    resizing_client = cur;

                    drag_start_x = e->x_root;
                    drag_start_y = e->y_root;

                    Window dummy;
                    unsigned int bw, depth;
                    XGetGeometry(dpy, resizing_client->w, &dummy,
                                 &win_start_x, &win_start_y,
                                 &win_start_w, &win_start_h, &bw, &depth);

                    XGrabPointer(dpy, root, True,
                                 PointerMotionMask | ButtonReleaseMask,
                                 GrabModeAsync, GrabModeAsync,
                                 None, None, CurrentTime);
                }
                break;
            }

            case MotionNotify: {
                XMotionEvent *e = &ev.xmotion;

                if (moving && moving_client) {
                    int dx = e->x_root - drag_start_x;
                    int dy = e->y_root - drag_start_y;
                    XMoveWindow(dpy, moving_client->w,
                                win_start_x + dx, win_start_y + dy);
                }

                if (resizing && resizing_client) {
                    int dx = e->x_root - drag_start_x;
                    int dy = e->y_root - drag_start_y;
                    int new_w = (int)win_start_w + dx;
                    int new_h = (int)win_start_h + dy;

                    if (new_w < 50) new_w = 50; // Minimum size
                    if (new_h < 50) new_h = 50;

                    XResizeWindow(dpy, resizing_client->w,
                                  (unsigned int)new_w, (unsigned int)new_h);
                }
                break;
            }

            case ButtonRelease: {
                XButtonEvent *e = &ev.xbutton;

                if (e->button == Button1 && moving) {
                    moving = 0;
                    moving_client = NULL;
                    XUngrabPointer(dpy, CurrentTime);
                }

                if (e->button == Button3 && resizing) {
                    resizing = 0;
                    resizing_client = NULL;
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
