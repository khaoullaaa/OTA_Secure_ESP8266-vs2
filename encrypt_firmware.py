#!/usr/bin/env python3
"""
AES-256-CTR Firmware Encryption Script
Encrypts firmware.bin with AES-256 in CTR mode and prepends IV

Uses a simple CTR counter format compatible with BearSSL:
- IV (16 bytes): First 12 bytes = nonce, Last 4 bytes = initial counter (big-endian)
"""

import os
import sys
import struct
from Crypto.Cipher import AES
from Crypto.Random import get_random_bytes

# AES-256 key (must match the key in OTA_Secure.ino)
AES_KEY = bytes([
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
    0x76, 0x2e, 0x71, 0x60, 0xf3, 0x8b, 0x4d, 0xa5,
    0x6a, 0x78, 0x4d, 0x90, 0x45, 0x19, 0x0c, 0xfe
])

def encrypt_firmware(input_file, output_file):
    """Encrypt firmware with AES-256-CTR"""
    
    print(f"[ENCRYPT] Reading firmware: {input_file}")
    with open(input_file, 'rb') as f:
        plaintext = f.read()
    
    print(f"[ENCRYPT] Firmware size: {len(plaintext)} bytes")
    
    # Generate random nonce (12 bytes) + counter starts at 1 (4 bytes big-endian)
    nonce = get_random_bytes(12)
    initial_counter = 1
    iv = nonce + struct.pack('>I', initial_counter)  # Big-endian 32-bit counter
    
    print(f"[ENCRYPT] Nonce (12 bytes): {nonce.hex()}")
    print(f"[ENCRYPT] Full IV (16 bytes): {iv.hex()}")
    
    # Create AES cipher in CTR mode with nonce and initial counter
    cipher = AES.new(AES_KEY, AES.MODE_CTR, nonce=nonce, initial_value=initial_counter)
    
    # Encrypt the firmware
    ciphertext = cipher.encrypt(plaintext)
    print(f"[ENCRYPT] Encrypted size: {len(ciphertext)} bytes")
    
    # Write IV + encrypted firmware
    with open(output_file, 'wb') as f:
        f.write(iv)  # 16 bytes: 12-byte nonce + 4-byte counter
        f.write(ciphertext)
    
    total_size = len(iv) + len(ciphertext)
    print(f"[ENCRYPT] Total output size: {total_size} bytes (IV + encrypted)")
    print(f"[ENCRYPT] ✓ Encrypted firmware saved: {output_file}")
    
    return len(plaintext)  # Return original size for manifest

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: encrypt_firmware.py <input.bin> <output.bin>")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    if not os.path.exists(input_file):
        print(f"Error: Input file not found: {input_file}")
        sys.exit(1)
    
    original_size = encrypt_firmware(input_file, output_file)
    print(f"\n[ENCRYPT] Encryption complete!")
    print(f"[ENCRYPT] Original: {input_file} ({original_size} bytes)")
    print(f"[ENCRYPT] Encrypted: {output_file} ({os.path.getsize(output_file)} bytes)")
