#define MOD Mod4Mask

char *terminal[] = {"xterm", 0};

struct KeyEvent keys[] = {
	{MOD, XK_Return, spawn, {.v = terminal}},
};
