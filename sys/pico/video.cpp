#include "pico/stdlib.h"
#include <stdio.h>

#include "st7789.hpp"
#include "input.hpp"

#include "fb.h"
#include "rc.h"

#define DISPLAY_ST7789

#define SCALE

#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 240

// Display specific
static bool have_vsync = false;
static bool backlight_enabled = false;
static uint32_t last_render = 0;

static volatile bool do_render = true;


static void vsync_callback(uint gpio, uint32_t events) {
  if(!do_render && !st7789::dma_is_busy()) {
    st7789::update();
    do_render = true;
  }
}

// Common
struct fb fb;

static byte pix[160 * 144 * sizeof(uint16_t)];


rcvar_t vid_exports[] =
{
};

extern "C" void vid_init()
{

  	st7789::init();
	#ifdef SCALE
	st7789::set_pixel_sesquialteral(true);
	#endif
  	st7789::clear();

	#ifdef SCALE
  	st7789::set_window((DISPLAY_WIDTH - 240) / 2, (DISPLAY_HEIGHT - 216) / 2, 240, 216);
	#else
  	st7789::set_window((DISPLAY_WIDTH - 160) / 2, (DISPLAY_HEIGHT - 144) / 2, 160, 144);
	#endif

  	st7789::set_backlight(255);

	// draw_setup();

	st7789::frame_buffer = (uint16_t *)pix;
	st7789::update();
	

	fb.w = 160;
	fb.h = 144;
	fb.pelsize = 2;
	fb.pitch = fb.w * fb.pelsize;
	fb.indexed = fb.pelsize == 1;
	fb.ptr = pix;
	fb.cc[0].r = 3;
	fb.cc[1].r = 2;
	fb.cc[2].r = 3;
	fb.cc[3].r = 0;
	fb.cc[0].l = 0;
	fb.cc[1].l = 5;
	fb.cc[2].l = 11;
	fb.cc[3].l = 0;

	fb.enabled = 1;
	fb.dirty = 0;

	init_input();
}

extern "C" void vid_setpal(int i, int r, int g, int b)
{
}

extern "C" void vid_preinit()
{
}

extern "C" void vid_close()
{
  st7789::clear();
}

extern "C" void vid_settitle(char *title)
{
}

extern "C" void vid_begin()
{
}

extern "C" void vid_fullscreen_toggle()
{	
}

extern "C" void vid_end()
{
	uint32_t time = to_ms_since_boot(get_absolute_time());

  	if((do_render || (!have_vsync && time - last_render >= 20)) && !st7789::dma_is_busy()) {
		if(!have_vsync) {
			while(st7789::dma_is_busy()) {} // may need to wait for lores.
			st7789::update();
		}

		if(last_render && !backlight_enabled) {
			// the first render should have made it to the screen at this point
			st7789::set_backlight(255);
			backlight_enabled = true;
		}

		last_render = time;
    	do_render = false;
  	}
}

extern "C" void vid_screenshot()
{
}
