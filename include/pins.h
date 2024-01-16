#ifndef PINS_H
#define PINS_H

void pins_init(void);

void pins_set_clear_interrupt(void (*f)(void));

#endif