#ifndef CONFIG_H
#define CONFIG_H

#define MOD Mod4Mask

static char *terminal[] = { "xterm", NULL };

static KeyEvent keys[] = {
    { MOD, XK_c,      kill, { .v = NULL } },
    { MOD, XK_Return, spawn,      { .v = terminal } }
};

#endif
