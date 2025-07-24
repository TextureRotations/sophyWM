#ifndef CONFIG_H
#define CONFIG_H

#define MOD Mod4Mask

static char *terminal[] = { "st", 0};

static struct KeyEvent keys[] = {
    { MOD, XK_c,      kill,       {.v = 0}},
    { MOD, XK_Return, spawn,      {.v = terminal}}
};

#endif
