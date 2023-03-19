#include "pico/stdlib.h"
#include <time.h>
#include "file.hpp"
#include "usb.hpp"
#include "rc.h"

extern "C" uint32_t *sys_timer()
{
	uint32_t *timer = new uint32_t;
	*timer = time_us_32();
	return timer;
}

extern "C" int sys_elapsed(int *cl)
{
	uint32_t now;
	uint32_t usecs;

	now = time_us_32();
	usecs = now - *cl;
	*cl = now;

	return usecs;
}

extern "C" void sys_sleep(int us)
{
	if(us <= 0) return;

	sleep_us(us);
}

extern "C" void sys_sanitize(char *s)
{
}
extern "C" void sys_initpath()
{
	init_fs();
	init_usb();

	char *buf = ".";

	if (rc_getstr("rcpath") == NULL)
		rc_setvar("rcpath", 1, &buf);

	if (rc_getstr("savedir") == NULL)
		rc_setvar("savedir", 1, &buf);
}

extern "C" void sys_checkdir(char *path, int wr)
{
}
