#include "psx.h"
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#define PSX_SPI spi0
#define PSX_ATT_DELAY_US   20
#define PSX_POST_DELAY_US  100
#define PSX_MAX_FRAME      21   // 3 cabecalho + 18 dados (DualShock 2 com pressao)

static const uint att_pin[PSX_PORTS] = { PSX_PIN_ATT0, PSX_PIN_ATT1 };
static bool            was_conn[PSX_PORTS];
static absolute_time_t retry_at[PSX_PORTS];
static absolute_time_t mode_until[PSX_PORTS];
static uint8_t         prev_id[PSX_PORTS];

// O SPI do RP2040 e MSB primeiro; o PSX e LSB primeiro -> inverte os bits.
static inline uint8_t rev8(uint8_t b) {
  b = (uint8_t)(((b & 0xF0) >> 4) | ((b & 0x0F) << 4));
  b = (uint8_t)(((b & 0xCC) >> 2) | ((b & 0x33) << 2));
  b = (uint8_t)(((b & 0xAA) >> 1) | ((b & 0x55) << 1));
  return b;
}

static uint8_t xfer(uint8_t tx) {
  uint8_t t = rev8(tx), r = 0xFF;
  spi_write_read_blocking(PSX_SPI, &t, &r, 1);
  busy_wait_us_32(PSX_BYTE_GAP_US);
  return rev8(r);
}

static void att(int port, bool active) {
  gpio_put(att_pin[port], !active);
  if (active) busy_wait_us_32(PSX_ATT_DELAY_US);
  else        busy_wait_us_32(PSX_POST_DELAY_US);
}

// Comando de tamanho fixo (configuracao)
static void txn_fixed(int port, const uint8_t *tx, uint8_t *rx, int n) {
  att(port, true);
  for (int i = 0; i < n; i++) rx[i] = xfer(tx[i]);
  att(port, false);
}

// Poll 0x42 de tamanho variavel: o ID (byte 1) diz quantos bytes de dados vem.
// Retorna o total de bytes lidos, ou 0 se nao ha controle.
static int poll_raw(int port, uint8_t small, uint8_t large, uint8_t *rx) {
  int total = 3;
  att(port, true);
  rx[0] = xfer(0x01);
  rx[1] = xfer(0x42);
  rx[2] = xfer(0x00);
  if (rx[2] != 0x5A || rx[1] == 0xFF || rx[1] == 0x00) {   // sem controle: DAT fica em 1 pelo pull-up
    att(port, false);
    return 0;
  }
  total += (rx[1] & 0x0F) * 2;
  if (total > PSX_MAX_FRAME) total = PSX_MAX_FRAME;
  for (int i = 3; i < total; i++)
    rx[i] = xfer(i == 3 ? small : (i == 4 ? large : 0x00));
  att(port, false);
  return total;
}

