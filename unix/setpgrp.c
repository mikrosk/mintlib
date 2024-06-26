/*  src/setpgrp.c -- MiNTLib.
    Copyright (C) 1999 Guido Flohr <guido@freemint.de>

    This file is part of the MiNTLib project, and may only be used
    modified and distributed under the terms of the MiNTLib project
    license, COPYMINT.  By continuing to use, modify, or distribute
    this file you indicate that you have read the license and
    understand and accept it fully.
*/

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include "lib.h"

int
__setpgrp (void)
{
	return __bsd_setpgrp (0, 0);
}
weak_alias (__setpgrp, setpgrp)
