#include "st7789.hpp"

#include <cstdlib>
#include <math.h>

#include "pico/stdlib.h"
#include <stdio.h>

#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "pico/binary_info.h"
#include "pico/time.h"

#include "config.h"
#include "st7789.pio.h"

namespace st7789 {

  enum MADCTL : uint8_t {
    ROW_ORDER   = 0b10000000,
    COL_ORDER   = 0b01000000,
    SWAP_XY     = 0b00100000,  // AKA "MV"
    SCAN_ORDER  = 0b00010000,
    RGB         = 0b00001000,
    HORIZ_ORDER = 0b00000100
  };

  #define ROT_240_240_0      0
  #define ROT_240_240_90     MADCTL::SWAP_XY | MADCTL::HORIZ_ORDER | MADCTL::COL_ORDER
  #define ROT_240_240_180    MADCTL::SCAN_ORDER | MADCTL::HORIZ_ORDER | MADCTL::COL_ORDER | MADCTL::ROW_ORDER
  #define ROT_240_240_270    MADCTL::SWAP_XY | MADCTL::HORIZ_ORDER | MADCTL::ROW_ORDER

   enum reg {
    SWRESET   = 0x01,
    TEOFF     = 0x34,
    TEON      = 0x35,
    MADCTL    = 0x36,
    COLMOD    = 0x3A,
    GCTRL     = 0xB7,
    VCOMS     = 0xBB,
    LCMCTRL   = 0xC0,
    VDVVRHEN  = 0xC2,
    VRHS      = 0xC3,
    VDVS      = 0xC4,
    FRCTRL2   = 0xC6,
    PWCTRL1   = 0xD0,
    PORCTRL   = 0xB2,
    PVGAMCTRL = 0xE0,
    NVGAMCTRL = 0xE1,
    INVOFF    = 0x20,
    SLPOUT    = 0x11,
    DISPON    = 0x29,
    GAMSET    = 0x26,
    DISPOFF   = 0x28,
    RAMWR     = 0x2C,
    INVON     = 0x21,
    CASET     = 0x2A,
    RASET     = 0x2B,
    STE       = 0x44
  };

  static PIO pio = pio0;
  static uint pio_sm = 0;
  static uint pio_offset = 0, pio_double_offset = 0;

  static uint32_t dma_channel = 0;

  // screen properties
  static const uint16_t width = DISPLAY_WIDTH;
  static const uint16_t height = DISPLAY_HEIGHT;

  static uint16_t win_w, win_h; // window size

#ifdef PIMORONI_PICOSYSTEM
  #define CS        PICOSYSTEM_LCD_CSN_PIN
  #define DC        PICOSYSTEM_LCD_DC_PIN
  #define SCK       PICOSYSTEM_LCD_SCLK_PIN
  #define MOSI      PICOSYSTEM_LCD_MOSI_PIN
  #define BACKLIGHT PICOSYSTEM_BACKLIGHT_PIN
  #define VSYNC     PICOSYSTEM_LCD_VSYNC_PIN
  #define RESET     PICOSYSTEM_LCD_RESET_PIN
#elif defined(WAVESHARE_PICO_LCD_1_3)
  #define CS        WAVESHARE_PICO_LCD_1_3_LCD_CS_PIN
  #define DC        WAVESHARE_PICO_LCD_1_3_LCD_DC_PIN
  #define SCK       WAVESHARE_PICO_LCD_1_3_LCD_SCLK_PIN
  #define MOSI      WAVESHARE_PICO_LCD_1_3_LCD_TX_PIN
  #define BACKLIGHT WAVESHARE_PICO_LCD_1_3_LCD_BL_PIN
  #define RESET     WAVESHARE_PICO_LCD_1_3_LCD_RST_PIN
#else
  // #define CS        PICO_DEFAULT_SPI_CSN_PIN
  // #define DC        16
  // #define SCK       PICO_DEFAULT_SPI_SCK_PIN
  // #define MOSI      PICO_DEFAULT_SPI_TX_PIN
  // #define BACKLIGHT 20
  
  #define CS        9
  #define DC        8
  #define SCK       10
  #define MOSI      11
  #define BACKLIGHT 13
  #define RESET     12
#endif

  static bool write_mode = false; // in RAMWR
  static bool pixel_sesquialteral = false;
  static uint16_t *upd_frame_buffer = nullptr;

  // frame buffer where pixel data is stored
  uint16_t *frame_buffer = nullptr;

  // pixel double scanline counter
  static volatile int cur_scanline = 240;

  void prepare_write();

  // PIO helpers
  static void pio_put_byte(PIO pio, uint sm, uint8_t b) {
    while (pio_sm_is_tx_fifo_full(pio, sm));
    *(volatile uint8_t*)&pio->txf[sm] = b;
  }

