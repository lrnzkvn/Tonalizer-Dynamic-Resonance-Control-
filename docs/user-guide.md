# Tonalizer User Guide

Tonalizer is a spectral audio plugin that removes atonal frequencies, corrects pitch, and reshapes the tonal character of your sound — all driven by musical scales. It works on any audio: vocals, guitars, synths, drums, full mixes.

---

## Getting Started

1. **Insert Tonalizer** on a track in your DAW as a VST3 or LV2 plugin (or run it standalone).
2. **Choose a key** from the Key dropdown (C, C#, D, etc.).
3. **Choose a scale** from the Scale dropdown (Major, Minor, Blues, etc.).
4. **Turn up the Erase knob** to start removing frequencies that don't belong to your chosen scale.

That's the basic idea. Everything else builds on top of this.

---

## The Display

The dot-matrix display at the top shows a real-time spectral view of your audio across 32 frequency bands (low on the left, high on the right). Bars fill from the bottom up based on activity in each band.

- **Bright dots** are in-scale frequencies.
- **Dim dots** are atonal frequencies (the ones being affected by the eraser).
- The border colour shifts from cyan to green as you move the crossfader from Erase toward Tune.
- When **Freeze** is active, a pulsing orange border and a "FROZEN" badge appear.
- When the crossfader is moved toward Tune, a **pitch readout** appears at the bottom showing the detected note, how many cents sharp or flat it is, and the target note.

---

## Controls Reference

### Header

| Control | What it does |
|---------|-------------|
| **Clean / Creative** | Switches between two processing modes. Clean mode attenuates atonal frequencies. Creative mode boosts them instead, for lo-fi, textural, or experimental effects. |
| **Bypass** | Passes audio through unprocessed. |

### Crossfader (ERASE ↔ TUNE)

The horizontal slider blends between two processing paths:

- **ERASE (left, 0%):** Spectral erasing only — atonal frequencies are suppressed (or boosted in Creative mode).
- **TUNE (right, 100%):** Pitch correction only — notes are pulled toward the nearest scale tone.
- **In between:** A spectral blend of both effects. This is a complex-domain crossfade, not a simple volume mix, so intermediate positions can produce richer interference patterns and interesting harmonic combinations.

### Row 1: Scale & Erase Controls

| Control | Range | What it does |
|---------|-------|-------------|
| **Key** | C through B | Sets the root note of the scale. |
| **Scale** | 42 scales | Sets the scale type (see Scale List below). |
| **Erase** | 0–100% | How aggressively atonal frequencies are reduced (Clean mode) or boosted (Creative mode). At 0% the eraser has no effect. |
| **Sharp** | 0–100% | Controls the precision of the eraser. Low values give a gentle, broad suppression. High values create a sharper cut — frequencies are either in-scale or they're not. |
| **Mix** | 0–100% | Dry/wet control. 0% is fully dry (unprocessed), 100% is fully wet. |

### Row 2: Tune & Formant Controls

| Control | Range | What it does |
|---------|-------|-------------|
| **Retune** | 0–400 ms | How quickly pitch correction reaches the target note. 0 ms is instant (hard-tune / robotic effect). Higher values let notes slide naturally into place. |
| **Correct** | 0–200% | Strength of pitch correction. 100% pulls notes exactly to the scale. Below 100% is gentler (notes get nudged but not fully corrected). Above 100% overshoots for exaggerated pitch effects. |
| **Formant** | -12 to +12 st | Shifts the timbral character without changing pitch. Positive values make the sound brighter and thinner. Negative values make it darker and fuller. At 0, no formant shift is applied. |
| **Boost** | 0–200% | (Creative mode only) Controls how much atonal frequencies are boosted. Only visible when Creative mode is selected. |

### Row 3: Freeze & Morph Controls

| Control | Range | What it does |
|---------|-------|-------------|
| **FREEZE** | On/Off | Captures a spectral snapshot of whatever is playing when you press it. The button glows orange when active. |
| **Morph** | 0–100% | Blends between the live signal (0%) and the frozen snapshot (100%). Only has an effect when Freeze is on. |
| **X-Synth** | 0–100% | Cross-synthesis amount. Applies the sidechain signal's tonal shape to your main signal. Requires a sidechain input to be routed in your DAW. Has no effect without a sidechain connected. |

### Scene Row (A/B Morph)

| Control | What it does |
|---------|-------------|
| **A button** | Saves a snapshot of all current settings as Scene A. Lights up purple when a scene is stored. |
| **B button** | Saves a snapshot of all current settings as Scene B. Lights up purple when a scene is stored. |
| **Scene slider** | Blends between Scene A (left) and Scene B (right). Continuous parameters interpolate smoothly. Discrete settings (key, scale, mode) snap at the midpoint. |

---

## Features In Depth

### Spectral Erasing

Tonalizer analyses your audio in real time, breaking it into 1025 frequency bins. Each bin is scored against your chosen key and scale — frequencies that land on scale tones are scored as "tonal," and those that fall between scale tones are scored as "atonal."

The **Erase** knob controls how much the atonal frequencies are turned down. The **Sharp** knob controls the transition: at low sharpness, there's a gradual rolloff between tonal and atonal bins. At high sharpness, the line is drawn more strictly.

**Clean mode** attenuates atonal frequencies (useful for cleaning up recordings, isolating tonal content, or tightening a mix).

**Creative mode** does the opposite — it boosts atonal frequencies for lo-fi, gritty, or otherworldly textures. The **Boost** knob controls how much.

### Pitch Correction (Autotune)

When the crossfader is moved toward TUNE, Tonalizer detects the pitch of incoming audio using the YIN algorithm and shifts it toward the nearest note in your chosen scale.

- **Retune Speed** controls how fast the correction happens. Fast settings (0–10 ms) give the classic hard-tune/T-Pain effect. Slower settings (50–200 ms) are more transparent and natural.
- **Correction Amount** at 100% pulls the note all the way to the target. Lower values give partial correction (notes are nudged but retain some of their original pitch). Values above 100% overshoot the target, which can create unusual pitch effects.
- The pitch readout on the display shows what note was detected, how far off it was, and what note it's being pulled toward.

Tonalizer processes stereo signals with coherent pitch detection — channel 1 (left) detects the pitch, and channel 2 (right) uses the same correction ratio, so the stereo image stays stable.

### Spectral Crossfade

The ERASE ↔ TUNE crossfader doesn't just mix volumes. It blends the erase and autotune paths in the complex frequency domain (both magnitude and phase), which means intermediate positions produce constructive and destructive interference between the two spectral representations. This gives you tonal combinations that wouldn't be possible with a simple A/B volume blend.

### Freeze + Morph

Pressing FREEZE captures the current spectrum — a snapshot of the frequency content at that instant. The Morph knob then controls how much of that frozen snapshot replaces the live signal.

Practical uses:
- **Infinite sustain:** Freeze a chord, set Morph to 100%, and the tone sustains forever regardless of input.
- **Textural blending:** Freeze a sound, set Morph to 30–50%, and the frozen spectrum adds a ghostly layer behind whatever you play next.
- **Sound design:** Freeze a transient (a drum hit, a vocal consonant) and use it as a tonal bed.

The frozen snapshot is taken from the fully processed signal (after erasing, autotune, and crossfading), so whatever settings you have active at the moment of capture are baked into the freeze.

### Formant Shifting

Formants are the resonant peaks that define the character of a sound — they're why a violin and a human voice singing the same note sound completely different, and why the vowel "ah" sounds different from "ee."

The Formant knob shifts these resonant peaks up or down without changing the pitch:

- **+6 semitones:** The sound takes on a brighter, smaller, more nasal quality. Vocals sound like a smaller person.
- **-6 semitones:** Darker, larger, more chesty. Vocals get deeper in character.
- This works independently of pitch correction, so you can autotune someone's pitch while completely reshaping their timbre.

Under the hood, Tonalizer uses cepstral analysis to separate the spectral envelope (formant shape) from the fine spectral detail (pitch and harmonics), shifts the envelope, and recombines them.

### Cross-Synthesis (X-Synth)

Cross-synthesis takes the spectral envelope (overall tonal shape) of a sidechain signal and applies it to your main signal. The main signal keeps its pitch and fine detail, but its broad tonal character is replaced by the sidechain's.

To use it:
1. Route a second audio signal to Tonalizer's sidechain input in your DAW. (The method varies by DAW — check your DAW's documentation for sidechain routing.)
2. Turn up the X-Synth knob.

