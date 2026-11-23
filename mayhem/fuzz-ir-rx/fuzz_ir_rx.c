/* IR remote receiver: the fuzz input is the sequence of mark/space
 * durations (little-endian u16, microseconds) a demodulating IR receiver
 * produces on its GPIO.  Each edge advances the clock, flips the pin level
 * and runs the firmware's edge IRQ handler; cs_ir_poll() decodes
 * (NEC/RC5/RC6/hash) as the main loop does.  The first byte selects the
 * receiver polarity and whether learn mode is armed. */
#include "host_io.h"
#include "harness_common.h"
#include "control_surfaces_ir.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 1) return -1;
    harness_boot(NULL, 0);
    const uint8_t pin = 22;
    cs_ir_attach(pin, data[0] & 1);
    if (data[0] & 2) cs_ir_learn_arm();
    irq_handler_t edge = g_host_irq_handlers[IO_IRQ_BANK0];
    if (!edge) __builtin_trap();   /* harness setup broken */
    bool level = true;
    CsIrEvent ev;
    for (size_t off = 1; off + 2 <= size; off += 2) {
        g_host_time_us += (uint16_t)(data[off] | (data[off + 1] << 8));
        level = !level;
        if (level) g_host_gpio_in |= 1u << pin; else g_host_gpio_in &= ~(1u << pin);
        edge();
        if (((off - 1) / 2) % 8 == 7) while (cs_ir_poll(&ev)) {}
    }
    g_host_time_us += 200000;   /* frame gap: flush the decoder */
    while (cs_ir_poll(&ev)) {}
    (void)cs_ir_learn_take_change();
    uint8_t proto; uint32_t code;
    (void)cs_ir_learn_result(&proto, &code);
    return 0;
}
