#ifndef FIBER_H
#define FIBER_H

#include <sys/cdefs.h>

#include <assert.h>
#include <stdint.h>

#include <cpu/registers.h>

typedef struct fiber fiber_t;

typedef void (*fiber_entry_t)(void);

struct fiber {
        cpu_registers_t reg_file;
        ssize_t size;
        uint8_t *stack;
} __aligned(16);

static_assert(sizeof(fiber_t) == ((92 + 20)));

typedef void *(*fiber_stack_alloc_t)(size_t amount, size_t align);
typedef void (*fiber_stack_free_t)(void *p);

extern void fiber_init(void);
extern void fiber_stack_allocator_set(fiber_stack_alloc_t stack_alloc, fiber_stack_free_t stack_free);

extern int32_t fiber_fiber_init(fiber_t *fiber, ssize_t stack_size, fiber_entry_t entry);
extern void fiber_yield(fiber_t *to);

#endif /* !FIBER_H */
