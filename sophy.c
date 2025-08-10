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

typedef struct ClientNode {
    client *c;
    struct ClientNode *next;
} ClientNode;

static client *cur = NULL; // current focused client
static Display *dpy;       // connection handle
static Window root;

static unsigned long border_color_focused;
static unsigned long border_color_unfocused;
static const unsigned int border_width = 2;

ClientNode *clients = NULL;  // linked list of clients

void focus(client *c);
void grab_keys(void);
void kill(Arg *a);
void spawn(Arg *a);

#include "config.h"

unsigned long parse_hex_color(const char *hex) {
    XColor color, dummy;
    Colormap cmap = DefaultColormap(dpy, DefaultScreen(dpy));
    if (!XParseColor(dpy, cmap, hex, &color) || !XAllocColor(dpy, cmap, &color)) {
        fprintf(stderr, "Failed to parse or allocate color %s\n", hex);
        return BlackPixel(dpy, DefaultScreen(dpy));
    }
    return color.pixel;
}

void add_client(client *c) {
    ClientNode *node = malloc(sizeof(ClientNode));
    if (!node) return;
    node->c = c;
    node->next = clients;
    clients = node;
}

client *find_client(Window w) {
    for (ClientNode *node = clients; node; node = node->next) {
        if (node->c->w == w)
            return node->c;
    }
    return NULL;
}

void focus(client *c) {
    if (!c) return;

    // Unfocus previous client border
    if (cur && cur != c) {
        XSetWindowBorder(dpy, cur->w, border_color_unfocused);
    }

    cur = c;

    // Focused client border color
    XSetWindowBorder(dpy, cur->w, border_color_focused);

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

	border_color_focused = parse_hex_color(BORDER_COLOR_FOCUSED);
	border_color_unfocused = parse_hex_color(BORDER_COLOR_UNFOCUSED);

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

    int moving = 0, resizing = 0;
    int drag_start_x = 0, drag_start_y = 0;
    int win_start_x = 0, win_start_y = 0;
    unsigned int win_start_w = 0, win_start_h = 0;
    client *moving_client = NULL;
    client *resizing_client = NULL;

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

                // Set border width and initial color
                XSetWindowBorderWidth(dpy, w, border_width);
                XSetWindowBorder(dpy, w, border_color_unfocused);

                add_client(c);

                // Focus (no raise)
                focus(c);
                break;
            }

            case EnterNotify: {
                if (moving || resizing) break;

                Window w = ev.xcrossing.window;
                if (cur && cur->w == w) break;

                client *c = find_client(w);
                if (!c) break;

                focus(c);
                break;
            }

            case ButtonPress: {
                XButtonEvent *e = &ev.xbutton;

                client *clicked_client = find_client(e->window);
                if (clicked_client) {
                    // Raise and focus on click
                    XRaiseWindow(dpy, clicked_client->w);
                    focus(clicked_client);
                }

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

                    if (new_w < 50) new_w = 50;
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
