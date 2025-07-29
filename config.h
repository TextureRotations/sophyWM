#ifndef CONFIG_H
#define CONFIG_H

#define MOD Mod4Mask

static char* term[]  = { "st",         0};
static char* clock[] = { "xeyes",      0};

static struct KeyEvent keys[] = {
    { MOD, XK_c,      kill,  {0}},
    { MOD, XK_Return, spawn, {.v = term}},
	{ MOD, XK_t,      spawn, {.v = clock}}
};

#endif
