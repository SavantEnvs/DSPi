/* Host stub layer: lets the DSPi firmware sources compile on x86 for fuzzing.
 * Hardware is replaced by plain memory; SDK functions not defined in
 * mayhem/host/sdk_stubs.c are generated as no-op stubs by mayhem/build.sh. */
#ifndef DSPI_HOST_H
#define DSPI_HOST_H
/* Firmware globals go to dedicated sections so the harness can restore the
 * load-time image before every input (see mayhem/host/state.c, dspi.ld). */
#if defined(DSPI_HOST_STATE_SECTIONS) && defined(__clang__)
#pragma clang section bss=".bss.dspi_state" data=".data.dspi_state"
#endif
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define PICO_RP2350 1
#define PICO_RP2040 0
#define PICO_NO_HARDWARE 0
typedef unsigned int uint;
typedef uint64_t absolute_time_t;
typedef volatile uint32_t io_rw_32;
typedef volatile uint32_t io_ro_32;
typedef volatile uint32_t io_wo_32;
typedef uint32_t uint32;
#define __not_in_flash_func(f) f
#define __no_inline_not_in_flash_func(f) __attribute__((noinline)) f
#define __time_critical_func(f) f
#define __not_in_flash(x)
#define __scratch_x(x)
#define __scratch_y(x)
#define __in_flash(x)
#define __uninitialized_ram(x) x
#define __force_inline inline __attribute__((always_inline))
#define __always_inline inline __attribute__((always_inline))
#define __noinline __attribute__((noinline))
#define __aligned(x) __attribute__((aligned(x)))
#define __packed __attribute__((packed))
#define __unused __attribute__((unused))
#define __isr
#define __weak __attribute__((weak))
#define __dmb() __sync_synchronize()
#define __dsb() __sync_synchronize()
#define __isb() __sync_synchronize()
#define __sev() do{}while(0)
#define __wfe() do{}while(0)
#define __wfi() do{}while(0)
#define __nop() do{}while(0)
#define __compiler_memory_barrier() __asm__ volatile("" ::: "memory")
#define tight_loop_contents() do{}while(0)
#define bi_decl(x)
#define count_of(a) (sizeof(a)/sizeof((a)[0]))
#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif
#define hard_assert(x) do{ if(!(x)) __builtin_trap(); }while(0)
#define invalid_params_if(m,x) do{}while(0)
#define valid_params_if(m,x) do{}while(0)
#define panic(...) __builtin_trap()
#define panic_unsupported() __builtin_trap()
#define XOSC_HZ 12000000u
#define SYS_CLK_HZ 150000000u

/* time: the harness advances g_host_time_us */
extern uint64_t g_host_time_us;
static inline uint32_t time_us_32(void){ return (uint32_t)g_host_time_us; }
static inline uint64_t time_us_64(void){ return g_host_time_us; }
static inline absolute_time_t get_absolute_time(void){ return g_host_time_us; }
static inline uint32_t to_ms_since_boot(absolute_time_t t){ return (uint32_t)(t/1000); }
static inline uint64_t to_us_since_boot(absolute_time_t t){ return t; }
static inline absolute_time_t make_timeout_time_ms(uint32_t ms){ return g_host_time_us + 1000ull*ms; }
static inline absolute_time_t make_timeout_time_us(uint64_t us){ return g_host_time_us + us; }
static inline int64_t absolute_time_diff_us(absolute_time_t a, absolute_time_t b){ return (int64_t)(b-a); }
static inline absolute_time_t delayed_by_us(absolute_time_t t, uint64_t us){ return t+us; }
static inline absolute_time_t delayed_by_ms(absolute_time_t t, uint32_t ms){ return t+1000ull*ms; }
static inline bool time_reached(absolute_time_t t){ return g_host_time_us >= t; }
static inline void sleep_us(uint64_t us){ g_host_time_us += us; }
static inline void sleep_ms(uint32_t ms){ g_host_time_us += 1000ull*ms; }
static inline void busy_wait_us_32(uint32_t us){ g_host_time_us += us; }
static inline void busy_wait_us(uint64_t us){ g_host_time_us += us; }
static inline void busy_wait_ms(uint32_t ms){ g_host_time_us += 1000ull*ms; }
static inline uint32_t save_and_disable_interrupts(void){ return 0; }
static inline void restore_interrupts(uint32_t s){ (void)s; }
static inline void restore_interrupts_from_disabled(uint32_t s){ (void)s; }
static inline uint get_core_num(void){ return 0; }

