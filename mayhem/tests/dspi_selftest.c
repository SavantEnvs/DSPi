/* Known-answer tests for the host-built DSPi control plane (the oracle run by
 * mayhem/test.sh).  Each check drives a public entry point with a fixed
 * input and asserts the exact result.  Prints "PASS <name>" / "FAIL <name>". */
#include <math.h>
#include "usb_ops.h"
#include "harness_common.h"
#include "bulk_params.h"
#include "flash_storage.h"
#include "control_surfaces_ir.h"
#include "config.h"

#include <stdlib.h>
static int failed;
/* DSPI_DUMP_BULK / DSPI_DUMP_FLASH: write the blob / preset-region image the
 * tests produce (used once to make the fuzz seeds). */
static void dump(const char *env, const void *p, size_t n) {
    const char *path = getenv(env);
    if (!path) return;
    FILE *f = fopen(path, "wb");
    if (f) { fwrite(p, 1, n, f); fclose(f); }
}
static void check(const char *name, int ok) { printf("%s %s\n", ok ? "PASS" : "FAIL", name); if (!ok) failed++; }

static float usb_get_f32(uint8_t req, uint16_t wval) {
    g_host_ctrl_in_len = 0;
    usb_vendor_transfer(0xC1, req, wval, 0, 4, NULL, 0);
    float f = NAN;
    if (g_host_ctrl_in_len == 4) memcpy(&f, g_host_ctrl_in, 4);
    return f;
}
static void usb_set_f32(uint8_t req, uint16_t wval, float f) {
    uint8_t b[4]; memcpy(b, &f, 4);
    usb_vendor_transfer(0x41, req, wval, 0, 4, b, 4);
    harness_main_loop_step();
}

static void ir_edge(uint32_t dur_us, bool *level) {
    g_host_time_us += dur_us;
    *level = !*level;
    if (*level) g_host_gpio_in |= 1u << 22; else g_host_gpio_in &= ~(1u << 22);
    g_host_irq_handlers[IO_IRQ_BANK0]();
}

int main(void) {
    static uint8_t blob[sizeof(WireBulkParams)];
    WireBulkParams *w = (WireBulkParams *)blob;
    static WireBulkParams back;

    /* 1. bulk GET over USB returns a full V32 blob for this platform */
    harness_boot(NULL, 0);
    g_host_ctrl_in_len = 0;
    usb_vendor_transfer(0xC1, REQ_GET_ALL_PARAMS, 0, 0, (uint16_t)sizeof(WireBulkParams), NULL, 0);
    memcpy(blob, g_host_ctrl_in, sizeof blob);
    dump("DSPI_DUMP_BULK", blob, sizeof blob);
    check("bulk_get_size", g_host_ctrl_in_len == sizeof(WireBulkParams));
    check("bulk_get_header", w->header.format_version == WIRE_FORMAT_VERSION && w->header.payload_length == sizeof(WireBulkParams)
          && w->header.num_channels == NUM_CHANNELS && w->header.num_output_channels == NUM_OUTPUT_CHANNELS);

    /* 2. bulk apply: a changed loudness reference is applied and read back; bad versions are rejected */
    WireBulkParams mod = *w;
    mod.global.loudness_ref_spl = 83.0f;
    check("bulk_apply_ok", bulk_params_apply(&mod, false) == 0);
    bulk_params_collect(&back);
    printf("loudness_ref_spl after apply: %f\n", back.global.loudness_ref_spl);
    check("bulk_apply_readback", back.global.loudness_ref_spl == 83.0f);
    WireBulkParams bad = *w; bad.header.format_version = 1;
    check("bulk_apply_rejects_version", bulk_params_apply(&bad, false) != 0);
    bad = *w; bad.header.payload_length = 100;
    check("bulk_apply_rejects_length", bulk_params_apply(&bad, false) != 0);

    /* 3. preset save to flash, change state, load back restores it */
    harness_boot(NULL, 0);
    check("bulk_apply_83", bulk_params_apply(&mod, false) == 0);
    uint8_t st = preset_save(3);
    printf("preset_save=%u\n", st);
    check("preset_save", st == PRESET_OK);
    dump("DSPI_DUMP_FLASH", host_flash + (HOST_FLASH_REGION_BYTES - 21u * 4096u), 21u * 4096u);
    mod.global.loudness_ref_spl = 79.0f;
    (void)bulk_params_apply(&mod, false);
    st = preset_load(3);
    printf("preset_load=%u\n", st);
    check("preset_load", st == PRESET_OK);
    bulk_params_collect(&back);
    check("preset_restores_state", back.global.loudness_ref_spl == 83.0f);

    /* 4. NEC IR frame address 0x04 command 0x08 decodes */
    harness_boot(NULL, 0);
    cs_ir_attach(22, false);
    bool level = true;
    uint32_t frame = 0x04u | (0xFBu << 8) | (0x08u << 16) | (0xF7u << 24);
    g_host_time_us += 100000;
    ir_edge(100000, &level);           /* idle high -> leading mark starts */
    ir_edge(9000, &level); ir_edge(4500, &level);
    for (int i = 0; i < 32; i++) { ir_edge(562, &level); ir_edge((frame >> i) & 1 ? 1687 : 562, &level); }
    ir_edge(562, &level);
    g_host_time_us += 120000;
    CsIrEvent ev; int got = 0; uint32_t code = 0; uint8_t proto = 0;
    while (cs_ir_poll(&ev)) { if (!got) { code = ev.code; proto = ev.protocol; } got++; }
    printf("ir: got=%d proto=%u code=0x%08x\n", got, proto, code);
    check("ir_nec_decode", got >= 1 && proto == CS_IR_PROTO_NEC && code == 0xF708FB04u);

    printf("RESULT failed=%d\n", failed);
    return failed ? 1 : 0;
}
