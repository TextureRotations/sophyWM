#include <X11/Xlib.h>
#include <stdio.h>
#include <unistd.h>

static Display *dpy;
static Window root;

void keypress(XEvent *event);

void keypress(XEvent *event) {
	fprintf(stderr, "haii \n");
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
	while (1 && XNextEvent(dpy, &event)) { // Xsession remains opened only when loop is running
		if 	(eventhandler[event.type]) 
			 eventhandler[event.type] (&event);
	}
}