/* spin locks */
typedef volatile uint32_t spin_lock_t;
extern spin_lock_t g_host_spinlock;
static inline spin_lock_t *spin_lock_instance(uint n){ (void)n; return &g_host_spinlock; }
static inline uint32_t spin_lock_blocking(spin_lock_t *l){ (void)l; return 0; }
static inline void spin_unlock(spin_lock_t *l, uint32_t s){ (void)l; (void)s; }
static inline void spin_lock_unsafe_blocking(spin_lock_t *l){ (void)l; }
static inline void spin_unlock_unsafe(spin_lock_t *l){ (void)l; }
static inline int spin_lock_claim_unused(bool r){ (void)r; return 0; }
static inline void spin_lock_claim(uint n){ (void)n; }

/* gpio */
enum gpio_function { GPIO_FUNC_XIP=0, GPIO_FUNC_SPI=1, GPIO_FUNC_UART=2, GPIO_FUNC_I2C=3, GPIO_FUNC_PWM=4,
  GPIO_FUNC_SIO=5, GPIO_FUNC_PIO0=6, GPIO_FUNC_PIO1=7, GPIO_FUNC_PIO2=8, GPIO_FUNC_GPCK=9, GPIO_FUNC_USB=10,
  GPIO_FUNC_UART_AUX=11, GPIO_FUNC_NULL=0x1f };
typedef enum gpio_function gpio_function_t;
#define GPIO_OUT 1
#define GPIO_IN 0
enum gpio_irq_level { GPIO_IRQ_LEVEL_LOW=1, GPIO_IRQ_LEVEL_HIGH=2, GPIO_IRQ_EDGE_FALL=4, GPIO_IRQ_EDGE_RISE=8 };
enum gpio_drive_strength { GPIO_DRIVE_STRENGTH_2MA=0, GPIO_DRIVE_STRENGTH_4MA, GPIO_DRIVE_STRENGTH_8MA, GPIO_DRIVE_STRENGTH_12MA };
enum gpio_slew_rate { GPIO_SLEW_RATE_SLOW=0, GPIO_SLEW_RATE_FAST=1 };
enum gpio_override { GPIO_OVERRIDE_NORMAL=0, GPIO_OVERRIDE_INVERT=1, GPIO_OVERRIDE_LOW=2, GPIO_OVERRIDE_HIGH=3 };
typedef void (*gpio_irq_callback_t)(uint gpio, uint32_t event_mask);
typedef void (*irq_handler_t)(void);
extern uint32_t g_host_gpio_in;
static inline bool gpio_get(uint g){ return (g_host_gpio_in >> (g & 31)) & 1u; }
static inline uint32_t gpio_get_all(void){ return g_host_gpio_in; }
#define NUM_BANK0_GPIOS 48
#define NUM_GPIOS NUM_BANK0_GPIOS
#define IO_IRQ_BANK0 21
#define UART0_IRQ 33
#define UART1_IRQ 34
#define I2C0_IRQ 36
#define I2C1_IRQ 37
#define DMA_IRQ_0 10
#define DMA_IRQ_1 11
#define DMA_IRQ_2 12
#define DMA_IRQ_3 13
#define PIO0_IRQ_0 15
#define PIO0_IRQ_1 16
#define PIO1_IRQ_0 17
#define PIO1_IRQ_1 18
#define PIO2_IRQ_0 19
#define PIO2_IRQ_1 20
#define USBCTRL_IRQ 14
#define SIO_IRQ_FIFO 25
#define TIMER0_IRQ_0 0
#define TIMER0_IRQ_1 1
#define TIMER0_IRQ_2 2
#define TIMER0_IRQ_3 3
#define PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY 0x80
#define PICO_HIGHEST_IRQ_PRIORITY 0
#define PICO_LOWEST_IRQ_PRIORITY 0xff
#define PICO_DEFAULT_IRQ_PRIORITY 0x80
#define NUM_IRQS 52

