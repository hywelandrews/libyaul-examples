/*
 * Copyright (c) Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef TONE_H
#define TONE_H

#include <stdbool.h>
#include <stdint.h>

/* Octave range around the base pitch of about 440 Hz (octave 0) */
#define TONE_OCTAVE_MIN (-2)
#define TONE_OCTAVE_MAX (+2)

/* Master volume range (0 is off, 15 is maximum) */
#define TONE_VOLUME_MIN     (0)
#define TONE_VOLUME_MAX     (15)
#define TONE_VOLUME_DEFAULT (15)

/* Number of samples in the looped waveform: 128 cycles of 128 samples */
#define TONE_LOOP_SAMPLES (16384)

/* The monitored playback position advances in steps of this many samples, so
 * tone_position_get() returns 0 to 3 for the 16384 sample loop (it can read 4
 * for a moment at the loop boundary) */
#define TONE_POSITION_STEP_SAMPLES (4096)

/* Halt the sound CPU. Call once, from user_init(), before the VBlank-OUT
 * handler that polls the pad is installed.
 *
 * The SMPC command register is not locked, so the command must not run while
 * that handler can issue a pad INTBACK. */
extern void tone_sound_cpu_halt(void);

/* Prepare the sound system. Call once, after tone_sound_cpu_halt() and before
 * any other tone function.
 *
 * Keys off all 32 slots, sets the master volume to TONE_VOLUME_DEFAULT, loads
 * the waveform into sound RAM and programs slot 0 (left keyed off, octave 0). */
extern void tone_init(void);

/* Key slot 0 on or off. KEY_ON/KEY_OFF is applied to all slots, but only slot 0
 * is ever programmed. */
extern void tone_key_on(void);
extern void tone_key_off(void);
extern bool tone_is_on(void);

/* Set the octave. Values outside TONE_OCTAVE_MIN to TONE_OCTAVE_MAX are
 * clamped. May be called while the tone is playing. */
extern void tone_octave_set(int8_t octave);
extern int8_t tone_octave_get(void);

/* Playback frequency of the current octave, in tenths of a hertz.
 *
 * The frequency is 44100 Hz * 2^octave * (1024 + 284) / 1024 / 128, where 284
 * is the fixed fine tune value. In integers, with rounding:
 *
 *   (576828000 * 2^octave + 65536) / 131072
 *
 * 576828000 << 2 fits in 32 bits, so uint32_t is enough. Shift right for
 * negative octaves. Expected results:
 *
 *   octave -2: 1100   octave -1: 2200   octave 0: 4401
 *   octave +1: 8802   octave +2: 17603 */
extern uint32_t tone_frequency_decihz_get(void);

/* Set the master volume. Values outside TONE_VOLUME_MIN to TONE_VOLUME_MAX are
 * clamped. */
extern void tone_volume_set(int8_t volume);
extern uint8_t tone_volume_get(void);

/* Playback position of slot 0 in steps of TONE_POSITION_STEP_SAMPLES samples
 * from the start of the waveform. The value is read from the SCSP monitor
 * and is only meaningful while the tone is playing. */
extern uint8_t tone_position_get(void);

#endif /* !TONE_H */
