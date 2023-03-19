#include <stdio.h>

#include "rc.h"

#include "input.h"
#include "input.hpp"
#include "sys.h"

#include "hardware/gpio.h"

#include "pico/binary_info.h"

rcvar_t joy_exports[] =
{
	RCV_END
};

static bool left, right, up, down, a_btn, b_btn, x_btn, y_btn, ctrl_btn; 

enum class ButtonIO {
#ifdef PIMORONI_PICOSYSTEM
  UP = 23,
  DOWN = 20,
  LEFT = 22,
  RIGHT = 21,

  A = 18,
  B = 19,
  X = 17,
  Y = 16,
#else
  // UP = 2,
  // DOWN = 3,
  // LEFT = 4,
  // RIGHT = 5,

  // A = 12,
  // B = 13,
  // X = 14,
  // Y = 15,

  UP = 2,
  DOWN = 18,
  LEFT = 16,
  RIGHT = 20,

  A = 15,
  B = 17,
  X = 19,
  Y = 21,

  CTRL = 3
#endif
};

static void init_button(ButtonIO b) {
  int gpio = static_cast<int>(b);
  gpio_set_function(gpio, GPIO_FUNC_SIO);
  gpio_set_dir(gpio, GPIO_IN);
  gpio_pull_up(gpio);
}

inline bool get_button(ButtonIO b) {
  return !gpio_get(static_cast<int>(b));
}

void update_button(ButtonIO b, int code, bool& state) {
  bool new_state = get_button(b);
  if (state != new_state) {
    event_t ev;
    ev.type = new_state ? EV_PRESS : EV_RELEASE;
    ev.code = code;
    printf("Button %03x %s\n", code, new_state ? "up" : "down");
    ev_postevent(&ev);
    state = new_state;
  }
}

void init_input() {
  init_button(ButtonIO::UP);
  init_button(ButtonIO::DOWN);
  init_button(ButtonIO::LEFT);
  init_button(ButtonIO::RIGHT);
  init_button(ButtonIO::A);
  init_button(ButtonIO::B);
  init_button(ButtonIO::X);
  init_button(ButtonIO::Y);
  init_button(ButtonIO::CTRL);

  #define BUTTON_DECL(btn) bi_decl(bi_1pin_with_name(static_cast<int>(ButtonIO::btn), #btn" Button"));
  BUTTON_DECL(UP)
  BUTTON_DECL(DOWN)
  BUTTON_DECL(LEFT)
  BUTTON_DECL(RIGHT)
  BUTTON_DECL(A)
  BUTTON_DECL(B)
  BUTTON_DECL(X)
  BUTTON_DECL(Y)
  #undef BUTTON_DECL
}

void update_input() {
  update_button(ButtonIO::LEFT,   K_JOYLEFT,  left  );
  update_button(ButtonIO::RIGHT,  K_JOYRIGHT, right );
  update_button(ButtonIO::UP,     K_JOYUP,    up    );
  update_button(ButtonIO::DOWN,   K_JOYDOWN,  down  );

  update_button(ButtonIO::A, K_JOY1, a_btn);
  update_button(ButtonIO::B, K_JOY0, b_btn);
  update_button(ButtonIO::X, K_JOY2, x_btn);
  update_button(ButtonIO::Y, K_JOY3, y_btn);

  update_button(ButtonIO::CTRL, K_CTRL, ctrl_btn);
}

extern "C" void ev_poll()
{
  update_input();
}
