# SCSP Tone Example

This example demonstrates how to play a tone on the Sega Saturn SCSP sound processor using libyaul.

## Controls

- **A**: Toggle tone on / off
- **Up / Down**: Change octave
- **Left / Right**: Change master volume

## Building

Build the example using make:

```sh
make
```

## How It Works

- The sound CPU (68EC000) is halted by `scsp_init()` in `user_init()`, before the pad polling handler is installed, so the SMPC command cannot interleave with the pad INTBACK. `scsp_init()` also waits until the SCSP can be programmed safely.
- A 16384-sample looped sine waveform is loaded into sound RAM at byte offset `0x10000`.
- Slot 0 is programmed through the libyaul SCSP API: forward loop, 16-bit samples, fastest attack, no decay, shortest release, full level, centred, no effect send.
- The playback position bar reads the SCSP monitor slot (MSLC/CA), which reports progress in 4096-sample steps.

## Limits

- Tested in an emulator only, not on hardware.
- `scsp_init()` waits a fixed, conservative delay after halting the sound CPU before the SCSP is programmed. Measure the halt latency on hardware and shrink the delay if it is confirmed much shorter.