// Tenta colocar um DualShock em modo analogico (travado) com pressao e motores mapeados.
// Em controle digital PS1 (sem modo de configuracao) nao faz nada.
static void try_configure(int port) {
  uint8_t rx[9];
  // 0x44: byte 3 = 1 (analogico); byte 4 = 3 trava o modo, 0 deixa o botao ANALOG funcionar
  const uint8_t analog[9] = {0x01,0x44,0x00,0x01,PSX_LOCK_ANALOG ? 0x03 : 0x00,0x00,0x00,0x00,0x00};
  static const uint8_t enter[9]  = {0x01,0x43,0x00,0x01,0x00,0x00,0x00,0x00,0x00};
  static const uint8_t query[9]  = {0x01,0x45,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
  static const uint8_t press[9]  = {0x01,0x4F,0x00,0xFF,0xFF,0x03,0x00,0x00,0x00};
  static const uint8_t motors[9] = {0x01,0x4D,0x00,0x00,0x01,0xFF,0xFF,0xFF,0xFF};
  static const uint8_t leave[9]  = {0x01,0x43,0x00,0x00,0x5A,0x5A,0x5A,0x5A,0x5A};

  txn_fixed(port, enter, rx, 9);
  sleep_us(500);
  txn_fixed(port, query, rx, 9);
  if (rx[1] != 0xF3) return;                 // nao entrou em config: controle digital
  sleep_us(500); txn_fixed(port, analog, rx, 9);
  sleep_us(500); txn_fixed(port, press,  rx, 9);
  sleep_us(500); txn_fixed(port, motors, rx, 9);
  sleep_us(500); txn_fixed(port, leave,  rx, 9);
  sleep_ms(2);
}

void psx_init(void) {
  spi_init(PSX_SPI, PSX_BAUD);
  spi_set_format(PSX_SPI, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
  gpio_set_function(PSX_PIN_SCK, GPIO_FUNC_SPI);
  gpio_set_function(PSX_PIN_CMD, GPIO_FUNC_SPI);
  gpio_set_function(PSX_PIN_DAT, GPIO_FUNC_SPI);
  gpio_pull_up(PSX_PIN_DAT);                 // reforco; o pull-up externo de 1k e obrigatorio
  for (int i = 0; i < PSX_PORTS; i++) {
    gpio_init(att_pin[i]);
    gpio_set_dir(att_pin[i], GPIO_OUT);
    gpio_put(att_pin[i], 1);
    retry_at[i] = get_absolute_time();
  }
}

bool psx_poll(int port, psx_pad_t *p, uint8_t small, uint8_t large) {
  uint8_t rx[PSX_MAX_FRAME + 1];
  int n = poll_raw(port, small, large, rx);

  if (n == 0) {
    memset(p, 0, sizeof *p);
    p->lx = p->ly = p->rx = p->ry = 128;
    was_conn[port] = false;
    prev_id[port] = 0;
    return false;
  }
  if (!was_conn[port]) { was_conn[port] = true; retry_at[port] = get_absolute_time(); }

  // Botao ANALOG do DualShock (modo destravado): o controle volta para digital (0x41).
  // Gera um pulso no botao virtual "Mode" e reconfigura para analogico na hora.
  if ((prev_id[port] == 0x73 || prev_id[port] == 0x79) && rx[1] == 0x41) {
    mode_until[port] = make_timeout_time_ms(PSX_MODE_PULSE_MS);
    retry_at[port] = get_absolute_time();
  }

  // ID 0x41 = digital. Pode ser PS1 digital ou DualShock em modo digital: tenta configurar
  // (imediatamente na conexao e depois a cada 2 s se continuar 0x41).
  if (rx[1] == 0x41 && time_reached(retry_at[port])) {
    try_configure(port);
    retry_at[port] = make_timeout_time_ms(2000);
    n = poll_raw(port, small, large, rx);
    if (n == 0) { memset(p, 0, sizeof *p); p->lx = p->ly = p->rx = p->ry = 128; return false; }
  }

  uint8_t id = rx[1];
  prev_id[port] = id;
  int data = n - 3;
  memset(p, 0, sizeof *p);
  p->connected = true;
  p->id = id;
  p->mode = !time_reached(mode_until[port]);
  p->lx = p->ly = p->rx = p->ry = 128;
  if (data >= 2) p->buttons = (uint16_t)~(rx[3] | (rx[4] << 8));   // no fio: 0 = pressionado
  if (data >= 6 && (id == 0x73 || id == 0x53 || id == 0x79)) {
    p->rx = rx[5]; p->ry = rx[6]; p->lx = rx[7]; p->ly = rx[8];
  }
  if (id == 0x79 && data >= 18) {        // pressao: dir,esq,cima,baixo,tri,cir,x,quad,L1,R1,L2,R2
    p->l2 = rx[19]; p->r2 = rx[20]; p->has_pressure = true;
  } else {
    p->l2 = (p->buttons & (1u << PSX_L2)) ? 255 : 0;
    p->r2 = (p->buttons & (1u << PSX_R2)) ? 255 : 0;
  }
  return true;
}
