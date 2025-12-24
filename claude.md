# Claude Code Guide for SimpleSynth

This document describes how to use Claude Code to develop and extend SimpleSynth.

## Overview

SimpleSynth is a minimal, composable JUCE audio plugin test harness designed to be developed and extended using Claude Code. This guide covers:

1. Development workflow with Claude Code
2. Common tasks and how to accomplish them
3. Code structure and architecture
4. Building and testing
5. Extending the plugin with new features

## Quick Start

### Building the Project

```bash
cd /c/code/simplesynth-repo
cmake -B build -S .
cmake --build build
```

### Running Tests

```bash
# Simple note generation test
echo -ne '\x90\x3C\x64' | ./SimpleSynthHost/cmake-build/Debug/SimpleSynthHost.exe --duration 2.0 > test.raw

# Generate melody
./generate_melody_sustained.sh | ./SimpleSynthHost/cmake-build/Debug/SimpleSynthHost.exe --duration 6.0 > mary.wav
```

### Using with Claude Code

```bash
# Read the codebase structure
claude-code read SimpleSynth/src/PluginProcessor.cpp

# Ask Claude to implement a feature
claude-code ask "Add a cutoff frequency parameter to the plugin"

# Run tests after modifications
cmake --build build && echo -ne '\x90\x3C\x64' | SimpleSynthHost
```

## Note: Relearning C++ After 30 Years

If you're coming back to C++ after a long break, this is a great project to work with Claude Code. Here's what you should know:

### What's Changed Since the 90s

**Modern C++ Features You'll See:**
- **Smart pointers** (`std::unique_ptr`, `std::shared_ptr`) - Memory management is automatic now, no more manual `delete`
- **Auto type deduction** (`auto keyword`) - Compiler figures out types
- **Range-based for loops** - `for (auto& item : container)` instead of iterators
- **Lambda functions** - Inline anonymous functions with `[captures](args) { code }`
- **RAII** - Resource Acquisition Is Initialization (memory/file cleanup is automatic)
- **Constexpr** - Compile-time computation
- **Move semantics** - Efficient resource transfer instead of copying

### How to Ask Claude for Help

**Be specific about what you need explained:**

```
"I see this code uses std::unique_ptr - I remember pointers from
the 90s but don't understand unique_ptr. Can you explain how it
prevents memory leaks?"
```

```
"This code has 'auto' everywhere. How do I know what type a variable is?
Do I need to add type declarations?"
```

```
"Can you show me the modern C++ way to do this instead of the old pattern?
I'm used to manual memory management."
```

**Claude will help with:**
- Explaining modern C++ syntax you haven't seen before
- Suggesting idiomatic (modern) approaches instead of old patterns
- Pointing out memory safety improvements
- Walking through code and breaking down unfamiliar constructs

### JUCE Framework Notes

JUCE is a mature C++ framework that uses:
- Lots of inheritance (OOP style)
- Custom memory management with `OwnedArray`, `ScopedPointer`
- Heavy use of virtual functions
- Windows API integration (for this project)

Don't worry if it seems complex at first - Claude can explain any JUCE concept.

### Recommended Approach

1. **Focus on the plugin logic first** - Start with `PluginProcessor.cpp`, which is the audio generation code
2. **Don't worry about framework details** - Just ask "What does this JUCE method do?"
3. **Build often** - Test after each small change to catch errors early
4. **Ask Claude about patterns** - "How would I modernize this to use smart pointers?"
5. **Use debug output** - Don't rely on a debugger at first, just print values to stderr

### C++ Gotchas to Watch For

- **References vs pointers** - `&` creates a reference (like a safer pointer)
- **const correctness** - Methods marked `const` promise not to change state
- **Headers and implementations** - `.h` files declare, `.cpp` files define (same as before)
- **Namespaces** - `std::` prefix means it's from the standard library
- **Templates** - Generic code that works with any type (see `AudioBuffer<float>`)

### Quick Reference

| Modern C++ | Old C++ | Use Case |
|-----------|---------|----------|
| `auto x = value;` | `Type x = value;` | Let compiler deduce type |
| `std::unique_ptr<T>` | `T* = new T;` | Single ownership, auto delete |
| `for (auto x : array)` | `for (int i = 0; i < n; ++i)` | Iterate through containers |
| `[x](){ code }` | Function pointers | Inline callbacks |
| `std::vector<T>` | `T array[SIZE]` or new array | Dynamic arrays |

### When in Doubt

Ask Claude directly:
```
"I don't recognize this syntax: [&](float x) { ... }
What is this doing? Is it a C++ thing or JUCE thing?"
```

