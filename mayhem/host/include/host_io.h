#ifndef DSPI_HOST_IO_H
#define DSPI_HOST_IO_H
#include "dspi_host.h"
#include "tusb.h"
typedef struct { const uint8_t *data; size_t len, pos; } host_stream_t;
extern host_stream_t g_host_uart_rx, g_host_i2c_rx;
extern size_t g_host_i2c_rx_avail;
extern size_t g_host_i2c_tx_space;   /* free TX FIFO entries; write_byte_raw consumes one */
extern uint8_t g_host_i2c_tx_last;
extern uint32_t g_host_i2c_tx_count;
extern irq_handler_t g_host_irq_handlers[NUM_IRQS];
extern i2c_slave_handler_t g_host_i2c_slave_handler;
extern i2c_inst_t *g_host_i2c_slave_inst;
extern gpio_irq_callback_t g_host_gpio_callback;
extern const uint8_t *g_host_ctrl_out; extern uint16_t g_host_ctrl_out_len;
extern uint8_t g_host_ctrl_in[8192]; extern uint16_t g_host_ctrl_in_len;
extern uint8_t *g_host_ep_buf[16]; extern uint16_t g_host_ep_len[16];
int host_stream_get(host_stream_t *s);
/* Firmware state reset: restores every firmware global to its load-time image. */
void host_state_snapshot(void);
void host_state_restore(void);
#endif
