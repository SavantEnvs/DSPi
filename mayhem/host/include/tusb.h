/* Minimal TinyUSB device API surface used by DSPi (host stub). */
#ifndef DSPI_HOST_TUSB_H
#define DSPI_HOST_TUSB_H
#include "dspi_host.h"
#define TU_ATTR_PACKED __attribute__((packed))
#define TU_ATTR_ALIGNED(x) __attribute__((aligned(x)))
#define TU_ATTR_WEAK __attribute__((weak))
#define TU_ATTR_UNUSED __attribute__((unused))
#define TU_ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#define TU_MIN(a,b) ((a)<(b)?(a):(b))
#define TU_MAX(a,b) ((a)>(b)?(a):(b))
#define TU_U16_LOW(x) ((uint8_t)((x) & 0xff))
#define TU_U16_HIGH(x) ((uint8_t)(((x) >> 8) & 0xff))
#define U16_TO_U8S_LE(x) TU_U16_LOW(x), TU_U16_HIGH(x)
#define U32_TO_U8S_LE(x) (uint8_t)((x)&0xff),(uint8_t)(((x)>>8)&0xff),(uint8_t)(((x)>>16)&0xff),(uint8_t)(((x)>>24)&0xff)
#define TU_VERIFY(c, ...) do { if (!(c)) return __VA_ARGS__ +0 ? __VA_ARGS__ : false; } while(0)
#define TU_ASSERT(c, ...) TU_VERIFY(c, __VA_ARGS__)
#define TU_LOG1(...) do{}while(0)
#define TU_LOG2(...) do{}while(0)
#define CFG_TUD_ENDPOINT0_SIZE 64
#define TUD_OPT_RHPORT 0
#define BOARD_TUD_RHPORT 0
typedef enum { TUSB_DIR_OUT = 0, TUSB_DIR_IN = 1, TUSB_DIR_IN_MASK = 0x80 } tusb_dir_t;
typedef enum { TUSB_XFER_CONTROL = 0, TUSB_XFER_ISOCHRONOUS, TUSB_XFER_BULK, TUSB_XFER_INTERRUPT } tusb_xfer_type_t;
typedef enum { TUSB_REQ_TYPE_STANDARD = 0, TUSB_REQ_TYPE_CLASS, TUSB_REQ_TYPE_VENDOR, TUSB_REQ_TYPE_INVALID } tusb_request_type_t;
typedef enum { TUSB_REQ_RCPT_DEVICE = 0, TUSB_REQ_RCPT_INTERFACE, TUSB_REQ_RCPT_ENDPOINT, TUSB_REQ_RCPT_OTHER } tusb_request_recipient_t;
typedef enum { TUSB_REQ_GET_STATUS = 0, TUSB_REQ_CLEAR_FEATURE = 1, TUSB_REQ_SET_FEATURE = 3, TUSB_REQ_SET_ADDRESS = 5,
  TUSB_REQ_GET_DESCRIPTOR = 6, TUSB_REQ_SET_DESCRIPTOR = 7, TUSB_REQ_GET_CONFIGURATION = 8, TUSB_REQ_SET_CONFIGURATION = 9,
  TUSB_REQ_GET_INTERFACE = 10, TUSB_REQ_SET_INTERFACE = 11, TUSB_REQ_SYNCH_FRAME = 12 } tusb_request_code_t;
typedef enum { TUSB_DESC_DEVICE = 1, TUSB_DESC_CONFIGURATION, TUSB_DESC_STRING, TUSB_DESC_INTERFACE, TUSB_DESC_ENDPOINT,
  TUSB_DESC_DEVICE_QUALIFIER, TUSB_DESC_OTHER_SPEED_CONFIG, TUSB_DESC_INTERFACE_POWER, TUSB_DESC_OTG, TUSB_DESC_DEBUG,
  TUSB_DESC_INTERFACE_ASSOCIATION, TUSB_DESC_BOS = 15, TUSB_DESC_DEVICE_CAPABILITY = 16,
  TUSB_DESC_CS_DEVICE = 0x21, TUSB_DESC_CS_CONFIGURATION, TUSB_DESC_CS_STRING, TUSB_DESC_CS_INTERFACE, TUSB_DESC_CS_ENDPOINT } tusb_desc_type_t;