Claude will explain and can rewrite code in a simpler style if needed.

### Projects Like This Are Ideal for Relearning

SimpleSynth is great because:
- It's real code (not a tutorial)
- It has both simple (parameter handling) and complex (audio DSP) parts
- You get immediate feedback (audio output!)
- JUCE documentation is excellent
- The codebase is small enough to understand completely

Don't hesitate to ask Claude to explain anything. Relearning a language after 30 years is common, and modern C++ is genuinely different enough to warrant explanation.

### Claude Should Point Things Out As You Go

When working together with Claude Code, you should **explicitly tell Claude to educate you as you work**. Here's how:

**At the start of a session:**
```
"I'm relearning C++ after 30 years. As we work, please:
- Point out modern C++ features I might not recognize
- Explain what code is doing, especially framework stuff
- Suggest if there's a more idiomatic (modern) way
- Warn me about potential bugs or gotchas
- Explain why a certain approach is better
Don't assume I know what everything means."
```

**Claude should proactively:**

1. **Explain unfamiliar syntax**
   ```
   "This uses a lambda function - that's [captures](args) { code }
   The [&] means 'capture all variables by reference'"
   ```

2. **Point out modern patterns**
   ```
   "This uses std::unique_ptr for automatic memory cleanup.
   In the 90s you'd use new/delete manually and risk leaks.
   unique_ptr prevents that automatically."
   ```

3. **Walk through framework concepts**
   ```
   "JUCE's AudioProcessor is the base class. The processBlock()
   method is called repeatedly with audio data. We override it
   to generate sound."
   ```

4. **Warn about gotchas**
   ```
   "Watch out: This method is marked const, so it can't modify
   member variables. If you try to change a parameter value here,
   it won't compile."
   ```

5. **Suggest alternatives**
   ```
   "You could also write this as a range-based for loop:
   for (auto& channel : buffer) { ... }
   It's more readable than traditional indexing."
   ```

6. **Explain compiler errors**
   ```
   "This error means the compiler can't figure out which version
   of the function you meant. That's because we have multiple
   functions with the same name (overloading)."
   ```

**Example: Good Claude Explanation**

When you ask to add a parameter, Claude should:
1. Explain what it's doing
2. Point out any modern C++ features
3. Walk through the JUCE framework parts
4. Explain the flow (parameter → GUI → audio processing)
5. Test the change

**Bad:** Just show code without context
**Good:**
```
I'm adding a 'Gain' parameter. Here's how it works:

1. **Enum** - This defines parameter IDs (0, 1, 2...)
   Modern C++ note: We could use std::string_view instead
   of comparing string names, but JUCE uses the string approach.

2. **getParameterName()** - JUCE calls this to display
   parameter names. It's marked const because we're not
   changing any state.

3. **processBlock()** - This is called repeatedly with audio data.
   We read the gain parameter and apply it to each sample.
   Note: getParameter() returns 0.0-1.0, so we multiply by 2.0
   to get a range of 0.0-2.0x gain.

4. **The math** - sample *= gain * 2.0 makes the audio louder.
   If gain goes above 1.0, watch for clipping (distortion).
```

**Tell Claude explicitly what you want:**

```
"I'm going to ask you to implement features, but I want you
to explain as you go. For each code change:
- Show the code
- Explain what it does in simple terms
- Point out C++ features I might not recognize
- Explain JUCE concepts
- Warn about potential issues"
```

This turns coding into a learning experience, not just changing code.

## Design Patterns: Learning as You Code

As we develop SimpleSynth, Claude should teach you appropriate **design patterns** - reusable solutions to common programming problems. This is especially important when relearning C++ after 30 years, as modern patterns are quite different.

### What Patterns We'll See

**Structural Patterns** (How objects are organized):
- **RAII** (Resource Acquisition Is Initialization) - Resources cleaned up automatically
- **Dependency Injection** - Pass dependencies to constructors instead of hardcoding them
- **Observer** - JUCE uses this heavily (objects notify listeners of changes)

**Behavioral Patterns** (How objects interact):
- **State Machine** - For things like ADSR envelopes (Attack → Decay → Sustain → Release)
- **Factory** - Creating objects of different types (e.g., different waveform generators)
- **Strategy** - Swapping algorithms at runtime (e.g., different oscillator types)

**Concurrency Patterns** (For real-time audio):
- **Lock-free queues** - MIDI messages without mutex locks
- **Double buffering** - For safe parameter changes without glitches

### How Claude Should Teach Patterns

When you're writing code, Claude should:

1. **Identify the pattern being used**
   ```
   "This code is implementing the Observer pattern - when a parameter
   changes, JUCE notifies the UI and the processor separately."
   ```

2. **Explain why it's better than alternatives**
   ```
   "We're using RAII (std::unique_ptr) instead of manual delete.
   The old way (C++98) would look like:
   ```cpp
   AudioPluginInstance* plugin = new AudioPluginInstance();
   // ... later, you might forget to delete ...
   delete plugin;  // Easy to forget!
   ```

   Modern way:
   ```cpp
   auto plugin = std::make_unique<AudioPluginInstance>();
   // Automatically deleted when scope ends - can't forget!
   ```
   "

3. **Show how it applies to SimpleSynth**
   ```
   "The State Machine pattern would be perfect for ADSR envelopes.
   Each phase (Attack, Decay, Sustain, Release) is a state:
   - Attack: increase amplitude over time
   - Decay: decrease to sustain level
   - Sustain: hold at sustain level
   - Release: fade to zero when note ends
   ```

4. **Suggest when to apply it**
   ```
   "If we add polyphony, we should use a Factory pattern to create
   Voice objects. Instead of hardcoding voice creation, we'd pass
   a VoiceFactory. This makes it easy to change how voices work."
   ```

### Patterns We'll Likely Implement

**Soon:**
- RAII with unique_ptr ✓ (already using)
- Observer pattern (JUCE parameters) ✓ (already in framework)
- Resource management (file I/O, memory) ✓

**Medium term:**
- State Machine (ADSR envelope)
- Factory pattern (voice allocation in polyphony)
- Strategy pattern (waveform selection)

**Advanced:**
- Lock-free queues (MIDI input)
- Double buffering (parameter smoothing)

### How to Ask Claude About Patterns

```
"I want to add an ADSR envelope. What pattern should I use?"
```

Claude should respond with:
1. Name the pattern (State Machine)
2. Why it fits (Four distinct phases)
3. How to implement (Simple state variable + time counter)
4. Show the code structure
5. Explain trade-offs vs simpler approaches

### Bad vs Good Pattern Explanations

**Bad:**
```cpp
// Add ADSR
float adsr = 0.0f;  // Just have Claude implement it
```

**Good:**
```
"You'll use a State Machine pattern for ADSR:

enum ADSRState { Attack, Decay, Sustain, Release };

This tracks which phase the note is in. As time passes,
we transition between states. For example:

- In Attack phase: envelope_value increases from 0 to 1
- In Decay phase: envelope_value decreases from 1 to sustain_level
- In Sustain phase: envelope_value stays constant
- In Release phase: envelope_value decreases from current to 0

This is better than having a single 'envelope_time' because:
1. Each phase has different rates
2. Sustain doesn't progress over time (always the same)
3. Release happens only after note-off
4. Clear intent: the state variable documents what's happening

Here's the implementation..."
```

### Patterns vs Over-Engineering

**Important:** Don't over-engineer. Use patterns when they solve real problems:

✓ **USE pattern:** Adding polyphony needs voice allocation → Factory
✓ **USE pattern:** ADSR needs phases with different behavior → State Machine
✗ **SKIP pattern:** Simple parameter setter doesn't need Factory
✗ **SKIP pattern:** Three waveforms don't need Strategy yet

Claude should help you recognize when a pattern is worth using and when simpler code is better.

### Learning Through Implementation

The best way to learn patterns is to implement them:

1. **See the problem** - "How do I manage multiple voices?"
2. **Learn the pattern** - "Factory pattern creates objects"
3. **Implement it** - Write the code with Claude's guidance
4. **Refactor later** - If the pattern doesn't fit, change it
5. **Remember it** - For the next project

## Architecture

### SimpleSynth Plugin (`SimpleSynth/src/`)

**PluginProcessor.h/cpp** - Core audio processing
- JUCE AudioProcessor subclass
- MIDI note handling
- Audio generation (sine wave oscillator)
- Parameter management
- Real-time and offline rendering

**PluginEditor.h/cpp** - GUI (minimal stub)
- Currently a placeholder
- Can be extended with visual controls

### SimpleSynthHost (`SimpleSynthHost/Source/Main.cpp`)

The host application is a single-file implementation with several key classes:

**CommandLineOptions**
- Parses command-line arguments
- Auto-detects batch vs interactive mode
- Stores plugin parameters

**StdinMidiReader**
- Reads raw MIDI bytes from stdin
- Supports Note On, Note Off, CC messages
- Binary mode on Windows

**StdoutAudioWriter**
- Writes float32 PCM to stdout
- Interleaved stereo format
- Binary mode on Windows

**OfflineRenderer**
- Batch processing engine
- Loads SimpleSynth plugin
- Processes MIDI and generates audio
- Writes to stdout

**UDPMIDIReceiver**
- UDP server for interactive mode
- Listens on port 9999
- Routes MIDI to plugin

## Development Tasks

### Adding a New Plugin Parameter

1. **Add to PluginProcessor.h:**
```cpp
enum Parameters
{
    frequency = 0,
    gain = 1,
    newParam = 2,  // Add here
    numParameters
};
```

2. **Implement in getParameterName():**
```cpp
case newParam:
    return "NewParam";