/* uart / i2c */
typedef struct uart_inst uart_inst_t;
typedef struct i2c_hw { io_rw_32 regs[64]; io_rw_32 enable; io_rw_32 data_cmd; io_rw_32 status; io_rw_32 txflr; io_rw_32 rxflr; io_rw_32 raw_intr_stat; io_rw_32 intr_mask; io_rw_32 intr_stat; io_rw_32 clr_intr; io_rw_32 clr_tx_abrt; io_rw_32 clr_rd_req; io_rw_32 clr_stop_det; io_rw_32 clr_start_det; io_rw_32 clr_restart_det; io_rw_32 sar; io_rw_32 tar; io_rw_32 con; io_rw_32 tx_abrt_source; } i2c_hw_t;
typedef struct i2c_inst { i2c_hw_t *hw; bool restart_on_next; } i2c_inst_t;
extern uart_inst_t *const host_uart0, *const host_uart1;
#define uart0 host_uart0
#define uart1 host_uart1
extern i2c_inst_t host_i2c_inst[2];
#define i2c0 (&host_i2c_inst[0])
#define i2c1 (&host_i2c_inst[1])
typedef enum { UART_PARITY_NONE, UART_PARITY_EVEN, UART_PARITY_ODD } uart_parity_t;
static inline uint uart_get_index(uart_inst_t *u){ return u == host_uart1; }
static inline uart_inst_t *uart_get_instance(uint n){ return n ? host_uart1 : host_uart0; }
static inline uint i2c_get_index(i2c_inst_t *i){ return i == i2c1; }
static inline i2c_inst_t *i2c_get_instance(uint n){ return n ? i2c1 : i2c0; }
static inline i2c_hw_t *i2c_get_hw(i2c_inst_t *i){ return i->hw; }
#define UART_IRQ_NUM(u) (uart_get_index(u) ? UART1_IRQ : UART0_IRQ)
#define I2C_IRQ_NUM(i) (i2c_get_index(i) ? I2C1_IRQ : I2C0_IRQ)
typedef enum i2c_slave_event_t { I2C_SLAVE_RECEIVE, I2C_SLAVE_REQUEST, I2C_SLAVE_FINISH } i2c_slave_event_t;
typedef void (*i2c_slave_handler_t)(i2c_inst_t *i2c, i2c_slave_event_t event);
bool uart_is_readable(uart_inst_t *u);
char uart_getc(uart_inst_t *u);
uint8_t i2c_read_byte_raw(i2c_inst_t *i);
size_t i2c_get_read_available(i2c_inst_t *i);
size_t i2c_get_write_available(i2c_inst_t *i);
void i2c_write_byte_raw(i2c_inst_t *i, uint8_t v);

/* pio / dma */
typedef struct pio_hw { io_rw_32 ctrl, fstat, fdebug, flevel, txf[4], rxf[4], irq, irq_force, input_sync_bypass, dbg_padout, dbg_padoe, dbg_cfginfo, instr_mem[32]; struct { io_rw_32 clkdiv, execctrl, shiftctrl, addr, instr, pinctrl; } sm[4]; io_rw_32 gpiobase; } pio_hw_t;
typedef pio_hw_t *PIO;
extern pio_hw_t host_pio_hw[3];
#define pio0 (&host_pio_hw[0])
#define pio1 (&host_pio_hw[1])
#define pio2 (&host_pio_hw[2])
#define pio0_hw pio0
#define pio1_hw pio1
#define pio2_hw pio2
#define NUM_PIOS 3
#define NUM_PIO_STATE_MACHINES 4
typedef struct { uint32_t clkdiv, execctrl, shiftctrl, pinctrl, pinhi; } pio_sm_config;
typedef struct pio_program { const uint16_t *instructions; uint8_t length; int8_t origin; uint8_t pio_version; uint8_t used_gpio_ranges; } pio_program_t;
enum pio_fifo_join { PIO_FIFO_JOIN_NONE=0, PIO_FIFO_JOIN_TX=1, PIO_FIFO_JOIN_RX=2 };
enum pio_mov_status_type { STATUS_TX_LESSTHAN=0, STATUS_RX_LESSTHAN=1 };
static inline uint pio_get_index(PIO p){ return (uint)(p - host_pio_hw); }
static inline PIO pio_get_instance(uint n){ return &host_pio_hw[n % 3]; }
static inline uint pio_get_dreq(PIO p, uint sm, bool tx){ (void)p; (void)sm; (void)tx; return 0; }
static inline pio_sm_config pio_get_default_sm_config(void){ pio_sm_config c = {0}; return c; }
typedef struct { uint32_t ctrl; } dma_channel_config;
enum dma_channel_transfer_size { DMA_SIZE_8=0, DMA_SIZE_16=1, DMA_SIZE_32=2 };
typedef struct { io_rw_32 read_addr, write_addr, transfer_count, ctrl_trig, al1_ctrl, al1_read_addr, al1_write_addr, al1_transfer_count_trig, al2_ctrl, al2_transfer_count, al2_read_addr, al2_write_addr_trig, al3_ctrl, al3_write_addr, al3_transfer_count, al3_read_addr_trig; } dma_channel_hw_t;
typedef struct { dma_channel_hw_t ch[16]; io_rw_32 intr, inte0, intf0, ints0, inte1, intf1, ints1, inte2, intf2, ints2, inte3, intf3, ints3, timer[4], multi_channel_trigger, sniff_ctrl, sniff_data, fifo_levels, abort; } dma_hw_t;
extern dma_hw_t host_dma_hw;
#define dma_hw (&host_dma_hw)
typedef struct { struct { io_rw_32 dbg_ctdreq, dbg_tcr, pad[14]; } ch[16]; } dma_debug_hw_t;
extern dma_debug_hw_t host_dma_debug_hw;
#define dma_debug_hw (&host_dma_debug_hw)
static inline dma_channel_hw_t *dma_channel_hw_addr(uint ch){ return &host_dma_hw.ch[ch & 15]; }
static inline dma_channel_config dma_channel_get_default_config(uint ch){ (void)ch; dma_channel_config c = {0}; return c; }
#define DREQ_PIO0_TX0 0
#define DREQ_FORCE 0x3f
#define NUM_DMA_CHANNELS 16
#define DMA_CH0_CTRL_TRIG_BUSY_BITS 0x04000000u
#define DMA_CH0_CTRL_TRIG_EN_BITS 1u

