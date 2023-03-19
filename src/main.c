#undef _GNU_SOURCE
#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <stdarg.h>
#include <signal.h>

#include "input.h"
#include "rc.h"
#include "sys.h"
#include "rckeys.h"
#include "emu.h"
#include "exports.h"
#include "loader.h"
#include "io.h"

#include "Version"

#ifdef PICO
#include "hardware/vreg.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"
#endif

static char *defaultconfig[] =
{
	"bind up +up",
	"bind down +down",
	"bind left +left",
	"bind right +right",
	"bind w +up",
	"bind s +down",
	"bind a +left",
	"bind d +right",
	"bind q +a",
	"bind e +b",
	"bind enter +start",
	"bind x +select",
	"bind tab +select",
	"bind joyup +up",
	"bind joydown +down",
	"bind joyleft +left",
	"bind joyright +right",
	"bind joy0 +b",
	"bind joy1 +a",
	"bind joy2 +select",
	"bind joy3 +start",
	"bind ctrl reset",
	"bind 1 \"set saveslot 1\"",
	"bind 2 \"set saveslot 2\"",
	"bind 3 \"set saveslot 3\"",
	"bind 4 \"set saveslot 4\"",
	"bind 5 \"set saveslot 5\"",
	"bind 6 \"set saveslot 6\"",
	"bind 7 \"set saveslot 7\"",
	"bind 8 \"set saveslot 8\"",
	"bind 9 \"set saveslot 9\"",
	"bind 0 \"set saveslot 0\"",
	"bind ins savestate",
	"bind del loadstate",
	"set filterdmg 0",
	"set sound 1",
	"set scale 2",
	"set density 4",
	"set integer_scale 1",
	"set altenter 1",
	"set joy_rumble_strength 100", //0 to 100%
	"set joy_deadzone 40", //0 to 100%
	"set alert_on_quit 0",
	"source gnuboy.rc",
	"bootrom_dmg \"\"",
	"bootrom_gbc \"\"",
	NULL
};


static void banner()
{
	printf("\ngnuboy " VERSION "\n");
}

static void copyright()
{
	banner();
	printf(
"Copyright (C) 2000-2001 Laguna and Gilgamesh\n"
"Copyright (C) 2020 Alex Oberhofer\n"
"Portions contributed by other authors; see CREDITS for details.\n"
"\n"
"This program is free software; you can redistribute it and/or modify\n"
"it under the terms of the GNU General Public License as published by\n"
"the Free Software Foundation; either version 2 of the License, or\n"
"(at your option) any later version.\n"
"\n"
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU General Public License\n"
"along with this program; if not, write to the Free Software\n"
"Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n"
"\n");
}

static void usage(char *name)
{
	copyright();
	printf("Type %s --help for detailed help.\n\n", name);
	exit(1);
}

static void copying()
{
	copyright();
	exit(0);
}

static void help(char *name)
{
	banner();
	printf("Usage: %s [options] romfile\n", name);
	printf("\n"
"      --source FILE             read rc commands from FILE\n"
"      --link CONFIG             connect via link cable\n"
"                                   to connect over TCP/IP: network:<local-port>:<remote-host>:<remote-port> (*nix systems only)\n"
"                                   to connect over local unix pipe: pipe:<file-path> (*nix systems only)\n"
"      --bind KEY COMMAND        bind KEY to perform COMMAND\n"
"      --VAR=VALUE               set rc variable VAR to VALUE\n"
"      --VAR                     set VAR to 1 (turn on boolean options)\n"
"      --no-VAR                  set VAR to 0 (turn off boolean options)\n"
"      --showvars                list all available rc variables\n"
"      --help                    display this help and exit\n"
"      --version                 output version information and exit\n"
"      --copying                 show copying permissions\n"
"");
	exit(0);
}


static void version(char *name)
{
	printf("%s-" VERSION "\n", name);
	exit(0);
}


void doevents()
{
	event_t ev;
	int st;

	ev_poll();
	while (ev_getevent(&ev))
	{
		if (ev.type != EV_PRESS && ev.type != EV_RELEASE)
			continue;
		st = (ev.type != EV_RELEASE);
		rc_dokey(ev.code, st);
	}

	io_recv();
}




static void shutdown()
{
	vid_close();
	pcm_close();
	io_shutdown();
}

void die(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	exit(1);
}

static int bad_signals[] =
{
	/* These are all standard, so no need to #ifdef them... */
	SIGINT, SIGSEGV, SIGTERM, SIGFPE, SIGABRT, SIGILL,
#ifdef SIGQUIT
	SIGQUIT,
#endif
#ifdef SIGPIPE
	SIGPIPE,
#endif
	0
};

static void fatalsignal(int s)
{
	die("Signal %d\n", s);
}

static void catch_signals()
{
	int i;
	for (i = 0; bad_signals[i]; i++)
		signal(bad_signals[i], fatalsignal);
}



static char *base(char *s)
{
	char *p;
	p = strrchr(s, '/');
	if (p) return p+1;
	return s;
}

#ifdef PICO
#ifndef OVERCLOCK_250
#define OVERCLOCK_250 1
#endif
#endif

// Pico version
int main()
{
	int i;
	char *s = 0;

#ifdef PICO
	#if OVERCLOCK_250
	// Apply a modest overvolt, default is 1.10v.
	// this is required for a stable 250MHz on some RP2040s
	vreg_set_voltage(VREG_VOLTAGE_1_20);
	sleep_ms(10);
	set_sys_clock_khz(250000, 0);
	#endif
#endif

	vid_preinit();

	init_exports();

	s = "./";
	sys_sanitize(s);
	sys_initpath(s);

	for (i = 0; defaultconfig[i]; i++)
		rc_command(defaultconfig[i]);

	atexit(shutdown);
	catch_signals();

	vid_init();
	pcm_init();
	loader_init();
	
	emu_reset();
	emu_run();

	/* never reached */
	return 0;
}
