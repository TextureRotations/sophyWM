#include <X11/Xlib.h>

typedef struct client {
	Window w;
} client;

typedef struct Arg {
	char **v;
} Arg;

typedef struct KeyEvent {
	unsigned int modifier;
	KeySym key;
	void (*func)(Arg *a);
	Arg arg;
} KeyEvent;

void buttonpress(XEvent *e);
void buttonrelease(XEvent *e);
void configurerequest(XEvent *e);
void destroynotify(XEvent *e);
void enternotify(XEvent *e);
void focus(client *a);
void grabbuttons(void);
void grabkeys(void);
void keypress(XEvent *e);
void kill(Arg *a);
void maprequest(XEvent *e);
void motionnotify(XEvent *e);
void spawn(Arg *a);