Examples:
- **Main = vocal, Sidechain = synth pad:** The vocal sounds like it's being filtered through the synth's harmonic structure.
- **Main = guitar, Sidechain = drum loop:** The guitar's tone pulses and shapes with the rhythm of the drums.
- **Main = anything, Sidechain = vocal:** Imprint speech-like formant motion onto any sound.

If no sidechain is connected, the X-Synth knob does nothing.

### A/B Scene Morph

Scene morph lets you save two complete snapshots of Tonalizer's settings and smoothly interpolate between them.

1. Dial in your first sound. Press **A** to save it.
2. Change your settings to something different. Press **B** to save it.
3. Move the **scene slider** to blend between the two.

All continuous parameters (Erase, Sharp, Mix, Retune, Correct, Formant, Crossfade, Morph, X-Synth, Boost) interpolate smoothly. Discrete parameters (Key, Scale, Mode) snap to one value or the other at the 50% mark.

The scene slider is an automatable parameter, so you can draw automation curves in your DAW to morph between two settings over time — useful for transitions, build-ups, or evolving textures across a song.

Scene data is saved with your project and persists across sessions.

---

## Scale List

Tonalizer includes 42 scales organised into nine categories:

**Western Modes:** Major, Dorian, Phrygian, Lydian, Mixolydian, Minor, Locrian

