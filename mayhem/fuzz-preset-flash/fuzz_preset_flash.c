/* Preset store in flash: the fuzz input is the image of the preset region
 * (directory sector followed by the ten slot sectors) that the firmware
 * finds in flash at boot.  preset_boot_load() parses and sanitizes it, then
 * every slot is loaded, the directory is read back, and slot 0 is saved, as
 * the vendor preset commands do. */
#include "host_io.h"
#include "harness_common.h"
#include "flash_storage.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    harness_boot(data, size);
    uint16_t occ = 0; uint8_t mode = 0, def = 0, last = 0, ocm = 0, mvm = 0;
    preset_get_directory(&occ, &mode, &def, &last, &ocm, &mvm);
    char name[64];
    for (uint8_t s = 0; s < 10; s++) {
        (void)preset_get_name(s, name);
        if (occ & (1u << s)) (void)preset_load(s);
    }
    (void)preset_save(0);
    (void)preset_load(0);
    return 0;
}
