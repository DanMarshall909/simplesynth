# SimpleSynth - JUCE Audio Plugin Test Harness

A minimal, composable VST3 synthesizer plugin and offline rendering host for testing and batch audio generation.

## Overview

SimpleSynth consists of two parts:

1. **SimpleSynth** - A VST3 synthesizer plugin with customizable waveforms and parameters
2. **SimpleSynthHost** - A dual-mode host application:
   - **Interactive mode**: UDP MIDI server for real-time playback
   - **Batch mode**: stdin/stdout test harness for automated audio generation

Designed with Unix philosophy: minimal, composable, no custom formats.

## Building

Requirements:
- JUCE 8.0.12
- CMake 3.24+
- C++17 compiler
- Windows with Winsock2 support (currently)

```bash
cmake -B build -S .
cmake --build build
```

Output:
- `SimpleSynth/cmake-build/SimpleSynth_artefacts/Debug/VST3/SimpleSynth.vst3`
- `SimpleSynthHost/cmake-build/Debug/SimpleSynthHost.exe`

## Usage

### Interactive Mode

Start the UDP MIDI server (listens on port 9999):

```bash
SimpleSynthHost
```

Send MIDI from another application to trigger the synth.

### Batch Mode - Test Harness

Generate audio from MIDI via stdin:

```bash
# Basic: MIDI from stdin, audio to stdout
cat midi_events.bin | SimpleSynthHost > output.raw

# With parameters
echo -ne '\x90\x3C\x64' | SimpleSynthHost --param Gain=0.5 > note.raw

# Set duration
generate_melody_sustained.sh | SimpleSynthHost --duration 6.0 > tune.raw

# Convert to WAV
SimpleSynthHost --duration 2.0 < midi.bin > audio.raw
sox -r 44100 -e float -b 32 -c 2 audio.raw audio.wav
```

### MIDI Input Format

Raw binary MIDI (3 bytes):

- **Note On**: `0x90 <note> <velocity>`
- **Note Off**: `0x80 <note> <velocity>`
- **Control Change**: `0xB0 <cc> <value>`

Example: C4 note at velocity 100
```bash
printf '\x90\x3C\x64'
```

### Parameters

Set plugin parameters via `--param`:

```bash
SimpleSynthHost --param Frequency=440 --param Gain=0.5
```

Available parameters depend on the plugin implementation.

## Testing

### Python Test Harness

```python
import subprocess

def test_note_generation():
    midi = bytes([0x90, 0x3C, 0x64])  # Note On C4
    proc = subprocess.Popen(
        ['SimpleSynthHost', '--duration', '1'],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE
    )
    audio, _ = proc.communicate(input=midi)
    assert len(audio) > 0  # Verify audio was generated
```

### Example: Mary Had a Little Lamb

```bash
./generate_melody_sustained.sh | SimpleSynthHost --duration 6.0 > mary.raw
sox -r 44100 -e float -b 32 -c 2 mary.raw mary.wav
```

## Audio Format

**Output Format**: Raw float32 PCM, interleaved stereo, native endianness
- Sample Rate: 44100 Hz
- Channels: 2 (stereo)
- Sample Size: 32-bit float
- **No headers, no metadata** - pure data stream

Compatible with:
- `sox` - audio converter
- `aplay` - ALSA player (Linux)
- Custom processing pipelines

## Architecture

### SimpleSynth Plugin

VST3 synthesizer with:
- Sine/square waveform selection
- ADSR envelope
- Frequency and gain control
- MIDI note input
- Real-time and offline rendering support

### SimpleSynthHost

Unified host with:
- **CommandLineOptions** - CLI argument parser
- **StdinMidiReader** - Read MIDI from stdin
- **StdoutAudioWriter** - Write PCM to stdout
- **OfflineRenderer** - Batch processing engine
- **UDPMIDIReceiver** - UDP MIDI server (interactive mode)

Auto-detects mode: stdin pipe = batch, terminal = interactive

## Design Philosophy

- **Minimal** - Do one thing well
- **Composable** - Read stdin, write stdout
- **Predictable** - No surprises, simple behavior
- **Testable** - Easy to integrate with test frameworks
- **Unix-like** - Inspired by shell utilities and pipes

## License

[Specify your license here]

## Notes

- Currently Windows-only (Winsock2 for UDP)
- MIDI timing: All events processed in order, at sample position 0 of their block
- Audio sustains across blocks after MIDI Note On (for batch processing)
- Debug output goes to stderr, audio to stdout

## Future Enhancements

- Linux/macOS support
- WAV file input/output
- MIDI file support
- Plugin state presets
- More waveform types
- Better timing control