```

3. **Handle in processBlock():**
```cpp
float newValue = getParameter(newParam);
// Use newValue in audio generation
```

4. **Test:**
```bash
echo -ne '\x90\x3C\x64' | SimpleSynthHost --param NewParam=0.5 --duration 1.0 > test.raw
```

### Adding a New Waveform

1. **Modify PluginProcessor.cpp in processBlock():**
```cpp
float sample;
int waveform = (int)(getParameter(Parameters::waveform) * 2);

switch(waveform)
{
    case 0: sample = sine(phase); break;
    case 1: sample = square(phase); break;
    case 2: sample = triangle(phase); break;  // New
    default: sample = 0.0f;
}
```

2. **Implement waveform function:**
```cpp
float triangle(float phase)
{
    // phase is 0 to 1
    if(phase < 0.5f)
        return -1.0f + 4.0f * phase;
    else
        return 3.0f - 4.0f * phase;
}
```

3. **Test with the melody generator:**
```bash
./generate_melody_sustained.sh | SimpleSynthHost --param Waveform=2 --duration 6.0 > triangle.raw
```

### Extending the Test Harness

**Add a new command-line option in CommandLineOptions::parse():**
```cpp
if (args.containsOption("--reverb"))
    opts.reverbAmount = args.getValueForOption("--reverb").getFloatValue();
```

**Use in OfflineRenderer:**
```cpp
if(options.reverbAmount > 0.0f)
{
    // Apply reverb effect
    applyReverb(outputBuffer, options.reverbAmount);
}
```

### Adding Interactive Features

1. **Extend UDPMIDIReceiver** - Already listens on port 9999
2. **Send MIDI via Python:**
```python
import socket
midi_bytes = bytes([0x90, 0x3C, 0x64])  # Note On C4
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.sendto(midi_bytes, ('127.0.0.1', 9999))
```

## File Structure

```
simplesynth-repo/
├── SimpleSynth/
│   ├── CMakeLists.txt              # Build configuration
│   └── src/
│       ├── PluginProcessor.h/cpp   # Audio engine
│       └── PluginEditor.h/cpp      # GUI stub
│
├── SimpleSynthHost/
│   ├── CMakeLists.txt              # Build configuration
│   └── Source/
│       └── Main.cpp                # Host app + batch mode
│
├── generate_melody_sustained.sh    # Test melody generator
├── test_harness.py                 # Python test example
├── README.md                       # User documentation
├── .gitattributes                  # CRLF line endings
├── .gitignore                      # Exclude build artifacts
└── claude.md                       # This file
```

## Common Claude Code Commands

### Exploring the Codebase

```bash
# Find parameter handling
claude-code grep "getParameter"

# Understand MIDI handling
claude-code read SimpleSynthHost/Source/Main.cpp -n 72-117

# Check audio generation logic
claude-code grep "processBlock" SimpleSynth/src/
```

### Code Modifications

```bash
# Modify a specific function
claude-code edit SimpleSynth/src/PluginProcessor.cpp

# Add a new feature
claude-code ask "Add a low-pass filter with cutoff parameter"

# Fix a bug
claude-code ask "Debug why MIDI Note Offs aren't working"
```

### Testing and Validation

```bash
# Test after changes
cmake --build build && \
echo -ne '\x90\x3C\x64' | ./SimpleSynthHost/cmake-build/Debug/SimpleSynthHost.exe

# Test melody generation
./generate_melody_sustained.sh | ./SimpleSynthHost/cmake-build/Debug/SimpleSynthHost.exe --duration 2.0 > test.wav

# Convert to WAV for listening
sox -r 44100 -e float -b 32 -c 2 test.raw test.wav
```

## Performance Optimization Tips

1. **Vectorize audio processing** - Use JUCE's AudioBuffer methods
2. **Cache parameter values** - Don't call getParameter() every sample
3. **Minimize allocations** - Pre-allocate buffers in prepareToPlay()
4. **Profile with debug output** - Use fprintf to stderr for timing

## Integration with Test Frameworks

### Python Integration

```python
import subprocess
import struct