/* clocks */
enum clock_index { clk_gpout0=0, clk_gpout1, clk_gpout2, clk_gpout3, clk_ref, clk_sys, clk_peri, clk_hstx, clk_usb, clk_adc, CLK_COUNT };
typedef enum clock_index clock_handle_t;
static inline uint32_t clock_get_hz(enum clock_index c){ (void)c; return SYS_CLK_HZ; }
#define GPIO_TO_GPOUT_CLOCK_HANDLE(g, d) (((g)==13||(g)==21)?clk_gpout0:((g)==15||(g)==23)?clk_gpout1:(g)==24?clk_gpout2:(g)==25?clk_gpout3:(d))
#define CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_SYS 6
#define CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS 0
#define CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB 3
#define CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_USB 7
#define CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_XOSC_CLKSRC 5
#define CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS 0
#define CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX 1
#define CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS 0
#define CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC 2
typedef struct { struct { io_rw_32 ctrl, div, selected; } clk[CLK_COUNT]; io_rw_32 fc0_ref_khz, fc0_min_khz, fc0_max_khz, fc0_delay, fc0_interval, fc0_src, fc0_status, fc0_result; } clocks_hw_t;
extern clocks_hw_t host_clocks_hw;
#define clocks_hw (&host_clocks_hw)
typedef struct pll_hw { io_rw_32 cs, pwr, fbdiv_int, prim; } pll_hw_t;
typedef pll_hw_t *PLL;
extern pll_hw_t host_pll_hw[2];
#define pll_sys (&host_pll_hw[0])
#define pll_usb (&host_pll_hw[1])
#define PLL_CS_LOCK_BITS 0x80000000u
#define PLL_PRIM_POSTDIV1_LSB 16
#define PLL_PRIM_POSTDIV2_LSB 12
#define PLL_PRIM_POSTDIV1_BITS 0x00070000u
#define PLL_PRIM_POSTDIV2_BITS 0x00007000u
#define PLL_FBDIV_INT_BITS 0xfffu
#define PLL_CS_REFDIV_BITS 0x3fu

