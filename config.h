#ifndef CONFIG_H
#define CONFIG_H

#define MOD Mod4Mask

char *terminal[] = { "st", NULL };

static KeyEvent keys[] = {
	{ MOD, XK_c, killclient, {0} },
	{ MOD, XK_Return, spawn, { .com = terminal } }
};

#endif
