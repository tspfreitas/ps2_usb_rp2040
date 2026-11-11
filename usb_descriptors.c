#include <string.h>
#include "tusb.h"
#include "pico/unique_id.h"

/* VID/PID de teste do TinyUSB. Troque se for distribuir o adaptador. */
#ifndef USB_VID
#define USB_VID 0xCafe
#endif
#ifndef USB_PID
#define USB_PID 0x5053
#endif

// Gamepad: 12 botoes + hat + X,Y (analogico esq), Z,Rz (analogico dir), Slider,Dial (pressao L2/R2)
// Ordem dos botoes igual ao "Standard Gamepad" do navegador:
//  B1 X, B2 O, B3 Quad, B4 Tri, B5 L1, B6 R1, B7 L2, B8 R2, B9 Select, B10 Start, B11 L3, B12 R3
static const uint8_t desc_hid_report[] = {
  0x05,0x01, 0x09,0x05, 0xA1,0x01,
    0x05,0x09, 0x19,0x01, 0x29,0x0C, 0x15,0x00, 0x25,0x01,
    0x75,0x01, 0x95,0x0C, 0x81,0x02,                  // 12 botoes
    0x75,0x01, 0x95,0x04, 0x81,0x03,                  // padding
    0x05,0x01, 0x09,0x39, 0x15,0x00, 0x25,0x07,
    0x35,0x00, 0x46,0x3B,0x01, 0x65,0x14,
    0x75,0x04, 0x95,0x01, 0x81,0x42,                  // hat (8 = neutro)
    0x75,0x04, 0x95,0x01, 0x81,0x03,                  // padding
    0x65,0x00, 0x35,0x00, 0x45,0x00,
    0x05,0x01, 0x09,0x30, 0x09,0x31, 0x09,0x32, 0x09,0x35, 0x09,0x36, 0x09,0x37,
    0x15,0x00, 0x26,0xFF,0x00, 0x75,0x08, 0x95,0x06, 0x81,0x02,  // X Y Z Rz Slider Dial
  0xC0
};

static const tusb_desc_device_t desc_device = {
  .bLength = sizeof(tusb_desc_device_t), .bDescriptorType = TUSB_DESC_DEVICE,
  .bcdUSB = 0x0200, .bDeviceClass = 0x00, .bDeviceSubClass = 0x00, .bDeviceProtocol = 0x00,
  .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
  .idVendor = USB_VID, .idProduct = USB_PID, .bcdDevice = 0x0100,
  .iManufacturer = 1, .iProduct = 2, .iSerialNumber = 3, .bNumConfigurations = 1
};

#define EPNUM_HID0 0x81
#define EPNUM_HID1 0x82
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + 2 * TUD_HID_DESC_LEN)

static const uint8_t desc_configuration[] = {
  TUD_CONFIG_DESCRIPTOR(1, 2, 0, CONFIG_TOTAL_LEN, 0x00, 200),
  TUD_HID_DESCRIPTOR(0, 4, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID0, 16, 1),
  TUD_HID_DESCRIPTOR(1, 5, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID1, 16, 1),
};

static const char *const string_desc[] = {
  "",                        // 0: idioma (tratado abaixo)
  "DIY",                      // 1: fabricante
  "PS1/PS2 Controller USB",  // 2: produto
  "",                        // 3: serial (id unico do chip)
  "Player 1",                // 4
  "Player 2",                // 5
};

const uint8_t *tud_descriptor_device_cb(void) { return (const uint8_t *)&desc_device; }
const uint8_t *tud_descriptor_configuration_cb(uint8_t index) { (void)index; return desc_configuration; }
const uint8_t *tud_hid_descriptor_report_cb(uint8_t instance) { (void)instance; return desc_hid_report; }

const uint16_t *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;
  static uint16_t buf[32];
  char serial[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
  const char *s;
  uint8_t n;

  if (index == 0) { buf[1] = 0x0409; n = 1; }
  else {
    if (index == 3) { pico_get_unique_board_id_string(serial, sizeof serial); s = serial; }
    else if (index < sizeof(string_desc) / sizeof(string_desc[0])) s = string_desc[index];
    else return NULL;
    n = (uint8_t)strlen(s);
    if (n > 31) n = 31;
    for (uint8_t i = 0; i < n; i++) buf[1 + i] = (uint16_t)s[i];
  }
  buf[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * n + 2));
  return buf;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen) {
  (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)reqlen;
  return 0;
}
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                           const uint8_t *buffer, uint16_t bufsize) {
  (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)bufsize;
}