  static void pio_wait(PIO pio, uint sm) {
    uint32_t stall_mask = 1u << (PIO_FDEBUG_TXSTALL_LSB + sm);
    pio->fdebug |= stall_mask;
    while(!(pio->fdebug & stall_mask));
  }

  // used for pixel doubling
  static void __isr st7789_dma_irq_handler() {
    if(dma_channel_get_irq0_status(dma_channel)) {
      dma_channel_acknowledge_irq0(dma_channel);

      if(++cur_scanline > win_h / 3 * 2)
        return;

      // Every 2 pixels (2 * 16 bits) become 3 pixels (new_w = win_w / 3 * 2) which are passed as 32 bits
      // trans_count = win_w / 3 * 2 * 16bits / 32bits = win_w / 3
      // For every second line we're passing twice as much
      bool even_line = not (cur_scanline % 2);
      bool leakage = cur_scanline == ((win_h + 1) / 3 * 2);
      auto count = (even_line and not leakage) ? win_w / 3 * 2 : win_w / 3;
      
      int shift = (cur_scanline - 1) * (win_w / 3 * 2);

      dma_channel_set_trans_count(dma_channel, count, false);
      dma_channel_set_read_addr(dma_channel, frame_buffer + shift, true);
    }
  }

  void init(bool auto_init_sequence) {
    // configure pins
    gpio_set_function(DC, GPIO_FUNC_SIO);
    gpio_set_dir(DC, GPIO_OUT);

    gpio_set_function(CS, GPIO_FUNC_SIO);
    gpio_set_dir(CS, GPIO_OUT);

    bi_decl_if_func_used(bi_1pin_with_name(DC, "Display D/C"));
    bi_decl_if_func_used(bi_1pin_with_name(CS, "Display CS"));

    // if supported by the display then the vsync pin is
    // toggled high during vertical blanking period
#ifdef VSYNC
    gpio_set_function(VSYNC, GPIO_FUNC_SIO);
    gpio_set_dir(VSYNC, GPIO_IN);
    gpio_set_pulls(VSYNC, false, true);

    bi_decl_if_func_used(bi_1pin_with_name(VSYNC, "Display TE/VSync"));
#endif

    // if a backlight pin is provided then set it up for
    // pwm control
#ifdef BACKLIGHT
    pwm_config pwm_cfg = pwm_get_default_config();
    pwm_set_wrap(pwm_gpio_to_slice_num(BACKLIGHT), 65535);
    pwm_init(pwm_gpio_to_slice_num(BACKLIGHT), &pwm_cfg, true);
    gpio_set_function(BACKLIGHT, GPIO_FUNC_PWM);

    bi_decl_if_func_used(bi_1pin_with_name(BACKLIGHT, "Display Backlight"));
#endif

#ifdef RESET
    gpio_set_function(RESET, GPIO_FUNC_SIO);
    gpio_set_dir(RESET, GPIO_OUT);
    gpio_put(RESET, 0);
    sleep_ms(100);
    gpio_put(RESET, 1);

    bi_decl_if_func_used(bi_1pin_with_name(RESET, "Display Reset"));
#endif

    // setup PIO
    pio_offset = pio_add_program(pio, &st7789_raw_program);
    pio_double_offset = pio_add_program(pio, &st7789_pixel_sesquialteral_program);

    pio_sm = pio_claim_unused_sm(pio, true);

    pio_sm_config cfg = st7789_raw_program_get_default_config(pio_offset);
#if OVERCLOCK_250
    sm_config_set_clkdiv(&cfg, 2); // back to 62.5MHz from overclock
#endif
    sm_config_set_out_shift(&cfg, false, true, 8);
    sm_config_set_out_pins(&cfg, MOSI, 1);
    sm_config_set_fifo_join(&cfg, PIO_FIFO_JOIN_TX);
    sm_config_set_sideset_pins(&cfg, SCK);

    pio_gpio_init(pio, MOSI);
    pio_gpio_init(pio, SCK);
    pio_sm_set_consecutive_pindirs(pio, pio_sm, MOSI, 1, true);
    pio_sm_set_consecutive_pindirs(pio, pio_sm, SCK, 1, true);

    pio_sm_init(pio, pio_sm, pio_offset, &cfg);
    pio_sm_set_enabled(pio, pio_sm, true);

    bi_decl_if_func_used(bi_1pin_with_name(MOSI, "Display TX"));
    bi_decl_if_func_used(bi_1pin_with_name(SCK, "Display SCK"));

    // if auto_init_sequence then send initialisation sequence
    // for our standard displays based on the width and height
    if(auto_init_sequence) {
      command(reg::SWRESET);

      sleep_ms(150);

      command(reg::TEON,      1, "\x00");  // enable frame sync signal if used
      command(reg::COLMOD,    1, "\x05");  // 16 bits per pixel

      if(width == 240 && height == 240) {
        command(reg::PORCTRL, 5, "\x0c\x0c\x00\x33\x33");
        command(reg::GCTRL, 1, "\x14");
        command(reg::VCOMS, 1, "\x37");
        command(reg::LCMCTRL, 1, "\x2c");
        command(reg::VDVVRHEN, 1, "\x01");
        command(reg::VRHS, 1, "\x12");
        command(reg::VDVS, 1, "\x20");
        command(reg::PWCTRL1, 2, "\xa4\xa1");

        command(reg::PVGAMCTRL, 14, "\xD0\x08\x11\x08\x0c\x15\x39\x33\x50\x36\x13\x14\x29\x2d");
        command(reg::NVGAMCTRL, 14, "\xD0\x08\x10\x08\x06\x06\x39\x44\x51\x0b\x16\x14\x2f\x31");


        // trigger "vsync" slightly earlier to avoid tearing while pixel-doubling
        // (this is still outside of the visible part of the screen)
        command(reg::STE, 2, "\x01\x2C");
      }

      if(width == 320 && height == 240) {
        command(reg::PORCTRL, 5, "\x0c\x0c\x00\x33\x33");
        command(reg::GCTRL, 1, "\x74");
        command(reg::VCOMS, 1, "\x2f");
        command(reg::LCMCTRL, 1, "\2c");
        command(reg::LCMCTRL, 1, "\x22");
        command(reg::VDVVRHEN, 1, "\x01");
        command(reg::VRHS, 1, "\x19");
        command(reg::VDVS, 1, "\x20");
        command(reg::PWCTRL1, 2, "\xa4\xa1");
        command(0xd6, 1, "\xa1"); // ???
        command(reg::PVGAMCTRL, 14, "\xF0\x08\x0F\x0B\x0B\x07\x34\x43\x4B\x38\x14\x13\x2C\x31");
        command(reg::NVGAMCTRL, 14, "\xF0\x0C\x11\x09\x08\x24\x34\x33\x4A\x3A\x16\x16\x2E\x32");
      }

      command(reg::FRCTRL2, 1, "\x15"); // 50Hz

      command(reg::INVON);   // set inversion mode
      command(reg::SLPOUT);  // leave sleep mode
      command(reg::DISPON);  // turn display on

      sleep_ms(100);

      // setup correct addressing window
      uint8_t madctl = MADCTL::RGB;
      if(width == 240 && height == 240) {
        madctl |= MADCTL::COL_ORDER | MADCTL::SWAP_XY | MADCTL::SCAN_ORDER;;
        set_window(0, 0, 240, 240);
      }

      if(width == 240 && height == 135) {
        madctl |= MADCTL::COL_ORDER | MADCTL::SWAP_XY | MADCTL::SCAN_ORDER;
        set_window(40, 53, 240, 135);
      }

      if(width == 320 && height == 240)
        set_window(0, 0, 320, 240);

      command(reg::MADCTL,    1, (char *)&madctl);
    }

    // initialise dma channel for transmitting pixel data to screen
    dma_channel = dma_claim_unused_channel(true);
    dma_channel_config config = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_16);
    channel_config_set_dreq(&config, pio_get_dreq(pio, pio_sm, true));
    dma_channel_configure(
      dma_channel, &config, &pio->txf[pio_sm], frame_buffer, width * height, false);