/* qmi / nvic / bus ctrl / accessctrl */
typedef struct { struct { io_rw_32 timing, rfmt, rcmd, wfmt, wcmd; } m[2]; io_rw_32 direct_csr, direct_tx, direct_rx, atrans[8]; } qmi_hw_t;
extern qmi_hw_t host_qmi_hw;
#define qmi_hw (&host_qmi_hw)
#define QMI_M0_TIMING_CLKDIV_LSB 0
#define QMI_M0_TIMING_CLKDIV_BITS 0xffu
#define QMI_M0_TIMING_RXDELAY_LSB 8
#define QMI_M0_TIMING_RXDELAY_BITS 0x700u
#define QMI_M0_TIMING_COOLDOWN_LSB 30
#define QMI_M0_TIMING_COOLDOWN_BITS 0xc0000000u
typedef struct { io_rw_32 iser[2]; io_rw_32 pad0[30]; io_rw_32 icer[2]; io_rw_32 pad1[30]; io_rw_32 ispr[2]; io_rw_32 pad2[30]; io_rw_32 icpr[2]; io_rw_32 pad3[30]; io_rw_32 iabr[2]; io_rw_32 pad4[62]; io_rw_32 ipr[16]; } nvic_hw_t;
extern nvic_hw_t host_nvic_hw;
#define nvic_hw (&host_nvic_hw)
typedef struct { io_rw_32 priority, priority_ack, perfctr_en, perf[8]; } bus_ctrl_hw_t;
extern bus_ctrl_hw_t host_bus_ctrl_hw;
#define bus_ctrl_hw (&host_bus_ctrl_hw)
#define BUSCTRL_BUS_PRIORITY_PROC0_BITS 1u
#define BUSCTRL_BUS_PRIORITY_PROC1_BITS 0x10u
#define BUSCTRL_BUS_PRIORITY_DMA_R_BITS 0x100u
#define BUSCTRL_BUS_PRIORITY_DMA_W_BITS 0x1000u
typedef struct { io_rw_32 lock, force_core_ns, cfgreset, gpio_nsmask[2], regs[64]; } accessctrl_hw_t;
extern accessctrl_hw_t host_accessctrl_hw;
#define accessctrl_hw (&host_accessctrl_hw)

/* flash: XIP_BASE is a RAM array */
/* Only the top HOST_FLASH_REGION_BYTES of the 4 MB flash (the preset store) are
 * backed by RAM; XIP_BASE is placed so that flash offset N maps to it. */
#define HOST_FLASH_REGION_BYTES (32u * 4096u)
extern uint8_t host_flash[];
#define XIP_BASE ((uintptr_t)host_flash - (PICO_FLASH_SIZE_BYTES - HOST_FLASH_REGION_BYTES))
#define XIP_NOCACHE_NOALLOC_BASE XIP_BASE
#define FLASH_PAGE_SIZE (1u << 8)
#define FLASH_SECTOR_SIZE (1u << 12)
#define FLASH_BLOCK_SIZE (1u << 16)
#define PICO_FLASH_SIZE_BYTES (4u * 1024u * 1024u)
#define FLASH_UNIQUE_ID_SIZE_BYTES 8
void flash_range_erase(uint32_t flash_offs, size_t count);
void flash_range_program(uint32_t flash_offs, const uint8_t *data, size_t count);

/* bootrom */
typedef void (*rom_connect_internal_flash_fn)(void);
typedef void (*rom_flash_exit_xip_fn)(void);
typedef void (*rom_flash_range_erase_fn)(uint32_t, size_t, uint32_t, uint8_t);
typedef void (*rom_flash_range_program_fn)(uint32_t, const uint8_t *, size_t);
typedef void (*rom_flash_flush_cache_fn)(void);
typedef void (*rom_flash_enter_cmd_xip_fn)(void);
void *rom_func_lookup_inline(uint32_t code);
void *rom_func_lookup(uint32_t code);
#define ROM_TABLE_CODE(c1, c2) ((c1) | ((c2) << 8))
#define ROM_FUNC_CONNECT_INTERNAL_FLASH ROM_TABLE_CODE('I','F')
#define ROM_FUNC_FLASH_EXIT_XIP ROM_TABLE_CODE('E','X')
#define ROM_FUNC_FLASH_RANGE_ERASE ROM_TABLE_CODE('R','E')
#define ROM_FUNC_FLASH_RANGE_PROGRAM ROM_TABLE_CODE('R','P')
#define ROM_FUNC_FLASH_FLUSH_CACHE ROM_TABLE_CODE('F','C')
#define ROM_FUNC_FLASH_ENTER_CMD_XIP ROM_TABLE_CODE('C','X')

