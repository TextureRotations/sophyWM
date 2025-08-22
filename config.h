#define MOD Mod4Mask

char *terminal[] = {"xterm",  0};
char *clock[]	 = {"xclock", 0};
char *menu[]     = {"dmenu_run", 0};

struct KeyEvent keys[] = {
	{MOD, XK_Return, spawn, {.v = terminal}},
	{MOD, XK_t, spawn, 		{.v = clock}},
	{MOD, XK_d, spawn,		{.v = menu}},
	{MOD, XK_c, kill, 		{0}},
};
