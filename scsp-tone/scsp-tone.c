/*
 * Copyright (c) Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <tone.h>

#define POSITION_CELLS (TONE_LOOP_SAMPLES / TONE_POSITION_STEP_SAMPLES)

static void _vblank_out_handler(void *work __unused);

static void _input(void);
static void _draw(void);

static smpc_peripheral_digital_t _digital;

static uint32_t _frame;
static bool _ready;

int
main(void)
{
        dbgio_init();
        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();

        while (true) {
                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &_digital);

                if (!_ready) {
                        /* Peripheral polling runs for 120 frames (two seconds
                         * at 60 Hz) before the sound RAM and the SCSP
                         * registers are first written */
                        if (_frame == 120) {
                                tone_init();
                                _ready = true;
                        }
                } else {
                        _input();
                }

                _draw();

                dbgio_flush();
                vdp2_sync();
                vdp2_sync_wait();

                _frame++;
        }

        return 0;
}

void
user_init(void)
{
        smpc_peripheral_init();

        /* Before the VBlank-OUT handler below exists, so the SMPC command
         * cannot interleave with the pad INTBACK it issues */
        tone_sound_cpu_halt();

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            RGB1555(1, 0, 3, 15));

        vdp_sync_vblank_out_set(_vblank_out_handler, NULL);

        vdp2_tvmd_display_set();
}

static void
_vblank_out_handler(void *work __unused)
{
        smpc_peripheral_intback_issue();
}

static void
_input(void)
{
        /* In libyaul `held` is set only on the frame a button goes down, so
         * each press acts once. `pressed` is the level, set every frame the
         * button stays down. */
        if (_digital.held.button.a != 0) {
                if (tone_is_on()) {
                        tone_key_off();
                } else {
                        tone_key_on();
                }
        }

        if (_digital.held.button.up != 0) {
                tone_octave_set(tone_octave_get() + 1);
        } else if (_digital.held.button.down != 0) {
                tone_octave_set(tone_octave_get() - 1);
        }

        if (_digital.held.button.right != 0) {
                tone_volume_set(tone_volume_get() + 1);
        } else if (_digital.held.button.left != 0) {
                tone_volume_set(tone_volume_get() - 1);
        }
}

static void
_draw(void)
{
        dbgio_printf("\033[H\033[2J");

        dbgio_printf("SCSP TONE\n\n");

        if (!_ready) {
                dbgio_printf("INITIALIZING\n");
                return;
        }

        dbgio_printf("KEY      %s\n", tone_is_on() ? "ON" : "OFF");

        /* %+d prints the signed octave, sign included */
        dbgio_printf("OCTAVE   %+d\n", (int)tone_octave_get());

        const uint32_t freq = tone_frequency_decihz_get();

        dbgio_printf("FREQ     %u.%u HZ\n", (unsigned int)(freq / 10),
            (unsigned int)(freq % 10));

        dbgio_printf("VOLUME   %u / %u\n", (unsigned int)tone_volume_get(),
            (unsigned int)TONE_VOLUME_MAX);

        dbgio_printf("POSITION ");

        if (tone_is_on()) {
                char cells[POSITION_CELLS + 1];
                const uint8_t position = tone_position_get();

                for (uint8_t i = 0; i < POSITION_CELLS; i++) {
                        cells[i] = (position == i) ? '#' : '.';
                }

                cells[POSITION_CELLS] = '\0';

                dbgio_printf("[%s]", cells);
        } else {
                dbgio_printf("[----]");
        }

        dbgio_printf("\n\n");

        dbgio_printf("A TOGGLE  UP DN OCTAVE  LT RT VOLUME\n");
}
