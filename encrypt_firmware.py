#!/usr/bin/env python3
"""
AES-256-CBC Firmware Encryption Script
Encrypts firmware.bin with AES-256 in CBC mode with PKCS7 padding
Prepends original size + 16-byte IV to encrypted data

Format: [Original Size (4 bytes LE)][IV (16 bytes)][Encrypted data with PKCS7 padding]
"""

import os
import sys
from Crypto.Cipher import AES
from Crypto.Random import get_random_bytes
from Crypto.Util.Padding import pad

# AES-256 key (32 bytes) - must match the key in OTA_Secure.ino
AES_KEY = bytes([
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
    0x76, 0x2e, 0x71, 0x60, 0xf3, 0x8b, 0x4d, 0xa5,
    0x6a, 0x78, 0x4d, 0x90, 0x45, 0x19, 0x0c, 0xfe
])

def encrypt_firmware(input_file, output_file):
    """Encrypt firmware with AES-256-CBC"""
    
    print(f"[ENCRYPT] Reading firmware: {input_file}")
    with open(input_file, 'rb') as f:
        plaintext = f.read()
    
    original_size = len(plaintext)
    print(f"[ENCRYPT] Original firmware size: {original_size} bytes")
    
    # Generate random IV (16 bytes)
    iv = get_random_bytes(16)
    print(f"[ENCRYPT] Generated IV: {iv.hex()}")
    
    # Pad plaintext to 16-byte boundary (PKCS7)
    padded_plaintext = pad(plaintext, AES.block_size)
    padded_size = len(padded_plaintext)
    print(f"[ENCRYPT] Padded size: {padded_size} bytes (PKCS7 padding)")
    
    # Create AES cipher in CBC mode
    cipher = AES.new(AES_KEY, AES.MODE_CBC, iv)
    
    # Encrypt the firmware
    ciphertext = cipher.encrypt(padded_plaintext)
    print(f"[ENCRYPT] Encrypted size: {len(ciphertext)} bytes")
    
    # Write: original_size (4 bytes LE) + IV (16 bytes) + encrypted data
    with open(output_file, 'wb') as f:
        # Write original size as 4-byte little-endian (so ESP8266 knows actual firmware size)
        f.write(original_size.to_bytes(4, 'little'))
        f.write(iv)
        f.write(ciphertext)
    
    total_size = 4 + 16 + len(ciphertext)
    print(f"[ENCRYPT] Total output: {total_size} bytes (4 size + 16 IV + {len(ciphertext)} encrypted)")
    print(f"[ENCRYPT] ✓ Encrypted firmware saved: {output_file}")
    
    return original_size

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
