/* Host implementations of the Pico SDK / TinyUSB pieces that DSPi's control
 * plane needs at run time: memory-backed registers and flash, captured IRQ
 * handlers, and byte sources the harnesses fill with fuzz input.  Every other
 * SDK call is a generated no-op (mayhem/host/gen_stubs.sh). */
#include "dspi_host.h"
#include "tusb.h"
#include "host_io.h"

uint64_t g_host_time_us = 1000000;
spin_lock_t g_host_spinlock;
uint32_t g_host_gpio_in = 0xffffffffu;
uart_hw_t host_uart_hw[2];
static int host_uart_tag[2];
uart_inst_t *const host_uart0 = (uart_inst_t *)&host_uart_tag[0];
uart_inst_t *const host_uart1 = (uart_inst_t *)&host_uart_tag[1];
static i2c_hw_t host_i2c_hw[2];
i2c_inst_t host_i2c_inst[2] = { { &host_i2c_hw[0], false }, { &host_i2c_hw[1], false } };
pio_hw_t host_pio_hw[3];
dma_hw_t host_dma_hw;
dma_debug_hw_t host_dma_debug_hw;
clocks_hw_t host_clocks_hw;
pll_hw_t host_pll_hw[2];
qmi_hw_t host_qmi_hw;
nvic_hw_t host_nvic_hw;
bus_ctrl_hw_t host_bus_ctrl_hw;
accessctrl_hw_t host_accessctrl_hw;
timer_hw_t host_timer_hw;
uint8_t host_flash[HOST_FLASH_REGION_BYTES] __attribute__((aligned(4096)));

host_stream_t g_host_uart_rx, g_host_i2c_rx;
irq_handler_t g_host_irq_handlers[NUM_IRQS];
i2c_slave_handler_t g_host_i2c_slave_handler;
i2c_inst_t *g_host_i2c_slave_inst;
gpio_irq_callback_t g_host_gpio_callback;

int host_stream_get(host_stream_t *s) {
    if (s->pos >= s->len) return -1;
    return s->data[s->pos++];
}

/* uart */
bool uart_is_readable(uart_inst_t *u) {
    int b = host_stream_get(&g_host_uart_rx);
    if (b < 0) return false;
    uart_get_hw(u)->dr = (uint32_t)b;
    return true;
}
char uart_getc(uart_inst_t *u) { return (char)uart_get_hw(u)->dr; }
bool uart_is_writable(uart_inst_t *u) { (void)u; return true; }
void uart_putc_raw(uart_inst_t *u, char c) { uart_get_hw(u)->dr = (uint8_t)c; }
uint uart_init(uart_inst_t *u, uint baud) { (void)u; return baud; }

/* irq */
void irq_set_exclusive_handler(uint num, irq_handler_t h) { if (num < NUM_IRQS) g_host_irq_handlers[num] = h; }
void irq_add_shared_handler(uint num, irq_handler_t h, uint8_t prio) { (void)prio; if (num < NUM_IRQS) g_host_irq_handlers[num] = h; }
void irq_remove_handler(uint num, irq_handler_t h) { if (num < NUM_IRQS && g_host_irq_handlers[num] == h) g_host_irq_handlers[num] = 0; }
irq_handler_t irq_get_exclusive_handler(uint num) { return num < NUM_IRQS ? g_host_irq_handlers[num] : 0; }
void gpio_set_irq_enabled_with_callback(uint g, uint32_t m, bool en, gpio_irq_callback_t cb) { (void)g; (void)m; (void)en; g_host_gpio_callback = cb; }

/* i2c target */
void i2c_slave_init(i2c_inst_t *i, uint8_t addr, i2c_slave_handler_t h) { (void)addr; g_host_i2c_slave_inst = i; g_host_i2c_slave_handler = h; }
void i2c_slave_deinit(i2c_inst_t *i) { (void)i; g_host_i2c_slave_handler = 0; }
uint8_t i2c_read_byte_raw(i2c_inst_t *i) { (void)i; int b = host_stream_get(&g_host_i2c_rx); return b < 0 ? 0 : (uint8_t)b; }
size_t i2c_get_read_available(i2c_inst_t *i) { (void)i; return g_host_i2c_rx.len - g_host_i2c_rx.pos; }
size_t i2c_get_write_available(i2c_inst_t *i) { (void)i; return g_host_i2c_tx_space; }
void i2c_write_byte_raw(i2c_inst_t *i, uint8_t v) { (void)i; g_host_i2c_tx_last = v; g_host_i2c_tx_count++; if (g_host_i2c_tx_space) g_host_i2c_tx_space--; }
size_t g_host_i2c_rx_avail;
size_t g_host_i2c_tx_space;
uint8_t g_host_i2c_tx_last;
uint32_t g_host_i2c_tx_count;

