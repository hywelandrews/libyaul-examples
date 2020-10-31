#ifndef _SHARED_MENU_SCROLL_MENU_H
#define _SHARED_MENU_SCROLL_MENU_H

#include <stdint.h>
#include <stddef.h>

#include "menu.h"

typedef struct scroll_menu_state scroll_menu_state_t;

typedef void (*scroll_menu_fn_t)(scroll_menu_state_t *);

#define SCROLL_MENU_STATE_MASK (0x0003)

typedef enum {
        SCROLL_MENU_STATE_NONE          = 0,
        SCROLL_MENU_STATE_ENABLED       = 1 << 0,
        SCROLL_MENU_STATE_INPUT_ENABLED = 1 << 1
} scroll_menu_state_flags_t;

struct scroll_menu_state {
        int8_t view_height; 
        int8_t top_index;
        int8_t bottom_index;

        menu_entry_t *entries;
        scroll_menu_state_flags_t flags;
        void *data;

        /* Private */
        menu_state_t _menu_state;
        scroll_menu_fn_t _input_fn;
        scroll_menu_fn_t _update_fn;
        menu_cursor_t _cursor;
        menu_cursor_t _y;
        menu_cursor_t _gp;
};

void scroll_menu_init(scroll_menu_state_t *);

void scroll_menu_entries_set(scroll_menu_state_t *, menu_entry_t *);

void scroll_menu_input_set(scroll_menu_state_t *, scroll_menu_fn_t);
void scroll_menu_update_set(scroll_menu_state_t *, scroll_menu_fn_t);

menu_cursor_t scroll_menu_local_cursor(scroll_menu_state_t *);
menu_cursor_t scroll_menu_cursor(scroll_menu_state_t *);
void scroll_menu_cursor_down(scroll_menu_state_t *);
void scroll_menu_cursor_up(scroll_menu_state_t *);
void scroll_menu_action_call(scroll_menu_state_t *);

void scroll_menu_update(scroll_menu_state_t *);

#endif /* _SHARED_MENU_SCROLL_MENU_H */
