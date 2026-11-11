// Leitura de controles PS1/PS2 (protocolo SPI modo 3, LSB primeiro) via SPI0 do RP2040.
// Dois "slots" compartilham SCK/CMD/DAT e tem ATT separado (igual ao console).
#pragma once
#include <stdbool.h>
#include <stdint.h>

#define PSX_PORTS 2

// Pinagem (RP2040-Zero e Pico: GP2..GP6). Pode ser sobrescrita no CMake com -D.
#ifndef PSX_PIN_SCK
#define PSX_PIN_SCK  2   // CLK  (pino 7 do conector)  SPI0 SCK
#endif
#ifndef PSX_PIN_CMD
#define PSX_PIN_CMD  3   // CMD  (pino 2)              SPI0 TX
#endif
#ifndef PSX_PIN_DAT
#define PSX_PIN_DAT  4   // DATA (pino 1) + pull-up 1k SPI0 RX
#endif
#ifndef PSX_PIN_ATT0
#define PSX_PIN_ATT0 5   // ATT porta 1 (pino 6)
#endif
#ifndef PSX_PIN_ATT1
#define PSX_PIN_ATT1 6   // ATT porta 2 (pino 6)
#endif

#ifndef PSX_BAUD
#define PSX_BAUD         250000  // clock do barramento
#endif
#ifndef PSX_BYTE_GAP_US
#define PSX_BYTE_GAP_US  20      // pausa entre bytes (aumente p/ 30-40 se o controle falhar)
#endif

// Bits de psx_pad_t.buttons (1 = pressionado)
enum {
  PSX_SELECT = 0, PSX_L3, PSX_R3, PSX_START, PSX_UP, PSX_RIGHT, PSX_DOWN, PSX_LEFT,
  PSX_L2, PSX_R2, PSX_L1, PSX_R1, PSX_TRIANGLE, PSX_CIRCLE, PSX_CROSS, PSX_SQUARE
};

typedef struct {
  bool     connected;
  uint8_t  id;             // 0x41 digital, 0x73 analogico, 0x79 analogico+pressao, 0x53 joystick analogico
  uint16_t buttons;        // bits PSX_* (1 = pressionado)
  uint8_t  lx, ly, rx, ry; // 0..255, centro ~128
  uint8_t  l2, r2;         // pressao analogica (DualShock 2) ou 0/255
  bool     has_pressure;
} psx_pad_t;

void psx_init(void);
// Le um controle. small: motor pequeno (0x00/0xFF), large: motor grande (0x00..0xFF).
// Retorna true se ha controle conectado.
bool psx_poll(int port, psx_pad_t *pad, uint8_t small, uint8_t large);
