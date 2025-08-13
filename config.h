#define MOD Mod4Mask

char *terminal[] = { "urxvt", NULL };

struct KeyEvent keys[] = {
	{ MOD, XK_Return, spawn, {.v = terminal} },
};
