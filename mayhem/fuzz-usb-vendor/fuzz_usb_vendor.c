/* USB EP0 vendor requests: the fuzz input is a sequence of control
 * transfers as a host would send them, each
 *   [bmRequestType][bRequest][wValue LE16][wIndex LE16][wLength LE16][dlen LE16][data[dlen]]
 * (data is the OUT data stage; wLength/dlen are independent, as on the wire
 * where the device sees whatever the host puts in the setup packet).  Every
 * transfer goes through tud_vendor_control_xfer_cb(), the firmware's TinyUSB
 * vendor callback, followed by the main-loop apply step. */
#include "usb_ops.h"
#include "harness_common.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    harness_boot(NULL, 0);
    size_t off = 0;
    for (int n = 0; n < 64 && size - off >= 10; n++) {
        const uint8_t *h = data + off;
        uint16_t dlen = (uint16_t)(h[8] | (h[9] << 8));
        off += 10;
        if (dlen > size - off) dlen = (uint16_t)(size - off);
        usb_vendor_transfer(h[0], h[1], (uint16_t)(h[2] | (h[3] << 8)), (uint16_t)(h[4] | (h[5] << 8)),
                            (uint16_t)(h[6] | (h[7] << 8)), data + off, dlen);
        off += dlen;
        harness_main_loop_step();
    }
    return 0;
}
