#ifndef INTEL_8080_EMULATOR_H
#define INTEL_8080_EMULATOR_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

struct ConditionCodes {
    uint8_t z:1;
    uint8_t s:1;
    uint8_t p:1;
    uint8_t cy:1;
    uint8_t ac:1;
    uint8_t pad:3;
};

typedef struct State8080 {
    uint8_t a, b, c, d, e, h, l;
    uint16_t sp, pc;
    uint8_t *memory;
    uint8_t int_enable;
	uint8_t cycles;
    struct ConditionCodes flags;
} State8080;

State8080* init_8080(void);

void not_implemented(State8080* state);
void show_state(State8080* state);
int parity(uint8_t n);
int parse_opcode(unsigned char* buffer, int offset);

void emulate_opcode(State8080* state);
void generate_interrupt(State8080* state, int int_num);

#endif
