#include "led.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "tusb.h"

#if STATUS_LED_MODE == 1
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

static PIO  led_pio = pio0;
static uint led_sm;

static void put_rgb(uint8_t r, uint8_t g, uint8_t b) {
  // WS2812 usa ordem GRB; o PIO desloca 24 bits a partir do bit mais alto
  pio_sm_put_blocking(led_pio, led_sm, ((uint32_t)g << 24) | ((uint32_t)r << 16) | ((uint32_t)b << 8));
}
#elif STATUS_LED_MODE == 2
static void put_rgb(uint8_t r, uint8_t g, uint8_t b) {
  gpio_put(STATUS_LED_PIN, (r | g | b) != 0);
}
#else
static void put_rgb(uint8_t r, uint8_t g, uint8_t b) { (void)r; (void)g; (void)b; }
#endif

void led_init(void) {
#if STATUS_LED_MODE == 1
  uint offset = pio_add_program(led_pio, &ws2812_program);
  led_sm = pio_claim_unused_sm(led_pio, true);

  pio_gpio_init(led_pio, STATUS_LED_PIN);
  pio_sm_set_consecutive_pindirs(led_pio, led_sm, STATUS_LED_PIN, 1, true);

  pio_sm_config c = ws2812_program_get_default_config(offset);
  sm_config_set_sideset_pins(&c, STATUS_LED_PIN);
  sm_config_set_out_shift(&c, false, true, 24);          // MSB primeiro, autopull em 24 bits
  sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
  float div = (float)clock_get_hz(clk_sys) / (800000.0f * (ws2812_T1 + ws2812_T2 + ws2812_T3));
  sm_config_set_clkdiv(&c, div);
  pio_sm_init(led_pio, led_sm, offset, &c);
  pio_sm_set_enabled(led_pio, led_sm, true);
#elif STATUS_LED_MODE == 2
  gpio_init(STATUS_LED_PIN);
  gpio_set_dir(STATUS_LED_PIN, GPIO_OUT);
#endif
  put_rgb(0, 0, 0);
}

void led_task(int pads_connected) {
  static uint32_t last = 0xFFFFFFFFu;
  uint32_t ms = to_ms_since_boot(get_absolute_time());
  uint8_t r = 0, g = 0, b = 0;

  if (tud_suspended()) {                 // computador dormindo: amarelo
    r = STATUS_LED_LEVEL; g = STATUS_LED_LEVEL;
  } else if (!tud_mounted()) {           // sem enumeração: vermelho piscando
    if ((ms / 500) & 1) r = STATUS_LED_LEVEL;
  } else if (STATUS_LED_SHOW_PADS && pads_connected > 0) {
    b = STATUS_LED_LEVEL;                // conectado + controle detectado
  } else {
    g = STATUS_LED_LEVEL;                // conectado ao computador
  }

  uint32_t color = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
  if (color != last) {                   // só reescreve quando muda
    last = color;
    put_rgb(r, g, b);
  }
}
