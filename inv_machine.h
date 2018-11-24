#ifndef SPACE_INVADERS_H
#define SPACE_INVADERS_H

#include "intel8080_emulator.h"

#define FILE1 "rom/invaders.h"
#define FILE2 "rom/invaders.g"
#define FILE3 "rom/invaders.f"
#define FILE4 "rom/invaders.e"

typedef enum machine_keys {
	LEFT, RIGHT, SHOOT, 
	START, COIN, TILT 
} mac_keys;

typedef struct machine {
	State8080 *state;

	double last_timer, next_interrupt;
	uint8_t which_interrupt;

	unsigned int *screen_buffer;

	uint8_t shift0, shift1, shift_offset;
	uint8_t port1, port2;
} machine;

machine* init_machine(void);
void read_file_into_memory_at(State8080* state, const char* filename, uint32_t offset);

void* frame_buffer(State8080* state);
void load_screen_buffer(machine* mac);
double get_time_micro(void);

uint8_t machine_in(machine* mac, uint8_t port);
void machine_out(machine* mac, uint8_t port, uint8_t value);

void machine_key_down(machine* mac, mac_keys key);
void machine_key_up(machine* mac, mac_keys key);

void run_cpu(machine* mac);

#endif
