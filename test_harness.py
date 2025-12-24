#!/usr/bin/env python3
import subprocess
import struct
import sys

# Create a MIDI sequence: Note On (C4, vel 100) -> wait -> Note Off
midi_sequence = bytes([
    0x90, 0x3C, 0x64,  # Note On, C4, velocity 100
    0x80, 0x3C, 0x00,  # Note Off, C4, velocity 0
])

print("=" * 60)
print("SimpleSynthHost Test Harness - Batch Mode Test")
print("=" * 60)
print()

# Test 1: Basic rendering with 0.5 second duration
print("[TEST 1] Basic rendering - 0.5 seconds")
print("-" * 60)
print("Running: echo <MIDI> | SimpleSynthHost --duration 0.5")

try:
    proc = subprocess.Popen(
        [r"C:\code\juce\SimpleSynthHost\cmake-build\Debug\SimpleSynthHost.exe", 
         "--duration", "0.5"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        creationflags=subprocess.CREATE_NO_WINDOW
    )
    
    audio_data, stderr = proc.communicate(input=midi_sequence, timeout=10)
    
    print(f"Exit code: {proc.returncode}")
    if stderr:
        stderr_text = stderr.decode('utf-8', errors='ignore')
        if "[SimpleSynthHost] Batch mode" in stderr_text:
            print("✓ Batch mode detected")
        print(f"Stderr: {stderr_text[:200]}")
    
    print(f"Audio output size: {len(audio_data)} bytes")
    
    # Expected: 0.5 sec * 44100 Hz * 2 channels * 4 bytes = 176,400 bytes
    expected = int(0.5 * 44100 * 2 * 4)
    print(f"Expected size: ~{expected} bytes")
    
    if len(audio_data) > 0:
        print(f"✓ Generated audio output")
        if len(audio_data) >= expected * 0.9:  # Within 90% of expected
            print(f"✓ Output size reasonable (got {len(audio_data)}, expected {expected})")
        else:
            print(f"⚠ Output smaller than expected")
    else:
        print("✗ No audio output generated")
        
except subprocess.TimeoutExpired:
    print("✗ Process timed out after 10 seconds")
    proc.kill()
except Exception as e:
    print(f"✗ Error: {e}")

print()

# Test 2: With parameter
print("[TEST 2] Rendering with parameters")
print("-" * 60)
print("Running: ... | SimpleSynthHost --param Waveform=1 --duration 0.2")

try:
    proc = subprocess.Popen(
        [r"C:\code\juce\SimpleSynthHost\cmake-build\Debug\SimpleSynthHost.exe",
         "--param", "Waveform=1",
         "--duration", "0.2"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        creationflags=subprocess.CREATE_NO_WINDOW
    )
    
    audio_data, stderr = proc.communicate(input=midi_sequence, timeout=10)
    
    print(f"Exit code: {proc.returncode}")
    if len(audio_data) > 0:
        print(f"✓ Generated {len(audio_data)} bytes of audio")
        expected = int(0.2 * 44100 * 2 * 4)
        if len(audio_data) >= expected * 0.9:
            print(f"✓ Output size reasonable")
        else:
            print(f"⚠ Output smaller than expected")
    else:
        print("✗ No audio output generated")
        
except subprocess.TimeoutExpired:
    print("✗ Process timed out")
    proc.kill()
except Exception as e:
    print(f"✗ Error: {e}")

print()
print("=" * 60)
print("Test complete!")
print("=" * 60)
