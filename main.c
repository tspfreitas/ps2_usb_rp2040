// PS1/PS2 -> USB (2 controles simultaneos) para RP2040
#include <string.h>
#include "pico/stdlib.h"
#include "tusb.h"
#include "psx.h"

typedef struct __attribute__((packed)) {
  uint16_t buttons;   // bits 0..11 (ver usb_descriptors.c)
  uint8_t  hat;       // 0..7 = N,NE,L,SE,S,SO,O,NO ; 8 = neutro
  uint8_t  x, y, z, rz, slider, dial;
} hid_report_t;

#define B(p, bit) (((p)->buttons >> (bit)) & 1u)

static void build_report(const psx_pad_t *p, hid_report_t *r) {
  memset(r, 0, sizeof *r);
  r->hat = 8; r->x = r->y = r->z = r->rz = 128;
  if (!p->connected) return;

  r->buttons = (uint16_t)(
      B(p, PSX_CROSS)    << 0  | B(p, PSX_CIRCLE) << 1  | B(p, PSX_SQUARE) << 2  |
      B(p, PSX_TRIANGLE) << 3  | B(p, PSX_L1)     << 4  | B(p, PSX_R1)     << 5  |
      B(p, PSX_L2)       << 6  | B(p, PSX_R2)     << 7  | B(p, PSX_SELECT) << 8  |
      B(p, PSX_START)    << 9  | B(p, PSX_L3)     << 10 | B(p, PSX_R3)     << 11);

  bool up = B(p, PSX_UP), dn = B(p, PSX_DOWN), lf = B(p, PSX_LEFT), rt = B(p, PSX_RIGHT);
  if (up && dn) up = dn = false;
  if (lf && rt) lf = rt = false;
  r->hat = (up && rt) ? 1 : (rt && dn) ? 3 : (dn && lf) ? 5 : (lf && up) ? 7 :
           up ? 0 : rt ? 2 : dn ? 4 : lf ? 6 : 8;

  r->x = p->lx; r->y = p->ly; r->z = p->rx; r->rz = p->ry;
  r->slider = p->l2; r->dial = p->r2;
}

int main(void) {
  psx_init();
  tusb_init();

  psx_pad_t pad[PSX_PORTS];
  hid_report_t rep[PSX_PORTS];
  memset(pad, 0, sizeof pad);
  for (int i = 0; i < PSX_PORTS; i++) build_report(&pad[i], &rep[i]);
  absolute_time_t next = get_absolute_time();
  uint8_t small[PSX_PORTS] = {0}, large[PSX_PORTS] = {0};

  while (true) {
    tud_task();

    if (time_reached(next)) {
      next = delayed_by_us(next, 4000);            // ~250 Hz
      for (int i = 0; i < PSX_PORTS; i++) {
        psx_poll(i, &pad[i], small[i], large[i]);
        build_report(&pad[i], &rep[i]);

        // Teste de vibracao (reparo): SELECT+START+L1 = motor pequeno, SELECT+START+R1 = motor grande
        bool combo = B(&pad[i], PSX_SELECT) && B(&pad[i], PSX_START);
        small[i] = (combo && B(&pad[i], PSX_L1)) ? 0xFF : 0x00;
        large[i] = (combo && B(&pad[i], PSX_R1)) ? 0xFF : 0x00;
      }
    }
    for (int i = 0; i < PSX_PORTS; i++)
      if (tud_hid_n_ready(i)) tud_hid_n_report(i, 0, &rep[i], sizeof rep[i]);
  }
}
