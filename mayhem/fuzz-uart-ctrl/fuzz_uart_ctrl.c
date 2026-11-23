/* UART control transport: the fuzz input is the raw byte stream an external
 * MCU sends on the control UART (A5-framed SET/GET requests, CRC16).  Bytes
 * enter through the firmware's own RX IRQ handler in FIFO-sized bursts and
 * are parsed/dispatched by uart_ctrl_poll(), as in the main loop. */
#include "host_io.h"
#include "harness_common.h"
#include "uart_control.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    harness_boot(NULL, 0);
    if (!harness_uart_up()) __builtin_trap();   /* the harness setup itself is broken */
    irq_handler_t rx = g_host_irq_handlers[UART0_IRQ] ? g_host_irq_handlers[UART0_IRQ] : g_host_irq_handlers[UART1_IRQ];
    size_t off = 0;
    while (off < size) {
        size_t n = size - off < 32 ? size - off : 32;   /* one RX FIFO burst */
        g_host_uart_rx.data = data + off; g_host_uart_rx.len = n; g_host_uart_rx.pos = 0;
        rx();
        off += n;
        uart_ctrl_poll();
        harness_main_loop_step();
        g_host_time_us += 100;   /* below the inter-byte timeout */
    }
    g_host_uart_rx.len = 0;
    for (int i = 0; i < 8; i++) { uart_ctrl_poll(); harness_main_loop_step(); }
    return 0;
}