    irq_add_shared_handler(DMA_IRQ_0, st7789_dma_irq_handler, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    irq_set_enabled(DMA_IRQ_0, true);
  }

  void command(uint8_t command, size_t len, const char *data) {
    pio_wait(pio, pio_sm);

    if(write_mode) {
      // reconfigure to 8 bits
      pio_sm_set_enabled(pio, pio_sm, false);
      pio->sm[pio_sm].shiftctrl &= ~PIO_SM0_SHIFTCTRL_PULL_THRESH_BITS;
      pio->sm[pio_sm].shiftctrl |= (8 << PIO_SM0_SHIFTCTRL_PULL_THRESH_LSB) | PIO_SM0_SHIFTCTRL_AUTOPULL_BITS;

      // switch back to raw
      pio_sm_restart(pio, pio_sm);
      pio_sm_set_wrap(pio, pio_sm, pio_offset + st7789_raw_wrap_target, pio_offset + st7789_raw_wrap);
      pio_sm_exec(pio, pio_sm, pio_encode_jmp(pio_offset));

      pio_sm_set_enabled(pio, pio_sm, true);
      write_mode = false;
    }

    gpio_put(CS, 0);

    gpio_put(DC, 0); // command mode
    pio_put_byte(pio, pio_sm, command);

    if(data) {
      pio_wait(pio, pio_sm);
      gpio_put(DC, 1); // data mode

      for(size_t i = 0; i < len; i++)
        pio_put_byte(pio, pio_sm, data[i]);
    }

    pio_wait(pio, pio_sm);
    gpio_put(CS, 1);
  }

