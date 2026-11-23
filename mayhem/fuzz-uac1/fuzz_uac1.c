/* UAC1 class driver (usb_audio.c), reached the way TinyUSB reaches it: the
 * driver table from usbd_app_driver_get_cb(), opened on an AC + AS + vendor
 * interface set, then driven by a sequence of host events.  Each op is
 *   [op] ...
 *   op%5==0 control transfer: [bmRequestType][bRequest][wValue LE16][wIndex LE16]
 *           [wLength LE16][dlen][data[dlen]]  -> SETUP, DATA (OUT), ACK
 *   op%5==1 iso OUT audio packet: [len LE16][bytes] -> xfer_cb(EP 0x01), then
 *           the main loop's usb_audio_drain_ring()
 *   op%5==2 SOF: [frame LE16][dt]           -> sof(), time advances dt*8 us
 *   op%5==3 feedback / notify IN completion  -> xfer_cb(EP 0x82 / 0x83)
 *   op%5==4 main-loop step (bulk apply, notify tick)
 * The iso length is clamped to what the driver armed, as the controller does. */
#include "host_io.h"
#include "harness_common.h"
#include "usb_descriptors.h"
#include "usb_audio.h"

usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count);
void usb_audio_drain_ring(void);
void usb_notify_tick(void);
extern uint8_t *g_host_ep_buf[16];
extern uint16_t g_host_ep_len[16];

/* AC itf 0 (+ CS header), AS itf 1 alt 0 and alt 1 (+ iso data / feedback EPs),
 * vendor itf 2 with the notify interrupt EP. */
static const uint8_t ac_as_desc[] = {
    9, TUSB_DESC_INTERFACE, 0, 0, 0, 0x01, 0x01, 0, 0,
    9, TUSB_DESC_CS_INTERFACE, 0x01, 0x00, 0x01, 9, 0, 1, 1,
    9, TUSB_DESC_INTERFACE, 1, 0, 0, 0x01, 0x02, 0, 0,
    9, TUSB_DESC_INTERFACE, 1, 1, 2, 0x01, 0x02, 0, 0,
    9, TUSB_DESC_ENDPOINT, AUDIO_OUT_ENDPOINT, 0x05, 0x14, 0x03, 1, 0, 0,
    9, TUSB_DESC_ENDPOINT, AUDIO_IN_ENDPOINT, 0x11, 4, 0, 1, 1, 0,
};
static const uint8_t vendor_desc[] = {
    9, TUSB_DESC_INTERFACE, 2, 0, 1, 0xFF, 0, 0, 0,
    7, TUSB_DESC_ENDPOINT, NOTIFY_IN_ENDPOINT, 0x03, 64, 0, 1,
};

static void control(const usbd_class_driver_t *d, const uint8_t *h, const uint8_t *data, uint16_t dlen) {
    tusb_control_request_t r;
    memset(&r, 0, sizeof r);
    r.bmRequestType = h[0]; r.bRequest = h[1];
    r.wValue = (uint16_t)(h[2] | (h[3] << 8)); r.wIndex = (uint16_t)(h[4] | (h[5] << 8));
    r.wLength = (uint16_t)(h[6] | (h[7] << 8));
    g_host_ctrl_out = data; g_host_ctrl_out_len = dlen;
    if (!d->control_xfer_cb(0, CONTROL_STAGE_SETUP, &r)) return;
    if (!(r.bmRequestType & 0x80) && r.wLength) d->control_xfer_cb(0, CONTROL_STAGE_DATA, &r);
    d->control_xfer_cb(0, CONTROL_STAGE_ACK, &r);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    harness_boot(NULL, 0);
    memset(g_host_ep_buf, 0, sizeof g_host_ep_buf);
    memset(g_host_ep_len, 0, sizeof g_host_ep_len);
    uint8_t n = 0;
    const usbd_class_driver_t *d = usbd_app_driver_get_cb(&n);
    if (!d || !n) return 0;
    d->init();
    d->reset(0);
    if (!d->open(0, (const tusb_desc_interface_t *)ac_as_desc, sizeof ac_as_desc)) return 0;
    d->open(0, (const tusb_desc_interface_t *)vendor_desc, sizeof vendor_desc);

    size_t off = 0;
    for (int k = 0; k < 128 && off < size; k++) {
        uint8_t op = data[off++] % 5;
        if (op == 0) {
            if (size - off < 9) break;
            const uint8_t *h = data + off;
            uint16_t dlen = h[8];
            off += 9;
            if (dlen > size - off) dlen = (uint16_t)(size - off);
            control(d, h, data + off, dlen);
            off += dlen;
        } else if (op == 1) {
            if (size - off < 2) break;
            uint32_t len = (uint32_t)(data[off] | (data[off + 1] << 8));
            off += 2;
            uint8_t ep = AUDIO_OUT_ENDPOINT & 15;
            uint8_t *buf = g_host_ep_buf[ep];
            uint32_t cap = g_host_ep_len[ep];
            if (len > cap) len = cap;
            uint32_t avail = (uint32_t)(size - off);
            uint32_t copy = len < avail ? len : avail;
            if (buf && copy) memcpy(buf, data + off, copy);
            off += copy;
            g_host_time_us += 1000;
            d->xfer_cb(0, AUDIO_OUT_ENDPOINT, XFER_RESULT_SUCCESS, len);
            usb_audio_drain_ring();
        } else if (op == 2) {
            if (size - off < 3) break;
            uint32_t frame = (uint32_t)(data[off] | (data[off + 1] << 8));
            g_host_time_us += 8u * data[off + 2];
            off += 3;
            if (d->sof) d->sof(0, frame);
        } else if (op == 3) {
            if (size - off < 1) break;
            uint8_t ep = (data[off++] & 1) ? NOTIFY_IN_ENDPOINT : AUDIO_IN_ENDPOINT;
            d->xfer_cb(0, ep, XFER_RESULT_SUCCESS, 3);
        } else {
            harness_main_loop_step();
            usb_notify_tick();
        }
    }
    return 0;
}
