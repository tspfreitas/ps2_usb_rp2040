// LED de status.
//   Verde  fixo : conectado ao computador (USB enumerada), nenhum controle ligado
//   Azul   fixo : conectado ao computador e pelo menos 1 controle PS1/PS2 detectado
//   Amarelo fixo: USB suspensa (computador dormindo)
//   Vermelho piscando: sem enumeração USB (ligado, mas o computador não reconheceu)
#pragma once
#include <stdint.h>

// 0 = sem LED | 1 = WS2812 da RP2040-Zero (GP16) | 2 = LED comum (ex.: Pico, GP25)
#ifndef STATUS_LED_MODE
#define STATUS_LED_MODE 1
#endif
#ifndef STATUS_LED_PIN
#if STATUS_LED_MODE == 2
#define STATUS_LED_PIN 25
#else
#define STATUS_LED_PIN 16
#endif
#endif
// Brilho (0..255). O WS2812 da Zero é forte; 24 já é bem visível.
#ifndef STATUS_LED_LEVEL
#define STATUS_LED_LEVEL 24
#endif
// 1 = azul quando há controle conectado; 0 = sempre verde quando conectado ao PC
#ifndef STATUS_LED_SHOW_PADS
#define STATUS_LED_SHOW_PADS 1
#endif

void led_init(void);
void led_task(int pads_connected);   // chamar a cada volta do laço principal
