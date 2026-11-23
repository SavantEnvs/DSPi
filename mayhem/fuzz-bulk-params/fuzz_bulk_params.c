/* REQ_SET_ALL_PARAMS (0xA1): the fuzz input is the bulk parameter blob
 * (WireBulkParams, as sent by the DSPi host app) delivered in one USB vendor
 * OUT transfer of its own length.  The main-loop step then hands the blob to
 * bulk_params_apply(), and REQ_GET_ALL_PARAMS (0xA0) reads the state back
 * through bulk_params_collect(). */
#include "usb_ops.h"
#include "harness_common.h"
#include "bulk_params.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size > 0xffff) return -1;   /* wLength is 16 bits */
    harness_boot(NULL, 0);
    usb_vendor_transfer(0x41, 0xA1, 0, 0, (uint16_t)size, data, (uint16_t)size);
    harness_main_loop_step();
    usb_vendor_transfer(0xC1, 0xA0, 0, 0, (uint16_t)sizeof(WireBulkParams), NULL, 0);
    return 0;
}
