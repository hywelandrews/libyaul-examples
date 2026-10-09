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

- The sound CPU (68EC000) is halted using the SMPC sound off command, from `user_init()`, before the pad polling handler is installed. The command register is not locked, so it must not interleave with the pad INTBACK.
- A 16384-sample looped sine waveform is loaded into sound RAM at byte offset `0x10000`.
- Slot 0 is programmed through the libyaul SCSP API: forward loop, 16-bit samples, fastest attack, no decay, shortest release, full level, centred, no effect send.
- The playback position bar reads the SCSP monitor slot (MSLC/CA), which reports progress in 4096-sample steps.

## Limits

- Tested in an emulator only, not on hardware.
- Halting the sound CPU relies on the SMPC sound off call (`smpc_smc_sndoff_call`), which libyaul marks "do not use".
