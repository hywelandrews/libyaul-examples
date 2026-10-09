/*
 * Copyright (c) Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <assert.h>

#include "tone.h"

#define TONE_SLOT 0

/* Sound RAM as the SH-2 sees it (cache-through), and the byte offset of the
 * waveform inside it */
#define TONE_SOUND_RAM_BASE  (0x25A00000UL)
#define TONE_WAVEFORM_OFFSET (0x10000UL)

static bool _halted;
static bool _on;
static int8_t _octave;
static uint8_t _volume;

/* Generated with round(16000 * sin(2 * pi * n / 128)) for n = 0 to 127. */
static const int16_t _sine_table[128] = {
         0,    785,   1568,   2348,   3121,   3888,   4645,   5390,
      6123,   6841,   7542,   8226,   8889,   9531,  10150,  10745,
     11314,  11855,  12368,  12851,  13304,  13724,  14111,  14464,
     14782,  15065,  15311,  15521,  15693,  15827,  15923,  15981,
     16000,  15981,  15923,  15827,  15693,  15521,  15311,  15065,
     14782,  14464,  14111,  13724,  13304,  12851,  12368,  11855,
     11314,  10745,  10150,   9531,   8889,   8226,   7542,   6841,
      6123,   5390,   4645,   3888,   3121,   2348,   1568,    785,
         0,   -785,  -1568,  -2348,  -3121,  -3888,  -4645,  -5390,
     -6123,  -6841,  -7542,  -8226,  -8889,  -9531, -10150, -10745,
    -11314, -11855, -12368, -12851, -13304, -13724, -14111, -14464,
    -14782, -15065, -15311, -15521, -15693, -15827, -15923, -15981,
    -16000, -15981, -15923, -15827, -15693, -15521, -15311, -15065,
    -14782, -14464, -14111, -13724, -13304, -12851, -12368, -11855,
    -11314, -10745, -10150,  -9531,  -8889,  -8226,  -7542,  -6841,
     -6123,  -5390,  -4645,  -3888,  -3121,  -2348,  -1568,   -785
};

void
tone_sound_cpu_halt(void)
{
        /* libyaul marks this call "do not use"; it is used here on purpose.
         * If the sound CPU is left running, its program rewrites slot registers
         * (slot 0 pitch changed within a second) and sound RAM; with SOUND OFF
         * issued first the registers keep the programmed values. Observed in
         * an emulator only, not on hardware.
         *
         * The SMPC command register is not locked: if the pad INTBACK issued
         * from the VBlank-OUT handler interleaves with this command, the
         * handler can wait forever. Call this before that handler exists. */
        smpc_smc_sndoff_call();

        _halted = true;
}

void
tone_init(void)
{
        assert(_halted);

        /* Key off every slot first, so nothing the halted sound CPU left
         * playing is amplified by the master volume below */
        for (uint8_t slot = 0; slot < SCSP_SLOT_COUNT; slot++) {
                scsp_slot_word_set(slot, SCSP_SLOT_WORD_KYONEX, 0);
        }

        scsp_slot_kyonex_execute();

        _volume = TONE_VOLUME_DEFAULT;
        scsp_mvol_set(scsp_mvol_pack(true, false, _volume));

        volatile uint16_t * const sound_ram = (volatile uint16_t *)
            (TONE_SOUND_RAM_BASE + TONE_WAVEFORM_OFFSET);

        /* TONE_LOOP_SAMPLES + 1 samples: the loop end address points at one
         * more sample, which repeats the first */
        for (uint32_t n = 0; n <= TONE_LOOP_SAMPLES; n++) {
                sound_ram[n] = (uint16_t)_sine_table[n % 128];
        }

        scsp_slot_regs_t regs = { 0 };

        regs.kyonex  = 0;
        regs.kyonb   = 0;
        regs.sbctl   = 0;
        regs.ssctl   = 0;
        regs.lpctl   = 1;
        regs.pcm8b   = 0;
        regs.sa_high = 1;

        regs.sa      = 0;
        regs.lsa     = 0;
        regs.lea     = TONE_LOOP_SAMPLES;

        regs.d2r     = 0;
        regs.d1r     = 0;
        regs.eghold  = 0;
        regs.ar      = 31;

        regs.lpslnk  = 0;
        regs.krs     = 15; /* key rate scaling off */
        regs.dl      = 0;
        regs.rr      = 31;

        regs.stwinh  = 0;
        regs.sdir    = 0;
        regs.tl      = 0;

        regs.mdl     = 0;
        regs.mdxsl   = 0;
        regs.mdysl   = 0;

        regs.oct     = 0;
        regs.fns     = 284;

        regs.lfore   = 0;
        regs.lfof    = 0;
        regs.plfows  = 0;
        regs.plfos   = 0;
        regs.alfows  = 0;
        regs.alfos   = 0;

        regs.isel    = 0;
        regs.imxl    = 0;

        regs.disdl   = 7; /* full direct send level */
        regs.dipan   = 0;
        regs.efsdl   = 0;
        regs.efpan   = 0;

#if defined(DEBUG)
        assert(regs.raw[0] == 0x0021);
        assert(regs.raw[1] == 0x0000);
        assert(regs.raw[2] == 0x0000);
        assert(regs.raw[3] == 0x4000);
        assert(regs.raw[4] == 0x001F);
        assert(regs.raw[5] == 0x3C1F);
        assert(regs.raw[6] == 0x0000);
        assert(regs.raw[7] == 0x0000);
        assert(regs.raw[8] == 0x011C);
        assert(regs.raw[9] == 0x0000);
        assert(regs.raw[10] == 0x0000);
        assert(regs.raw[11] == 0xE000);
#endif /* defined(DEBUG) */

        for (uint8_t word = 1; word < SCSP_SLOT_WORD_COUNT; word++) {
                scsp_slot_word_set(TONE_SLOT, word, regs.raw[word]);
        }

        scsp_slot_word_set(TONE_SLOT, 0, regs.raw[0]);

#if defined(DEBUG)
        _octave = -2;
        assert(tone_frequency_decihz_get() == 1100);
        _octave = -1;
        assert(tone_frequency_decihz_get() == 2200);
        _octave = 0;
        assert(tone_frequency_decihz_get() == 4401);
        _octave = 1;
        assert(tone_frequency_decihz_get() == 8802);
        _octave = 2;
        assert(tone_frequency_decihz_get() == 17603);
#endif /* defined(DEBUG) */

        _octave = 0;
        _on = false;
}

