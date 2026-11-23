/* Drive one USB EP0 vendor control transfer through TinyUSB's callback the
 * way the device stack does: SETUP, then (for OUT data) DATA, then ACK. */
#ifndef DSPI_USB_OPS_H
#define DSPI_USB_OPS_H
#include "host_io.h"
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *req);
static inline void usb_vendor_transfer(uint8_t bmrt, uint8_t breq, uint16_t wval, uint16_t widx,
                                       uint16_t wlen, const uint8_t *data, uint16_t dlen) {
    tusb_control_request_t r;
    memset(&r, 0, sizeof r);
    r.bmRequestType = bmrt; r.bRequest = breq; r.wValue = wval; r.wIndex = widx; r.wLength = wlen;
    g_host_ctrl_out = data; g_host_ctrl_out_len = dlen;
    if (!tud_vendor_control_xfer_cb(0, CONTROL_STAGE_SETUP, &r)) return;
    if (!(bmrt & 0x80) && wlen) tud_vendor_control_xfer_cb(0, CONTROL_STAGE_DATA, &r);
    tud_vendor_control_xfer_cb(0, CONTROL_STAGE_ACK, &r);
}
#endif