  void update(bool dont_block) {
    if(dma_channel_is_busy(dma_channel) && dont_block) {
      return;
    }

    dma_channel_wait_for_finish_blocking(dma_channel);

    if(!write_mode)
      prepare_write();

    if(pixel_sesquialteral) {
      cur_scanline = 0;
      upd_frame_buffer = frame_buffer;
      dma_channel_set_trans_count(dma_channel, win_w / 3, false);
    } else
      dma_channel_set_trans_count(dma_channel, win_w * win_h, false);

    dma_channel_set_read_addr(dma_channel, frame_buffer, true);
  }

  void set_backlight(uint8_t brightness) {
#ifdef BACKLIGHT
    // gamma correct the provided 0-255 brightness value onto a
    // 0-65535 range for the pwm counter
    float gamma = 2.8;
    uint16_t value = (uint16_t)(pow((float)(brightness) / 255.0f, gamma) * 65535.0f + 0.5f);
    pwm_set_gpio_level(BACKLIGHT, value);
#endif
  }

  bool vsync_callback(gpio_irq_callback_t callback) {
#ifdef VSYNC
    gpio_set_irq_enabled_with_callback(VSYNC, GPIO_IRQ_EDGE_RISE, true, callback);
    return true;
#else
    return false;
#endif
  }

  void set_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    uint32_t cols = __builtin_bswap32((x << 16) | (x + w - 1));
    uint32_t rows = __builtin_bswap32((y << 16) | (y + h - 1));

    command(reg::CASET, 4, (const char *)&cols);
    command(reg::RASET, 4, (const char *)&rows);

    win_w = w;
    win_h = h;
  }

  void set_pixel_sesquialteral(bool pd) {
    pixel_sesquialteral = pd;

    // nop to reconfigure PIO
    if(write_mode)
      command(0);

    if(pixel_sesquialteral) {
      dma_channel_acknowledge_irq0(dma_channel);
      dma_channel_set_irq0_enabled(dma_channel, true);
    } else
      dma_channel_set_irq0_enabled(dma_channel, false);
  }

  void clear() {
    if(!write_mode)
      prepare_write();

    for(int i = 0; i < win_w * win_h; i++)
      pio_sm_put_blocking(pio, pio_sm, 0);
  }

  bool dma_is_busy() {
    if(pixel_sesquialteral && cur_scanline <= win_h / 1.5)
      return true;

    return dma_channel_is_busy(dma_channel);
  }

  void prepare_write() {
    pio_wait(pio, pio_sm);

    // setup for writing
    uint8_t r = reg::RAMWR;
    gpio_put(CS, 0);

    gpio_put(DC, 0); // command mode
    pio_put_byte(pio, pio_sm, r);
    pio_wait(pio, pio_sm);

    gpio_put(DC, 1); // data mode

    pio_sm_set_enabled(pio, pio_sm, false);
    pio_sm_restart(pio, pio_sm);

    if(pixel_sesquialteral) {
      // switch program
      pio_sm_set_wrap(pio, pio_sm, pio_double_offset + st7789_pixel_sesquialteral_wrap_target, pio_double_offset + st7789_pixel_sesquialteral_wrap);

      // 32 bits, no autopull
      pio->sm[pio_sm].shiftctrl &= ~(PIO_SM0_SHIFTCTRL_PULL_THRESH_BITS | PIO_SM0_SHIFTCTRL_AUTOPULL_BITS);

      pio_sm_exec(pio, pio_sm, pio_encode_jmp(pio_double_offset));

      // reconfigure dma size
      dma_channel_hw_addr(dma_channel)->al1_ctrl &= ~DMA_CH0_CTRL_TRIG_DATA_SIZE_BITS;
      dma_channel_hw_addr(dma_channel)->al1_ctrl |= DMA_SIZE_32 << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB;
    } else {
      // 16 bits, autopull
      pio->sm[pio_sm].shiftctrl &= ~PIO_SM0_SHIFTCTRL_PULL_THRESH_BITS;
      pio->sm[pio_sm].shiftctrl |= (16 << PIO_SM0_SHIFTCTRL_PULL_THRESH_LSB) | PIO_SM0_SHIFTCTRL_AUTOPULL_BITS;

      dma_channel_hw_addr(dma_channel)->al1_ctrl &= ~DMA_CH0_CTRL_TRIG_DATA_SIZE_BITS;
      dma_channel_hw_addr(dma_channel)->al1_ctrl |= DMA_SIZE_16 << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB;
    }

    pio_sm_set_enabled(pio, pio_sm, true);

    write_mode = true;
  }
}
