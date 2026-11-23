#!/usr/bin/env bash
#
# mayhem/build.sh — host (x86) build of the DSPi firmware control plane for fuzzing.
#
# The firmware is bare-metal RP2350 code (Pico SDK + TinyUSB, cross-compiled
# upstream).  Here every firmware .c that the control plane reaches is compiled
# for the host against the stub layer in mayhem/host/include (memory-backed
# registers and flash, captured IRQ handlers); the audio-capture/descriptor
# files that are pure PIO/DMA/clock register code are left out, and every SDK
# call they or the rest leave undefined becomes a generated no-op.
set -euo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS
: "${SRC:=$(cd "$(dirname "$0")/.." && pwd)}"
cd "$SRC"

FW=firmware/DSPi
H=mayhem/host
OUT=mayhem/out
TARGETS="fuzz_uart_ctrl fuzz_usb_vendor fuzz_i2c_ctrl fuzz_bulk_params fuzz_preset_flash fuzz_ir_rx fuzz_uac1"
# Targets that also link usb_audio.c (the UAC1 class driver), host-built from a copy whose one
# ARM VFP asm block is replaced by C (mayhem/host/gen_usb_audio.sh); the others keep it stubbed.
UAC_TARGETS="fuzz_uac1"
# Pure PIO/DMA/clock-tree code (audio capture, descriptors, VCXO, flash clock), main.c (owns main()),
# and the two files with inline ARM assembly (pdm_generator.c, usb_audio.c) — not built for the host.
EXCLUDE="adat_input.c i2s_input.c spdif_input.c usb_descriptors.c soft_vcxo.c flash_clkdiv.c main.c pdm_generator.c usb_audio.c"

INC="-I$H/include -I$FW -I firmware/pico-extras/src/rp2_common/pico_audio_spdif_multi/include \
 -I firmware/pico-extras/src/rp2_common/pico_audio_i2s_multi/include -I firmware/pico-extras/src/rp2_common/pico_spdif_rx/include"
# -fcommon as upstream's CMakeLists; implicit declarations stand for SDK calls the stubs leave undeclared.
BASE="-std=gnu11 -O1 -fcommon -include $H/include/dspi_host.h $INC -Wno-implicit-function-declaration -Wno-int-conversion -Wno-incompatible-pointer-types -Wno-everything"

build_set() {  # $1 = out dir, $2 = extra cflags
  local d=$1 fl=$2; rm -rf "$d"; mkdir -p "$d/h"
  local srcs=()
  for f in "$FW"/*.c; do
    case " $EXCLUDE " in *" $(basename "$f") "*) continue;; esac
    srcs+=("$f")
  done
  printf '%s\n' "${srcs[@]}" "$H/sdk_stubs.c" | xargs -P "$MAYHEM_JOBS" -I{} sh -c \
    'o="$0/$(basename "$1" .c).o"; '"$CC"' '"$BASE"' -DDSPI_HOST_STATE_SECTIONS '"$fl"' -c "$1" -o "$o"' "$d" {}
  $CC $BASE $fl -c "$H/state.c" -o "$d/state.o"
  $CC $BASE $fl -c "$H/harness_common.c" -o "$d/harness_common.o"
  $CC $fl -c mayhem/lsan_off.c -o "$d/lsan_off.o"
}

# nonnull-attribute relaxed: vendor_send_response() memcpy()s zero-length NULL responses
# (benign) on every transport, which would halt each external-transport target at once.
FUZZ_FLAGS="$SANITIZER_FLAGS $DEBUG_FLAGS -fsanitize=fuzzer-no-link"
case "$SANITIZER_FLAGS" in *undefined*) FUZZ_FLAGS="$FUZZ_FLAGS -fno-sanitize=nonnull-attribute";; esac
build_set "$OUT/fuzz" "$FUZZ_FLAGS"
OBJS=$(ls "$OUT"/fuzz/*.o)
for t in $TARGETS; do d=$(echo "$t" | tr _ -); [ -f "mayhem/$d/$t.c" ] && $CC $BASE $FUZZ_FLAGS -c "mayhem/$d/$t.c" -o "$OUT/fuzz/h/$t.o"; done
bash $H/gen_stubs.sh "$OUT/fuzz/stubs.c" $OBJS "$OUT"/fuzz/h/*.o
$CC $DEBUG_FLAGS -c "$OUT/fuzz/stubs.c" -o "$OUT/fuzz/stubs.o"
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$OUT/fuzz/standalone_main.o.x"
mkdir -p "$OUT/fuzz/uac"
bash $H/gen_usb_audio.sh "$FW/usb_audio.c" "$OUT/fuzz/uac/usb_audio_host.c"
$CC $BASE -DDSPI_HOST_STATE_SECTIONS $FUZZ_FLAGS -c "$OUT/fuzz/uac/usb_audio_host.c" -o "$OUT/fuzz/uac/usb_audio.o.x"
UAC_OBJS=""; for t in $UAC_TARGETS; do UAC_OBJS="$UAC_OBJS $OUT/fuzz/h/$t.o"; done
bash $H/gen_stubs.sh "$OUT/fuzz/uac/stubs.c" $OBJS "$OUT/fuzz/uac/usb_audio.o.x" $UAC_OBJS
$CC $DEBUG_FLAGS -c "$OUT/fuzz/uac/stubs.c" -o "$OUT/fuzz/uac/stubs.o.x"
for t in $TARGETS; do
  [ -f "$OUT/fuzz/h/$t.o" ] || continue
  extra="$OUT/fuzz/stubs.o"
  case " $UAC_TARGETS " in *" $t "*) extra="$OUT/fuzz/uac/usb_audio.o.x $OUT/fuzz/uac/stubs.o.x";; esac
  $CC $FUZZ_FLAGS $LIB_FUZZING_ENGINE "$OUT/fuzz/h/$t.o" $OBJS $extra \
      -Wl,-T,$H/dspi.ld -lm -o "/mayhem/$t"
  $CC $FUZZ_FLAGS "$OUT/fuzz/standalone_main.o.x" "$OUT/fuzz/h/$t.o" $OBJS $extra \
      -Wl,-T,$H/dspi.ld -lm -o "/mayhem/$t-standalone"
done

# Test build (normal flags, no sanitizers): the regression driver over the same firmware objects.
TEST_FLAGS="-O1 $COVERAGE_FLAGS"
build_set "$OUT/test" "$TEST_FLAGS"
rm -f "$OUT/test/lsan_off.o"; TOBJS=$(ls "$OUT"/test/*.o)
bash $H/gen_stubs.sh "$OUT/test/stubs.c" $TOBJS
$CC -c "$OUT/test/stubs.c" -o "$OUT/test/stubs.o"
$CC $BASE $TEST_FLAGS mayhem/tests/dspi_selftest.c $TOBJS "$OUT/test/stubs.o" -Wl,-T,$H/dspi.ld -lm -o "$OUT/dspi_selftest"
