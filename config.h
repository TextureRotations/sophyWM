#ifndef CONFIG_H
#define CONFIG_H

#define MOD Mod4Mask

static char* term[]  = { "urxvt",       0};
static char* clock[] = { "xclock",      0};

static struct KeyEvent keys[] = {
    { MOD, XK_c,      kill,  {0}},
	{ MOD, XK_p,      move,  {0}},
    { MOD, XK_Return, spawn, {.v = term}},
	{ MOD, XK_t,      spawn, {.v = clock}}
};

#endif
