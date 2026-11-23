#!/usr/bin/env bash
# Usage: gen_usb_audio.sh <in usb_audio.c> <out .c>
# Host copy of usb_audio.c: the one ARM VFP inline-asm block (unpack_s24x4) is
# replaced by its C equivalent; #line keeps diagnostics on the original file.
set -euo pipefail
in=$1 out=$2
awk -v src="$in" '
BEGIN { print "#line 1 \"" src "\"" }
/unpack_s24x4\(const/ { inunpack=1 }
inunpack && /__asm__ volatile \(/ { skip=1;
  print "    (void)temp;";
  print "    f0 = (float)(((int32_t)((uint32_t)i0 << 8)) >> 8);";
  print "    f1 = (float)(((int32_t)(((((uint32_t)i1 & 0xFFFFu) << 8) | (((uint32_t)i0 >> 24) & 0xFFu)) << 8)) >> 8);";
  print "    f2 = (float)(((int32_t)(((((uint32_t)i1 >> 16) & 0xFFFFu) | (((uint32_t)i2 & 0xFFu) << 16)) << 8)) >> 8);";
  print "    f3 = (float)(i2 >> 8);";
  next }
skip { if ($0 ~ /^    \);/) { skip=0; inunpack=0; print "#line " NR+1 " \"" src "\"" } ; next }
{ print }
' "$in" > "$out"
