/*
 * ioctl() emulation for MiNT; written by Eric R. Smith and placed
 * in the public domain
 */

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <support.h>

#include <mint/linea.h>					/* for TIOCGWINSZ under TOS */
#include <mint/mintbind.h>
#include <mint/ssystem.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "lib.h"

static int _ttydisc = NTTYDISC;
static int _ldisc = LLITOUT;

/* in read.c */
extern struct tchars __tchars;
extern struct ltchars __ltchars;

int __ioctl(int fd, int cmd, void *arg)
{
	long r;
	struct sgttyb *sg;
	int istty;

	/* first try the systemcall */
	switch (cmd)
	{
	case TIOCSCTTY:
		/* The kernel ioctl() is broken in
		 * kernels < 1.16, so don't try it with
		 * these
		 */
		if (__mint && __mint < 0x110)
		{
			r = -ENOSYS;
			break;
		}
		/* fall through */
	default:
		r = Fcntl(fd, arg, cmd);
		break;
	}
	
	if (r >= 0)
	{
		return (int) r;
	}
	if (r != -ENOSYS && r != -EINVAL)
	{
		__set_errno((int) -r);
		return -1;
	}
	
	istty = isatty(fd);
	/* if failed fallback to some brain damaged emulation */
	switch (cmd)
	{
	case TIOCGETD:
		if (istty)
		{
			*((long *) arg) = _ttydisc;
			return 0;
		}
		break;
	case TIOCSETD:
		if (istty)
		{
			_ttydisc = *((long *) arg);
			return 0;
		}
		break;
	case TIOCLGET:
		if (istty)
		{
			*((long *) arg) = _ldisc;
			return 0;
		}
		break;
	case TIOCLSET:
		if (istty)
		{
			_ldisc = *((long *) arg);
			return 0;
		}
		break;
	case TIOCLBIS:
		if (istty)
		{
			_ldisc |= *((long *) arg);
			return 0;
		}
		break;
	case TIOCLBIC:
		if (istty)
		{
			_ldisc &= ~(*((long *) arg));
			return 0;
		}
		break;
	case TIOCSWINSZ:
		if (!istty)
		{
			__set_errno(ENOTTY);
			return -1;
		}
		break;
	case TIOCGWINSZ:
		if (istty)
		{
			struct winsize *win = (struct winsize *) arg;

			(void) linea0();
			win->ws_row = V_CEL_MY + 1;
			win->ws_col = V_CEL_MX + 1;
			win->ws_xpixel = V_X_MAX;
			win->ws_ypixel = V_Y_MAX;
			return 0;
		}
		break;

	case TIOCNOTTY:
		if (istty)
		{
			int null_fd;

			/* To be on the safe side, always reset
			 * the isatty()-flags to "unknown" first
			 */
			__open_stat[__OPEN_INDEX(-1)].status = FH_UNKNOWN;
			__open_stat[__OPEN_INDEX(fd)].status = FH_UNKNOWN;

			if (fd < 0 || !_isctty(fd))
			{
				__set_errno(EBADF);
				return -1;
			}

			r = Fclose(-1);
			if (r >= 0)
				return 0;

			null_fd = (int) Fopen("U:\\dev\\null", O_RDWR);
			if (null_fd <= -6)
				null_fd = (int) Fopen("NUL:", O_RDWR);
			(void) Fforce(-1, null_fd);
			if (null_fd != fd)
			{
				(void) Fforce(fd, null_fd);
				(void) Fclose(null_fd);
			}
			return 0;
		}
		break;

	case TIOCSCTTY:
		if (istty)
		{
			/* To be on the safe side, always reset
			 * the isatty()-flags to "unknown" first
			 */
			__open_stat[__OPEN_INDEX(-1)].status = FH_UNKNOWN;
			__open_stat[__OPEN_INDEX(fd)].status = FH_UNKNOWN;

			/* Try to emulate it.  In this case the
			   force flag is ignored.  */
			return dup2(fd, -1);
		}
		break;

	case TIOCSETP:
		if (istty)
		{
			fd = __OPEN_INDEX(fd);
			if (fd < 0 || fd >= __NHANDLES)
				fd = __NHANDLES - 1;
			sg = (struct sgttyb *) arg;
			__open_stat[fd].flags = sg->sg_flags;
			return 0;
		}
		break;

	case TIOCGETP:
		if (istty)
		{
			fd = __OPEN_INDEX(fd);
			if (fd < 0 || fd >= __NHANDLES)
				fd = __NHANDLES - 1;
			sg = (struct sgttyb *) arg;
			sg->sg_flags = __open_stat[fd].flags;
			sg->sg_ispeed = sg->sg_ospeed = B9600;
			sg->sg_erase = 'H' & 0x1f;
			sg->sg_kill = 'U' & 0x1f;
			return 0;
		}
		break;

	case TIOCGETC:
		if (istty)
		{
			*((struct tchars *) arg) = __tchars;
			return 0;
		}
		break;

	case TIOCSETC:
		if (istty)
		{
			__tchars = *((struct tchars *) arg);
			return 0;
		}
		break;

	case TIOCGLTC:
		if (istty)
		{
			*((struct ltchars *) arg) = __ltchars;
			return 0;
		}
		break;

	case TIOCSLTC:
		if (istty)
		{
			__ltchars = *((struct ltchars *) arg);
			return 0;
		}
		break;

	case TIOCGPGRP:
		if (istty)
		{
			*((long *) arg) = 0;
			return 0;
		}
		break;

	case TIOCSPGRP:
		if (istty)
			return 0;
		break;

	case TIOCCDTR:
		{
			long baud = 0;

			r = Fcntl(fd, &baud, TIOCOBAUD);
			if (r < 0)
			{
				__set_errno((int) -r);
				return -1;
			}
		}
		return 0;

	case TIOCSDTR:
		{
			long baud = -1;

			r = Fcntl(fd, &baud, TIOCOBAUD);
			if (r < 0)
			{
				__set_errno((int) -r);
				return -1;
			}
			r = Fcntl(fd, &baud, TIOCOBAUD);
			if (r < 0)
			{
				__set_errno((int) -r);
				return -1;
			}
		}
		return 0;

	case TIOCMGET:
		{
			char g;
			long ssp;
			short *mfp;
			short m;
			struct xattr sb;
			long *msig;

			msig = (long *) arg;
			r = Fcntl(fd, (long) &sb, FSTAT);
			if (r < 0)
			{
				__set_errno((int) -r);
				return -1;
			}
			if ((((mode_t) sb.st_mode & S_IFMT) == S_IFCHR) && (sb.st_rdev == 257))
			{
				*msig = TIOCM_DSR;
				g = Giaccess(0, 14);
				*msig |= ((g & (1 << 3)) ? 0 : TIOCM_RTS);
				*msig |= ((g & (1 << 4)) ? 0 : TIOCM_DTR);
				mfp = ((short *) 0xfffffa00L);
				if (__has_no_ssystem)
				{
					if (Super(1L) == 0L)
					{
						ssp = Super(0L);
						m = *mfp & 0xff;
						SuperToUser(ssp);
					} else
					{
						m = *mfp & 0xff;
					}
				} else
				{
					m = (short) (Ssystem(S_TIOCMGET, (u_long) mfp, NULL));
				}
				*msig |= ((m & (1 << 1)) ? 0 : TIOCM_CAR);
				*msig |= ((m & (1 << 2)) ? 0 : TIOCM_CTS);
				*msig |= ((m & (1 << 6)) ? 0 : TIOCM_RNG);
				return 0;
			}
		}
		__set_errno(EINVAL);
		return -1;

	case FIONBIO:
		r = Fcntl(fd, 0, F_GETFL);
		if (r < 0)
		{
			__set_errno((int) -r);
			return -1;
		}
		r = Fcntl(fd, *((long *) arg) ? (r | O_NDELAY) : (r & ~O_NDELAY), F_SETFL);
		if (r < 0)
		{
			__set_errno((int) -r);
			return -1;
		}
		return 0;
	}

	__set_errno(ENOSYS);
	return -1;
}

weak_alias(__ioctl, ioctl)