/* flash: XIP_BASE points at host_flash */
#define HOST_FLASH_BASE_OFF (PICO_FLASH_SIZE_BYTES - HOST_FLASH_REGION_BYTES)
void flash_range_erase(uint32_t off, size_t count) {
    if (off < HOST_FLASH_BASE_OFF || off >= PICO_FLASH_SIZE_BYTES) __builtin_trap();  /* outside the preset store */
    off -= HOST_FLASH_BASE_OFF;
    if (count > sizeof(host_flash) - off) __builtin_trap();
    memset(host_flash + off, 0xff, count);
}
void flash_range_program(uint32_t off, const uint8_t *data, size_t count) {
    if (off < HOST_FLASH_BASE_OFF || off >= PICO_FLASH_SIZE_BYTES) __builtin_trap();
    off -= HOST_FLASH_BASE_OFF;
    if (count > sizeof(host_flash) - off) __builtin_trap();
    memcpy(host_flash + off, data, count);
}
static void rom_nop(void) {}
void *rom_func_lookup_inline(uint32_t code) { (void)code; return (void *)rom_nop; }
void *rom_func_lookup(uint32_t code) { (void)code; return (void *)rom_nop; }
void pico_get_unique_board_id_string(char *s, uint len) { if (len) { strncpy(s, "E66038B713849D31", len - 1); s[len - 1] = 0; } }

/* dma config copies */
dma_channel_config dma_get_channel_config(uint ch) { (void)ch; dma_channel_config c = {0}; return c; }

/* TinyUSB control endpoint: OUT data comes from the harness buffer */
const uint8_t *g_host_ctrl_out; uint16_t g_host_ctrl_out_len;
uint8_t g_host_ctrl_in[8192]; uint16_t g_host_ctrl_in_len;
void *g_host_ctrl_pending_buf; uint16_t g_host_ctrl_pending_len;
bool tud_control_xfer(uint8_t rhport, tusb_control_request_t const *req, void *buffer, uint16_t len) {
    (void)rhport;
    if (req->bmRequestType_bit.direction == TUSB_DIR_IN) {
        uint16_t n = len < sizeof(g_host_ctrl_in) ? len : sizeof(g_host_ctrl_in);
        if (buffer && n) memcpy(g_host_ctrl_in, buffer, n);
        g_host_ctrl_in_len = n;
    } else {
        g_host_ctrl_pending_buf = buffer; g_host_ctrl_pending_len = len;
        if (buffer && len) {
            uint16_t n = len < g_host_ctrl_out_len ? len : g_host_ctrl_out_len;
            memset(buffer, 0, len);
            if (n) memcpy(buffer, g_host_ctrl_out, n);
        }
    }
    return true;
}
bool tud_control_status(uint8_t rhport, tusb_control_request_t const *req) { (void)rhport; (void)req; return true; }
uint8_t *g_host_ep_buf[16]; uint16_t g_host_ep_len[16];
bool usbd_edpt_xfer(uint8_t rhport, uint8_t ep, uint8_t *buf, uint16_t n) { (void)rhport; g_host_ep_buf[ep & 15] = buf; g_host_ep_len[ep & 15] = n; return true; }
bool usbd_edpt_open(uint8_t rhport, tusb_desc_endpoint_t const *d) { (void)rhport; (void)d; return true; }
bool usbd_edpt_iso_alloc(uint8_t rhport, uint8_t ep, uint16_t n) { (void)rhport; (void)ep; (void)n; return true; }
bool usbd_edpt_iso_activate(uint8_t rhport, tusb_desc_endpoint_t const *d) { (void)rhport; (void)d; return true; }
bool usbd_edpt_claim(uint8_t rhport, uint8_t ep) { (void)rhport; (void)ep; return true; }
bool tud_mounted(void) { return true; }
bool tud_ready(void) { return true; }

/* flash_clkdiv.c (QMI clock-divider + ROM flash driver, not host-built)
 * exposes the firmware's flash write primitives; on the host they are the
 * RAM flash model above. */
void dspi_flash_range_erase(uint32_t off, size_t count) { flash_range_erase(off, count); }
void dspi_flash_range_program(uint32_t off, const uint8_t *data, size_t count) { flash_range_program(off, data, count); }

/* ADC: a fixed 12-bit reading of 876, the RP2350 temperature sensor at ~25 C (0.706 V at 27 C,
 * -1.721 mV/C) and a mid-low pot position.  A generated `return 0` stub would read as ~437 C. */
uint16_t adc_read(void) { return 876; }

/* soft_vcxo.c is not built for the host (PLL/DMA/PWM register code).  On the host its
 * soft_vcxo_init() never runs, so the real code stays un-ready: set_ppm returns early and the
 * commanded trim stays 0 ppm.  These must return float, not the generated `long` stub. */
#include "soft_vcxo.h"
void soft_vcxo_set_ppm(float ppm) { (void)ppm; }
float soft_vcxo_current_ppm(void) { return 0.0f; }
#if DSPI_CLOCK_DIAG
void soft_vcxo_get_diag(SoftVcxoDiag *out) { memset(out, 0, sizeof *out); out->parked = true; }
#endif
