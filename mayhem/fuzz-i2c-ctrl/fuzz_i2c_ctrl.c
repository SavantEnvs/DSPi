/* I2C target control interface: the fuzz input is a sequence of bus
 * transactions a controller performs against DSPi's target address.  Each
 * op byte selects:
 *   0: write transaction  [0][n][n bytes]  -> RECEIVE (FIFO bursts), FINISH
 *   1: read transaction   [1][n]           -> REQUEST events for n bytes (16-byte TX FIFO), FINISH
 *   2: poll               [2]              -> i2c_ctrl_poll() + main-loop step
 *   3: advance time       [3][ms]
 * Events go to the handler the firmware registered with i2c_slave_init(). */
#include "host_io.h"
#include "harness_common.h"
#include "i2c_control.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    harness_boot(NULL, 0);
    if (!harness_i2c_up() || !g_host_i2c_slave_handler) __builtin_trap();   /* harness setup broken */
    i2c_inst_t *bus = g_host_i2c_slave_inst;
    size_t off = 0;
    while (off < size) {
        uint8_t op = data[off++] & 3;
        if (op == 0) {
            if (off >= size) break;
            size_t n = data[off++];
            if (n > size - off) n = size - off;
            size_t done = 0;
            while (done < n) {
                size_t burst = n - done < 16 ? n - done : 16;   /* RX FIFO depth */
                g_host_i2c_rx.data = data + off + done; g_host_i2c_rx.len = burst; g_host_i2c_rx.pos = 0;
                g_host_i2c_slave_handler(bus, I2C_SLAVE_RECEIVE);   /* drains while read_available > 0 */
                done += burst;
            }
            off += n;
            g_host_i2c_slave_handler(bus, I2C_SLAVE_FINISH);
        } else if (op == 1) {
            if (off >= size) break;
            size_t n = data[off++];
            for (size_t left = n; left > 0; ) {
                g_host_i2c_tx_space = left < 16 ? left : 16;
                left -= g_host_i2c_tx_space;
                g_host_i2c_slave_handler(bus, I2C_SLAVE_REQUEST);
            }
            g_host_i2c_tx_space = 0;
            g_host_i2c_slave_handler(bus, I2C_SLAVE_FINISH);
        } else if (op == 2) {
            i2c_ctrl_poll();
            harness_main_loop_step();
        } else {
            if (off >= size) break;
            g_host_time_us += 1000u * data[off++];
        }
    }
    for (int i = 0; i < 4; i++) { i2c_ctrl_poll(); harness_main_loop_step(); }
    return 0;
}
