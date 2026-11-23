/* Per-input reset of the firmware's global state (every global from the
 * firmware TUs and the host register/flash model lives between these
 * symbols, placed by mayhem/host/dspi.ld).  The copy loops are not
 * ASan-instrumented: the range includes ASan's global redzones, which are
 * only ever restored to their own (zero) contents. */
#include <stdlib.h>
#include <stdint.h>
#include "host_io.h"
extern char __dspi_state_data_start[], __dspi_state_data_end[];
extern char __dspi_state_bss_start[], __dspi_state_bss_end[];
static char *saved_data;

__attribute__((no_sanitize("address", "undefined")))
static void copy_bytes(volatile char *d, const volatile char *s, size_t n) { for (size_t i = 0; i < n; i++) d[i] = s[i]; }
__attribute__((no_sanitize("address", "undefined")))
static void zero_bytes(volatile char *d, size_t n) { volatile uint64_t *w = (volatile uint64_t *)d; for (size_t i = 0; i < n / 8; i++) w[i] = 0; for (size_t i = n & ~(size_t)7; i < n; i++) d[i] = 0; }

void host_state_snapshot(void) {
    size_t n = (size_t)(__dspi_state_data_end - __dspi_state_data_start);
    saved_data = malloc(n ? n : 1);
    copy_bytes(saved_data, __dspi_state_data_start, n);
}
void host_state_restore(void) {
    copy_bytes(__dspi_state_data_start, saved_data, (size_t)(__dspi_state_data_end - __dspi_state_data_start));
    zero_bytes(__dspi_state_bss_start, (size_t)(__dspi_state_bss_end - __dspi_state_bss_start));
}