/* pwm / adc / vreg */
typedef struct { uint32_t csr, div, top; } pwm_config;
static inline pwm_config pwm_get_default_config(void){ pwm_config c = {0}; return c; }
enum vreg_voltage { VREG_VOLTAGE_0_85=0, VREG_VOLTAGE_0_90, VREG_VOLTAGE_0_95, VREG_VOLTAGE_1_00, VREG_VOLTAGE_1_05,
  VREG_VOLTAGE_1_10, VREG_VOLTAGE_1_15, VREG_VOLTAGE_1_20, VREG_VOLTAGE_1_25, VREG_VOLTAGE_1_30,
  VREG_VOLTAGE_1_35, VREG_VOLTAGE_1_40, VREG_VOLTAGE_1_50, VREG_VOLTAGE_1_60, VREG_VOLTAGE_1_65,
  VREG_VOLTAGE_1_70, VREG_VOLTAGE_1_80, VREG_VOLTAGE_1_90, VREG_VOLTAGE_2_00, VREG_VOLTAGE_2_35,
  VREG_VOLTAGE_2_50, VREG_VOLTAGE_2_65, VREG_VOLTAGE_2_80, VREG_VOLTAGE_3_00, VREG_VOLTAGE_3_15,
  VREG_VOLTAGE_3_30, VREG_VOLTAGE_DEFAULT=VREG_VOLTAGE_1_10, VREG_VOLTAGE_MAX=VREG_VOLTAGE_3_30 };

/* multicore */
static inline bool multicore_fifo_rvalid(void){ return false; }
static inline bool multicore_fifo_wready(void){ return true; }

/* timers / alarms */
typedef struct { io_rw_32 timehw, timelw, timehr, timelr, alarm[4], armed, timerawh, timerawl, dbgpause, pause, locked, source, intr, inte, intf, ints; } timer_hw_t;
extern timer_hw_t host_timer_hw;
#define timer_hw (&host_timer_hw)
#define PICO_DEFAULT_TIMER_INSTANCE() (&host_timer_hw)
typedef struct alarm_pool alarm_pool_t;
typedef int32_t alarm_id_t;
typedef struct repeating_timer { int64_t delay_us; alarm_pool_t *pool; alarm_id_t alarm_id; void *callback; void *user_data; } repeating_timer_t;
static inline void hw_set_bits(io_rw_32 *a, uint32_t m){ *a |= m; }
static inline void hw_clear_bits(io_rw_32 *a, uint32_t m){ *a &= ~m; }
static inline void hw_xor_bits(io_rw_32 *a, uint32_t m){ *a ^= m; }
static inline void hw_write_masked(io_rw_32 *a, uint32_t v, uint32_t m){ *a = (*a & ~m) | (v & m); }
/* uart hw: uart_is_readable() (harness) loads the next byte into dr */
typedef struct { io_rw_32 dr, rsr, pad[4], fr, pad2, ilpr, ibrd, fbrd, lcr_h, cr, ifls, imsc, ris, mis, icr, dmacr; } uart_hw_t;
extern uart_hw_t host_uart_hw[2];
static inline uart_hw_t *uart_get_hw(uart_inst_t *u){ return &host_uart_hw[uart_get_index(u)]; }
#define UART_UARTFR_TXFF_BITS 0x20u
#define UART_UARTFR_RXFE_BITS 0x10u
#define UART_UARTFR_BUSY_BITS 0x08u
#define UART_UARTIMSC_RXIM_BITS 0x10u
#define UART_UARTIMSC_RTIM_BITS 0x40u
#define UART_UARTIMSC_TXIM_BITS 0x20u
/* pio encoding */
enum pio_src_dest { pio_pins = 0, pio_x = 1, pio_y = 2, pio_null = 3, pio_pindirs = 4, pio_exec_mov = 4, pio_status = 5, pio_pc = 5, pio_isr = 6, pio_osr = 7, pio_exec_out = 7 };
dma_channel_config dma_get_channel_config(uint ch);
#define DMA_CH0_TRANS_COUNT_MODE_VALUE_ENDLESS 0xfu
#define DMA_CH0_TRANS_COUNT_MODE_LSB 28
#define NUM_ADC_CHANNELS 5
#define QMI_DIRECT_CSR_CLKDIV_LSB 22
#define QMI_DIRECT_CSR_CLKDIV_BITS 0x3fc00000u
#endif