def test_synth_output():
    midi = bytes([0x90, 0x3C, 0x64])  # Note On C4
    proc = subprocess.Popen(
        ['SimpleSynthHost', '--duration', '1'],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL
    )
    audio_bytes, _ = proc.communicate(input=midi)

    # Convert bytes to float32 samples
    samples = struct.unpack(f'{len(audio_bytes)//4}f', audio_bytes)

    # Check for non-zero audio
    assert max(abs(s) for s in samples) > 0.1
    print("✓ Synth generates audio")

if __name__ == '__main__':
    test_synth_output()
```

### C++ Unit Tests

```cpp
#include <cassert>
#include <cmath>

void test_frequency_parameter()
{
    PluginProcessor synth;
    synth.prepareToPlay(44100, 512);

    // Set frequency parameter
    synth.setParameter(Parameters::frequency, 0.5f);  // Mid-range

    // Generate audio
    AudioBuffer<float> buffer(2, 512);
    MidiBuffer midi;
    midi.addEvent(MidiMessage::noteOn(1, 60, 100), 0);

    synth.processBlock(buffer, midi);

    // Verify non-zero output
    float maxSample = buffer.getMagnitude(0, 0, 512);
    assert(maxSample > 0.01f);
}
```

## Debugging

### Enable Debug Output

All debug output goes to stderr. Run with:

```bash
echo -ne '\x90\x3C\x64' | SimpleSynthHost --duration 1.0 > audio.raw 2> debug.log
cat debug.log
```

The OfflineRenderer writes comprehensive debug info:
- Plugin I/O configuration
- MIDI events received
- Audio samples generated
- Block processing progress

### Common Issues

**No audio output:**
- Check `acceptsMidi()` returns true (need NEEDS_MIDI_INPUT in CMakeLists.txt)
- Verify MIDI events are being read correctly
- Check plugin parameters aren't all at minimum

**Plugin not loading:**
- Verify VST3 path in Main.cpp matches build output
- Check JUCE_FORCE_USE_LEGACY_PARAM_IDS is set in CMakeLists.txt
- Ensure VST3 submodule path is correct

**Audio artifacts/distortion:**
- Check output amplitude isn't exceeding ±1.0
- Verify ADSR envelope values
- Look for DC offset issues

## Extension Ideas

1. **Add more waveforms** - Triangle, sawtooth, PWM
2. **Implement ADSR envelope** - Attack, decay, sustain, release
3. **Add effects** - Reverb, delay, chorus
4. **Parameter smoothing** - Prevent clicks/pops
5. **Polyphony** - Support multiple simultaneous notes
6. **MIDI CC mapping** - Modulation wheel, pedal support
7. **Preset system** - Save/load plugin state
8. **Real-time visualization** - Scope, spectrum analyzer

## Resources

- [JUCE Documentation](https://docs.juce.com/)
- [VST3 Specification](https://www.steinberg.net/vst3/)
- [Audio Processing Fundamentals](https://www.dsprelated.com/)

## Getting Help

When working with Claude Code:

1. **Describe the goal clearly** - What feature do you want to add?
2. **Show error messages** - Copy exact compiler/runtime errors
3. **Reference specific files** - Use file paths like `SimpleSynth/src/PluginProcessor.cpp`
4. **Test incrementally** - Build and test after each change
5. **Use debug output** - Check stderr for detailed debugging info

## Tips for Effective Collaboration with Claude

### Good Prompt Examples

```
"Add a cutoff frequency parameter to SimpleSynth.
It should range from 20Hz to 20kHz and use a
one-pole low-pass filter. Test with the melody generator."
```

```
"The MIDI Note Offs aren't working in batch mode.
The debug log shows they're being read but the plugin
keeps generating audio. How should I fix the sustain logic?"
```

### Iterative Development Pattern

1. Ask Claude to implement a feature
2. Build and test
3. Report any issues with specific error messages
4. Ask for refinements or bug fixes
5. Test again
6. Commit to git when working

### Checking Your Work

Always verify after changes:
```bash
# Rebuild
cmake --build build

# Quick test
echo -ne '\x90\x3C\x64' | SimpleSynthHost --duration 1.0 > test.raw

# Check file size (should be 352KB for 1 second stereo)
ls -lh test.raw
```

---

Happy coding! SimpleSynth is a solid foundation for audio plugin experimentation with Claude Code.