**Minor Variants:** Harmonic Minor, Melodic Minor, Harmonic Major, Hungarian Minor, Neapolitan Minor, Neapolitan Major

**Pentatonic & Blues:** Major Pentatonic, Minor Pentatonic, Blues, Blues Major

**Jazz:** Bebop Dominant, Bebop Major, Altered, Lydian Dominant, Diminished (Half-Whole), Diminished (Whole-Half), Augmented

**Middle Eastern:** Double Harmonic, Phrygian Dominant, Hijaz, Persian

**East Asian:** Hirajoshi, In Sen, Iwato, Kumoi, Balinese Pelog

**Indian:** Todi, Marwa, Purvi

**African:** Egyptian

**Experimental:** Whole Tone, Prometheus, Tritone, Enigmatic

**Special:** Chromatic (all 12 notes — effectively disables scale-based processing)

---

## Tips

- **Start with Major or Minor Pentatonic** if you're not sure what scale to use. Pentatonic scales have fewer notes, so the eraser and autotune are more aggressive and the effect is more obvious.
- **Chromatic scale** effectively turns off scale-based processing since every note is "in scale." Useful if you only want the formant shift or cross-synthesis features.
- **Retune Speed at 0 ms + Correction at 100%** gives the hardest autotune effect. Back off Correction to 60–80% for something more subtle.
- **Creative mode + high Erase** produces harsh, glitchy, broken textures — combine with Freeze for evolving drone-like pads.
- **Formant shift stacks with autotune.** Try pitch-correcting a vocal while shifting formants in the opposite direction of the pitch correction for an uncanny, synthetic quality.
- **All new parameters are fully automatable** in your DAW — Freeze, Morph, Formant, X-Synth, and Scene Morph all respond to automation lanes.

---

## Technical Details

- **Latency:** 2048 samples (about 46 ms at 44.1 kHz). This is fixed and does not change with any settings.
- **FFT Size:** 2048 samples with 75% overlap (512-sample hop size), giving 1025 frequency bins.
- **Processing:** All spectral operations (erasing, autotune, crossfade, freeze, formant shift, cross-synthesis) happen in a single unified FFT pipeline, so there's only one analysis pass per channel regardless of how many features you use.
- **Stereo handling:** Each channel is processed independently, but pitch detection runs on the left channel only and the correction ratio is shared to the right channel, keeping the stereo image coherent.
- **Formats:** VST3, LV2, Standalone.
