#!/usr/bin/env python3
import struct

# Filenames
png_in  = "cat.png"
mp4_in  = "spinner.mp4"
txt_in  = "text.txt"
out     = "polyglot.png"

# Read entire PNG
data = open(png_in, "rb").read()

# Skip the 8-byte PNG signature
offset = 8
iend_end = None

# Walk through chunks until IEND
while offset < len(data):
    length     = struct.unpack(">I", data[offset:offset+4])[0]
    chunk_type = data[offset+4:offset+8]
    # IEND chunk found?
    if chunk_type == b"IEND":
        # length+type+CRC = 4+4+4 bytes
        iend_end = offset + 12 + length
        break
    offset += length + 12  # move to next chunk

if iend_end is None:
    raise RuntimeError("IEND chunk not found in PNG!")

# Write out the polyglot
with open(out, "wb") as w:
    w.write(data[:iend_end])
    w.write(open(mp4_in, "rb").read())
    w.write(open(txt_in, "rb").read())

print(f"Created {out} (IEND ends at byte {iend_end})")

