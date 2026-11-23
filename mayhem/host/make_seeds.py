#!/usr/bin/env python3
"""Write the fuzz seeds: mayhem/host/make_seeds.py <bulk.bin> <flash.bin> (both dumped by
mayhem/out/dspi_selftest with DSPI_DUMP_BULK / DSPI_DUMP_FLASH)."""
import os, struct, sys
M = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")

def put(target, name, data):
    d = os.path.join(M, target, "testsuite"); os.makedirs(d, exist_ok=True)
    open(os.path.join(d, name), "wb").write(data)

def crc16(b):
    c = 0xFFFF
    for x in b:
        c ^= x << 8
        for _ in range(8):
            c = ((c << 1) ^ 0x1021) & 0xFFFF if c & 0x8000 else (c << 1) & 0xFFFF
    return c

def uart(typ, req, wval=0, widx=0, payload=b"", wlen=None):
    wlen = len(payload) if wlen is None else wlen
    body = struct.pack("<BBHHH", typ, req, wval, widx, wlen) + payload
    return b"\xA5" + body + struct.pack("<H", crc16(body))

def usb(bmrt, req, wval=0, widx=0, wlen=0, data=b""):
    return struct.pack("<BBHHHH", bmrt, req, wval, widx, wlen, len(data)) + data

def i2c_frame(typ, req, wval=0, widx=0, payload=b"", wlen=None):
    wlen = len(payload) if wlen is None else wlen
    return struct.pack("<BBHHH", typ, req, wval, widx, wlen) + payload

eq = struct.pack("<BBBBfff", 9, 2, 1, 0, 1000.0, 0.707, -3.0)   # channel 9 (out 1), band 2, peaking
bulk = open(sys.argv[1], "rb").read()
flash = open(sys.argv[2], "rb").read()

# UART: SET EQ then GET it back; GET status; GET build info; bulk SET
put("fuzz-uart-ctrl", "eq_set_get", uart(1, 0x42, payload=eq) + uart(2, 0x43, wval=(9 << 8) | (2 << 3) | 1))
put("fuzz-uart-ctrl", "get_status", uart(2, 0x50, wval=0) + uart(2, 0x80))
put("fuzz-uart-ctrl", "bulk_set", uart(1, 0xA1, payload=bulk))
# USB EP0
put("fuzz-usb-vendor", "eq_set_get", usb(0x41, 0x42, wlen=len(eq), data=eq) + usb(0xC1, 0x43, (9 << 8) | (2 << 3) | 1, 0, 4))
put("fuzz-usb-vendor", "ms_os_status", usb(0xC0, 0x01, 0, 7, 178) + usb(0xC1, 0x50, 0, 0, 64) + usb(0xC1, 0xF9, 0, 0, 8))
put("fuzz-usb-vendor", "bulk_chunks", usb(0x41, 0xA2, 0, 0, 64, bulk[:64]) + usb(0x41, 0xA2, 64, 0, 64, bulk[64:128]) + usb(0xC1, 0xA0, 0, 0, 6136))
# I2C: write a SET frame, poll, read response; GET status
w = i2c_frame(1, 0x42, payload=eq)
put("fuzz-i2c-ctrl", "eq_set", bytes([0, len(w)]) + w + b"\x02\x02" + bytes([1, 3]))
g = i2c_frame(2, 0x50, wlen=0)
put("fuzz-i2c-ctrl", "get_status", bytes([0, len(g)]) + g + b"\x02\x02\x03\x01" + bytes([1, 40]))
# Bulk blob
put("fuzz-bulk-params", "defaults_v32", bulk)
# Preset flash image (directory + slot 3 written by preset_save)
put("fuzz-preset-flash", "slot3_saved", flash)
# IR: NEC frame addr 0x04 cmd 0x08, then a repeat
def nec(frame):
    d = [9000, 4500]
    for i in range(32):
        d += [562, 1687 if (frame >> i) & 1 else 562]
    return d + [562]
seq = [50000] + nec(0xF708FB04) + [40000, 9000, 2250, 562]
put("fuzz-ir-rx", "nec_0408", b"\x00" + b"".join(struct.pack("<H", min(x, 65535)) for x in seq))
rc5 = [50000] + [889] * 26
put("fuzz-ir-rx", "rc5_learn", b"\x02" + b"".join(struct.pack("<H", x) for x in rc5))
# UAC1 class driver: op-tagged events (see mayhem/fuzz-uac1/fuzz_uac1.c)
def uctl(bmrt, req, wval=0, widx=0, wlen=0, data=b""):
    return b"\x00" + struct.pack("<BBHHH", bmrt, req, wval, widx, wlen) + bytes([len(data)]) + data
def iso(pcm):
    return b"\x01" + struct.pack("<H", len(pcm)) + pcm
def sof(frame, dt=125):
    return b"\x02" + struct.pack("<HB", frame, dt)
tone = b"".join(struct.pack("<hh", (i * 997) % 20000 - 10000, -((i * 991) % 20000 - 10000)) for i in range(48))
stream = uctl(0x01, 11, 1, 1) + uctl(0x22, 0x01, 0x0100, 0x01, 3, struct.pack("<I", 48000)[:3])
for f in range(8):
    stream += iso(tone) + sof(f) + b"\x03\x00"
stream += uctl(0xA2, 0x81, 0x0100, 0x01, 3) + b"\x04" + uctl(0x01, 11, 2, 1) + iso(tone * 2) + uctl(0x01, 11, 0, 1)
put("fuzz-uac1", "stream_48k", stream)
vol = uctl(0x21, 0x01, 0x0200, 2 << 8, 2, struct.pack("<h", -10 * 256)) + uctl(0xA1, 0x81, 0x0200, 2 << 8, 2) \
    + uctl(0x21, 0x01, 0x0100, 2 << 8, 1, b"\x01") + uctl(0xA1, 0x81, 0x0100, 2 << 8, 1) \
    + uctl(0xA1, 0x82, 0x0200, 2 << 8, 2) + uctl(0x81, 10, 0, 1, 1) + b"\x04\x03\x01"
put("fuzz-uac1", "volume_mute", vol)
put("fuzz-uac1", "multich_alt5", uctl(0x01, 11, 5, 1) + iso(bytes(range(256)) * 3) + sof(1) + b"\x04")
