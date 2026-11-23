/* Shared harness plumbing: per-input reset, the boot sequence the firmware's
 * core0_init() runs for the control plane, and the main-loop step that
 * applies a received bulk parameter blob. */
#include "host_io.h"
#include "harness_common.h"
#include "config.h"
#include "flash_storage.h"
#include "notify.h"
#include "uart_control.h"
#include "i2c_control.h"
#include "bulk_params.h"
#include "control_surfaces.h"
#include "usb_audio.h"

extern volatile bool bulk_params_pending;
extern uint8_t bulk_param_buf[];

static int snapshotted;

void harness_boot(const uint8_t *flash_image, size_t flash_len) {
    if (!snapshotted) { host_state_snapshot(); snapshotted = 1; }
    host_state_restore();
    memset(host_flash, 0xff, HOST_FLASH_REGION_BYTES);   /* erased flash */
    if (flash_image && flash_len) {
        size_t n = flash_len < HOST_FLASH_REGION_BYTES ? flash_len : HOST_FLASH_REGION_BYTES;
        memcpy(host_flash + (HOST_FLASH_REGION_BYTES - 21u * 4096u), flash_image, n > 21u * 4096u ? 21u * 4096u : n);
    }
    g_host_time_us = 1000000;
    audio_state.freq = 48000;   /* usb_audio.c (not host-built) boots at 48 kHz once enumerated */
    preset_boot_load();
    notify_init();
    control_surfaces_init();
}

void harness_main_loop_step(void) {
    g_host_time_us += 250;
    if (bulk_params_pending) {
        bulk_params_pending = false;
        (void)bulk_params_apply((const WireBulkParams *)bulk_param_buf, true);
    }
}

bool harness_uart_up(void) {
    static const uint8_t pairs[][2] = { {4, 5}, {8, 9}, {12, 13}, {16, 17}, {20, 21}, {0, 1} };
    for (size_t i = 0; i < sizeof(pairs) / sizeof(pairs[0]); i++) {
        UartCtrlConfig c = { 1, pairs[i][0], pairs[i][1], 1, 115200 };
        if (uart_ctrl_apply(&c) == PIN_CONFIG_SUCCESS && uart_ctrl_is_live()) return true;
    }
    return false;
}

bool harness_i2c_up(void) {
    static const uint8_t pairs[][2] = { {2, 3}, {6, 7}, {10, 11}, {14, 15}, {18, 19}, {26, 27} };
    for (size_t i = 0; i < sizeof(pairs) / sizeof(pairs[0]); i++) {
        I2cCtrlConfig c = { 1, pairs[i][0], pairs[i][1], 0x42, {0} };
        if (i2c_ctrl_apply(&c) == PIN_CONFIG_SUCCESS && i2c_ctrl_is_live()) return true;
    }
    return false;
}
