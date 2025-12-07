#!/usr/bin/env python3
"""
Create a simple disk image for testing ATA driver
"""

# Create a 10MB disk image (10 * 16 * 63 * 512 bytes)
# CHS geometry: 10 cylinders, 16 heads, 63 sectors per track
cylinders = 10
heads = 16
sectors = 63
sector_size = 512

total_size = cylinders * heads * sectors * sector_size

print(f"Creating disk image: {total_size} bytes ({total_size // (1024*1024)} MB)")
print(f"Geometry: C={cylinders}, H={heads}, S={sectors}")

# Create disk image filled with zeros
with open('bin/hdd.img', 'wb') as f:
    # Write zeros for the entire disk
    f.write(b'\x00' * total_size)
    
    # Write a simple boot signature to MBR (last 2 bytes of sector 0)
    f.seek(510)
    f.write(b'\x55\xAA')  # Boot signature
    
    print("Disk image created successfully at bin/hdd.img")
    print("Boot signature (0x55AA) written to MBR")

print("\nYou can now attach this disk in floppy.bxrc")
