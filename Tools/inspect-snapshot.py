"""Bounded metadata inspection for the checked Unreal 5.8 tagged SaveGame envelope."""
import json
import struct
import sys
import zlib
from pathlib import Path

b = Path(sys.argv[1]).read_bytes()
magic, size, crc = struct.unpack_from("<III", b)
if magic != 0x56545332 or size != len(b) - 12 or zlib.crc32(b[12:]) != crc:
    raise ValueError("Snapshot envelope failed validation")
pos = b.index(b"Players\0")
end = b.index(b"/Script/VoidAndThunder\0", pos) + len(b"/Script/VoidAndThunder\0")
# UE 5.8: array index (4), payload size (4), property GUID flag (1), array count (4).
count = struct.unpack_from("<I", b, end + 9)[0]
if count > 10000 or b[end + 17:end + 25] != b"Profile\0":
    raise ValueError("Unexpected Players property layout; use native Unreal inspection")
print(json.dumps({"checksum_valid": True, "players": count, "bytes": len(b)}))