void
tone_key_on(void)
{
        scsp_slot_kyonb_set(TONE_SLOT, true);
        scsp_slot_kyonex_execute();

        _on = true;
}

void
tone_key_off(void)
{
        scsp_slot_kyonb_set(TONE_SLOT, false);
        scsp_slot_kyonex_execute();

        _on = false;
}

bool
tone_is_on(void)
{
        return _on;
}

void
tone_octave_set(int8_t octave)
{
        int8_t clamped;

        clamped = octave;

        if (clamped < TONE_OCTAVE_MIN) {
                clamped = TONE_OCTAVE_MIN;
        } else if (clamped > TONE_OCTAVE_MAX) {
                clamped = TONE_OCTAVE_MAX;
        }

        _octave = clamped;

        scsp_slot_regs_t regs = { 0 };

        regs.oct = (uint16_t)(clamped & 0x0F);
        regs.fns = 284;

#if defined(DEBUG)
        switch (_octave) {
        case -2:
                assert(regs.raw[SCSP_SLOT_WORD_OCT] == 0x711C);
                break;
        case -1:
                assert(regs.raw[SCSP_SLOT_WORD_OCT] == 0x791C);
                break;
        case 0:
                assert(regs.raw[SCSP_SLOT_WORD_OCT] == 0x011C);
                break;
        case 1:
                assert(regs.raw[SCSP_SLOT_WORD_OCT] == 0x091C);
                break;
        case 2:
                assert(regs.raw[SCSP_SLOT_WORD_OCT] == 0x111C);
                break;
        default:
                assert(false);
                break;
        }
#endif /* defined(DEBUG) */

        scsp_slot_word_set(TONE_SLOT, SCSP_SLOT_WORD_OCT,
            regs.raw[SCSP_SLOT_WORD_OCT]);
}

int8_t
tone_octave_get(void)
{
        return _octave;
}

uint32_t
tone_frequency_decihz_get(void)
{
        uint32_t base;

        if (_octave >= 0) {
                base = 576828000UL << _octave;
        } else {
                base = 576828000UL >> (-_octave);
        }

        return (base + 65536UL) / 131072UL;
}

void
tone_volume_set(int8_t volume)
{
        int8_t clamped;

        clamped = volume;

        if (clamped < TONE_VOLUME_MIN) {
                clamped = TONE_VOLUME_MIN;
        } else if (clamped > TONE_VOLUME_MAX) {
                clamped = TONE_VOLUME_MAX;
        }

        _volume = (uint8_t)clamped;

        scsp_mvol_set(scsp_mvol_pack(true, false, _volume));
}

uint8_t
tone_volume_get(void)
{
        return _volume;
}

uint8_t
tone_position_get(void)
{
        scsp_mslc_set(TONE_SLOT);

        return scsp_ca_get();
}
