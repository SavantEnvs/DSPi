#ifndef DSPI_HARNESS_COMMON_H
#define DSPI_HARNESS_COMMON_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
void harness_boot(const uint8_t *flash_image, size_t flash_len);
void harness_main_loop_step(void);
bool harness_uart_up(void);
bool harness_i2c_up(void);
#endif
