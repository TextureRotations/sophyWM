#include <X11/Xlib.h>
#include <X11/keysym.h>
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
	void (*func)(Arg *a); // todo: replace func with more reasonable name 
	Arg arg;
} KeyEvent;

#include "config.h"

void spawn(Arg *a);
void keypress(XEvent *event);

void spawn(Arg *a) {
	fprintf(stderr, "haiii :P\n");
}

void keypress(XEvent *event) {

}

static void (*eventhandler[]) (XEvent *event) = {
	[KeyPress] = keypress, 
};

int main(void) {
	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "error opening display\n");
		return 1;
	}
	root = DefaultRootWindow(dpy);
	
	XEvent event;
	while (1 && XNextEvent(dpy, &event)) { // loop blocks until something happens
		if 	(eventhandler[event.type]) 
			 eventhandler[event.type] (&event);
	}
}
