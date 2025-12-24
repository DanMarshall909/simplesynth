# SimpleSynth Learning Lessons

This folder documents learning progress as we develop SimpleSynth with Claude Code.

## Lesson Structure

```
lessons/
├── current-lesson.md          ← LIVE file, refreshed constantly
└── completed/
    ├── lesson-001-setup.md
    ├── lesson-002-audio-generation.md
    └── [more completed lessons...]
```

## How It Works

### During Active Development

**`current-lesson.md`** is open and continuously updated with:
- What we're doing RIGHT NOW
- Current task progress
- Code changes being made
- Tests being run
- Next immediate step

This is a **live document**, not a historical log.

### When Lesson Completes

When we finish a major lesson/feature:
1. Archive `current-lesson.md` → `completed/lesson-NNN-title.md`
2. Update the filename with lesson number and descriptive title
3. Start fresh `current-lesson.md` for the next lesson
4. Never goes back to edit completed lessons - they're frozen snapshots

## What Goes in Current Lesson

Keep it minimal and focused on RIGHT NOW:

```markdown
# Current Lesson: [Title]

**Date Started:** [Date]
**Status:** In Progress
**Objective:** [What we're trying to accomplish]

## What We're Doing Right Now

### Current Task
[Exact thing being worked on at this moment]

### What We're Learning
[Concepts, C++, JUCE, DSP]

### Progress So Far
[How far along we are]

## Code Changes Made This Lesson

### File: SimpleSynth/src/PluginProcessor.cpp
[What changed, why, code snippet]

### File: SimpleSynthHost/Source/Main.cpp
[What changed, why, code snippet]

## Tests & Verification

```bash
# How we verified it works
```

## Next Immediate Step
[What comes next]
```

**Key principle:** Short, current, actionable. No archaeological digs through history.

## Archive Structure

Completed lessons are named:
```
lesson-001-foundation-setup.md
lesson-002-audio-generation-and-testing.md
lesson-003-add-adsr-envelope.md
lesson-004-implement-polyphony.md
```

Each contains a **complete snapshot** of:
- What we started with
- What we learned
- Every code change
- Tests and results
- Final state

They're READ-ONLY after archival (for reference).

## Workflow Example

### Session 1
1. Open `current-lesson.md`
2. Fill in: "Adding ADSR envelope"
3. Work through implementation
4. Update progress as you go
5. When complete: Archive → `lessons/completed/lesson-001-adsr.md`

### Session 2
1. Open fresh `current-lesson.md`
2. Fill in: "Add polyphony support"
3. Reference `lesson-001-adsr.md` if needed
4. Update as you work
5. Archive when done

## Why This Structure

- **current-lesson.md** stays clean and focused (no noise, no old info)
- **completed/** acts as reference library
- Easy to understand progress (just look at lesson count)
- No need to scroll through history
- Clear separation: "what I'm doing now" vs "what I've done"

## Tips for Best Results

1. **Update frequently** - After each major step, refresh current-lesson.md
2. **Use code blocks** - Show the actual code that changed
3. **Include test commands** - Copy-paste the exact command you ran
4. **Note confusions** - What was confusing? How did you figure it out?
5. **Explain decisions** - Why did you do it that way?
6. **Keep it conversational** - Write like you're explaining to a friend

## Example Current Lesson

```markdown
# Current Lesson: Add Low-Pass Filter

**Date Started:** Dec 24, 2025
**Status:** Working on cutoff parameter
**Objective:** Add frequency-dependent filter to SimpleSynth

## What We're Doing Right Now

### Current Task
Implementing one-pole low-pass filter in processBlock().
The filter will use a simple recursive formula:
output = input * alpha + output_previous * (1 - alpha)

### What We're Learning
- How filters work (attenuation at high frequencies)
- One-pole vs multi-pole filters
- Why you need state (output_previous) for filters

### Progress So Far
- Created filter class
- Added cutoff parameter (20Hz - 20kHz range)
- Testing with swept sine wave

## Code Changes Made This Lesson

### File: SimpleSynth/src/PluginProcessor.h
Added filter member variable and state:
```cpp
float filterState = 0.0f;
float filterCoefficient = 0.1f;
```

### File: SimpleSynth/src/PluginProcessor.cpp
In processBlock():
```cpp
float cutoff = getParameter(Parameters::cutoff);
filterCoefficient = 2.0f * 3.14159f * cutoff / sampleRate;

for (int i = 0; i < numSamples; ++i) {
    float sample = generateSine(phase);
    // Apply low-pass filter
    filterState = sample * filterCoefficient + filterState * (1.0f - filterCoefficient);
    output[i] = filterState;
}
```

## Tests & Verification

```bash
# Generate sine sweep from 100Hz to 10kHz with low-pass at 1kHz
./test_sweep.sh | SimpleSynthHost --param Cutoff=1000 > filtered.raw

# Compare with unfiltered
./test_sweep.sh | SimpleSynthHost > unfiltered.raw

# Listen to both and verify high frequencies are attenuated
```

## Next Immediate Step
Test with melody to ensure filtering doesn't make it sound bad
```

---

**ProTip:** Keep current-lesson.md open in your editor. Hit save frequently. It's your active learning journal, not a finished document.
