//TODO:
//FOCUSED/UNFOCUSED
//KILL
//MOVE

#include <X11/keysym.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <X11/Xutil.h>

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

static int 			wx, wy;
static unsigned int ww, wh;

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
}

void move(Arg *a) {
      if (!cur) return;
  
      Screen *scr = DefaultScreenOfDisplay(dpy);
      int screen_width = scr->width;
      int screen_height = scr->height;
  
      int new_x = (screen_width - cur->ww) / 2;
      int new_y = (screen_height - cur->wh) / 2;
  
      XMoveWindow(dpy, cur->w, new_x, new_y);
}

void grab_keys(void) {
    for (unsigned int i = 0; i < sizeof(keys)/sizeof(keys[0]); i++) {
        KeyCode keycode = XKeysymToKeycode(dpy, keys[i].key);
        XGrabKey(dpy, keycode, keys[i].modifier, root, True,
                GrabModeAsync, GrabModeAsync);
    }
}

void kill(Arg *a) {

}

void spawn(Arg *a) {
    if (fork() == 0) {
        if (dpy)
            close(ConnectionNumber(dpy));

        setsid();

        char *display_env = getenv("DISPLAY");
        if (!display_env) {
            fprintf(stderr, "DISPLAY not set\n");
            exit(1);
        }

        execvp(a->v[0], a->v);
        perror("execvp failed");
        exit(1);
    }
}

int main(void) {
    if (!(dpy = XOpenDisplay(NULL))) exit(1);

    root = DefaultRootWindow(dpy);

    XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask | StructureNotifyMask);
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
        case MapRequest:
            XMapWindow(dpy, ev.xmaprequest.window);
            break;
        }
    }

    XCloseDisplay(dpy);
    return 0;
}