typedef enum { TUSB_CLASS_AUDIO = 1, TUSB_CLASS_HID = 3, TUSB_CLASS_MISC = 0xEF, TUSB_CLASS_VENDOR_SPECIFIC = 0xFF } tusb_class_code_t;
enum { CONTROL_STAGE_IDLE, CONTROL_STAGE_SETUP, CONTROL_STAGE_DATA, CONTROL_STAGE_ACK };
typedef enum { XFER_RESULT_SUCCESS, XFER_RESULT_FAILED, XFER_RESULT_STALLED, XFER_RESULT_TIMEOUT, XFER_RESULT_INVALID } xfer_result_t;
typedef struct TU_ATTR_PACKED {
  union { struct TU_ATTR_PACKED { uint8_t recipient : 5; uint8_t type : 2; uint8_t direction : 1; } bmRequestType_bit; uint8_t bmRequestType; };
  uint8_t bRequest; uint16_t wValue; uint16_t wIndex; uint16_t wLength;
} tusb_control_request_t;
typedef struct TU_ATTR_PACKED { uint8_t bLength, bDescriptorType; uint16_t bcdUSB; uint8_t bDeviceClass, bDeviceSubClass, bDeviceProtocol, bMaxPacketSize0; uint16_t idVendor, idProduct, bcdDevice; uint8_t iManufacturer, iProduct, iSerialNumber, bNumConfigurations; } tusb_desc_device_t;
typedef struct TU_ATTR_PACKED { uint8_t bLength, bDescriptorType, bInterfaceNumber, bAlternateSetting, bNumEndpoints, bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol, iInterface; } tusb_desc_interface_t;
typedef struct TU_ATTR_PACKED { uint8_t bLength, bDescriptorType, bEndpointAddress; struct TU_ATTR_PACKED { uint8_t xfer : 2; uint8_t sync : 2; uint8_t usage : 2; uint8_t : 2; } bmAttributes; uint16_t wMaxPacketSize; uint8_t bInterval; } tusb_desc_endpoint_t;
static inline uint8_t tu_desc_len(void const *d){ return ((uint8_t const *)d)[0]; }
static inline uint8_t tu_desc_type(void const *d){ return ((uint8_t const *)d)[1]; }
static inline uint8_t const *tu_desc_next(void const *d){ return (uint8_t const *)d + tu_desc_len(d); }
static inline uint8_t tu_edpt_number(uint8_t a){ return a & 0x7f; }
static inline uint8_t tu_edpt_dir(uint8_t a){ return (a & 0x80) ? TUSB_DIR_IN : TUSB_DIR_OUT; }
static inline uint8_t tu_edpt_addr(uint8_t n, uint8_t dir){ return (uint8_t)(n | (dir ? 0x80 : 0)); }
static inline uint16_t tu_edpt_packet_size(tusb_desc_endpoint_t const *d){ return d->wMaxPacketSize & 0x7ff; }
static inline uint16_t tu_u16(uint8_t h, uint8_t l){ return (uint16_t)((h << 8) | l); }
static inline uint8_t tu_u16_high(uint16_t v){ return (uint8_t)(v >> 8); }
static inline uint8_t tu_u16_low(uint16_t v){ return (uint8_t)v; }
typedef struct {
  char const *name;
  void (*init)(void);
  bool (*deinit)(void);
  void (*reset)(uint8_t rhport);
  uint16_t (*open)(uint8_t rhport, tusb_desc_interface_t const *desc_intf, uint16_t max_len);
  bool (*control_xfer_cb)(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request);
  bool (*xfer_cb)(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
  bool (*xfer_isr)(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
  void (*sof)(uint8_t rhport, uint32_t frame_count);
} usbd_class_driver_t;
/* Implemented by the harness */
bool tud_control_xfer(uint8_t rhport, tusb_control_request_t const *request, void *buffer, uint16_t len);
bool tud_control_status(uint8_t rhport, tusb_control_request_t const *request);
bool usbd_edpt_xfer(uint8_t rhport, uint8_t ep_addr, uint8_t *buffer, uint16_t total_bytes);
bool usbd_edpt_open(uint8_t rhport, tusb_desc_endpoint_t const *desc_ep);
bool usbd_edpt_iso_alloc(uint8_t rhport, uint8_t ep_addr, uint16_t largest_packet_size);
bool usbd_edpt_iso_activate(uint8_t rhport, tusb_desc_endpoint_t const *desc_ep);
void usbd_edpt_close(uint8_t rhport, uint8_t ep_addr);
void usbd_edpt_stall(uint8_t rhport, uint8_t ep_addr);
void usbd_edpt_clear_stall(uint8_t rhport, uint8_t ep_addr);
bool usbd_edpt_claim(uint8_t rhport, uint8_t ep_addr);
bool usbd_edpt_release(uint8_t rhport, uint8_t ep_addr);
bool usbd_edpt_busy(uint8_t rhport, uint8_t ep_addr);
typedef enum { SOF_CONSUMER_USER = 0, SOF_CONSUMER_AUDIO } sof_consumer_t;
void usbd_sof_enable(uint8_t rhport, sof_consumer_t c, bool en);
bool tud_mounted(void);
bool tud_ready(void);
bool tud_init(uint8_t rhport);
void tud_task(void);
bool tud_hid_n_report(uint8_t instance, uint8_t report_id, void const *report, uint16_t len);
static inline bool tud_hid_report(uint8_t id, void const *r, uint16_t l){ return tud_hid_n_report(0, id, r, l); }
/* USB audio class (UAC1) constants */
enum { AUDIO_SUBCLASS_UNDEFINED = 0, AUDIO_SUBCLASS_CONTROL, AUDIO_SUBCLASS_STREAMING, AUDIO_SUBCLASS_MIDI_STREAMING };
enum { AUDIO_CS_AC_INTERFACE_HEADER = 1, AUDIO_CS_AC_INTERFACE_INPUT_TERMINAL, AUDIO_CS_AC_INTERFACE_OUTPUT_TERMINAL, AUDIO_CS_AC_INTERFACE_MIXER_UNIT, AUDIO_CS_AC_INTERFACE_SELECTOR_UNIT, AUDIO_CS_AC_INTERFACE_FEATURE_UNIT };
enum { AUDIO_CS_AS_INTERFACE_AS_GENERAL = 1, AUDIO_CS_AS_INTERFACE_FORMAT_TYPE };
enum { AUDIO_CS_EP_SUBTYPE_GENERAL = 1 };
enum { AUDIO_TERM_TYPE_USB_STREAMING = 0x0101, AUDIO_TERM_TYPE_OUT_GENERIC_SPEAKER = 0x0301 };
#define MISC_SUBCLASS_COMMON 2
#define MISC_PROTOCOL_IAD 1
#endif
