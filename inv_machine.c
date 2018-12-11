#include "inv_machine.h"
#include <sys/time.h>
#include <string.h>

machine* init_machine() {
	machine* mac = (machine*) malloc(sizeof(machine));
	mac->screen_buffer = (unsigned int*) malloc(224 * 256 * 4);
	mac->state = init_8080();

	memset(mac->screen_buffer, 0, 224 * 256 * 4);

	mac->last_timer = 0.0;
	mac->shift0 = mac->shift1 = 0;
	mac->shift_offset = 0;
	mac->port1 = mac->port2 = 0;

	read_file_into_memory_at(mac->state, FILE1, 0);
	read_file_into_memory_at(mac->state, FILE2, 0x800);
	read_file_into_memory_at(mac->state, FILE3, 0x1000);
	read_file_into_memory_at(mac->state, FILE4, 0x1800);

	return mac;
}

void read_file_into_memory_at(State8080* state, const char* filename, uint32_t offset) {

	FILE *file = fopen(filename, "rb");

	if(file == NULL) {
		printf("Error: Cannot open %s!\n", filename);
		exit(1);
	}

	fseek(file, 0L, SEEK_END);
	long fsize = ftell(file);
	fseek(file, 0L, SEEK_SET);

	unsigned char *buffer = &state->memory[offset];

	fread(buffer, fsize, 1, file);
	fclose(file);
}

void* frame_buffer(State8080* state) {
	return (void*) &state->memory[0x2400]; 
}

void load_screen_buffer(machine* mac) {
	int cnt;
	for(cnt = 0; cnt < 256 * 224 / 8; cnt++) {
		int i = (cnt * 8) / 256;	
		int j = (cnt * 8) % 256;

		uint8_t *byte = (uint8_t*) frame_buffer(mac->state) + cnt;

		int j_base = j;	
		unsigned int *pix;
		int bit;	
		for(bit = 0; bit < 8; bit++) {
			j = j_base + bit;

			int offset = (255-j) * 224 * 4 + (i * 4);

			pix = (unsigned int*) ((uint8_t*) mac->screen_buffer + offset);

			if((*byte & (1 << bit)) != 0)
				*pix = 0xffffffffL;
			else
				*pix = 0x00000000L;

			pix -= 224;
		}
	}	
}

double get_time_micro() {
	struct timeval time;
	gettimeofday(&time, NULL);
	return (double) time.tv_sec * 1e6 + (double) time.tv_usec;
}

uint8_t machine_in(machine* mac, uint8_t port) {
	uint8_t a;	
	switch(port) {
		case 1:
			a = mac->port1;
			break;
		case 2:
			a = mac->port2;
			break;
		case 3: {
					uint16_t temp = (mac->shift1 << 8) | mac->shift0;
					a = (temp >> (8 - mac->shift_offset)) & 0xff;
					break;
				}
		default:
				printf("/nUnknown in-port: %d/n", port);
				exit(1);
	}
	return a;
}

void machine_out(machine* mac, uint8_t port, uint8_t value) {
	switch(port) {
		case 2: mac->shift_offset = value & 0x7;
				break;
		case 3: break;
		case 4: mac->shift0 = mac->shift1;
				mac->shift1 = value;
				break;
		case 5: break;
		case 6: break;
		default:
				printf("/nUnknown out-port: %d/n", port);
				exit(1);
	}
}

void machine_key_down(machine* mac, mac_keys key) {
	switch(key) {
		case LEFT: mac->port1 |= 1 << 5;	
				   break;
		case RIGHT: mac->port1 |= 1 << 6;
					break;
		case SHOOT: mac->port1 |= 1 << 4;
					break;
		case START: mac->port1 |= 1 << 2;
					break;
		case COIN: mac->port1 |= 1 << 0;	
				   break;
		case TILT: mac->port2 |= 1 << 2;
				   break;
	}
}

void machine_key_up(machine* mac, mac_keys key) {
	switch(key) {
		case LEFT: mac->port1 &= ~(1 << 5);
				   break;
		case RIGHT: mac->port1 &= ~(1 << 6);
					break;
		case SHOOT: mac->port1 &= ~(1 << 4);
					break;
		case START: mac->port1 &= ~(1 << 2);
					break;
		case COIN: mac->port1 &= ~(1 << 0);	
				   break;
		case TILT: mac->port2 &= ~(1 << 2);
				   break;
	}
}

void run_cpu(machine* mac) {
	double now = get_time_micro();	

	if(mac->last_timer == 0.0) {
		mac->next_interrupt = now + 16000.0;	
		mac->which_interrupt = 1;
	}

	if(mac->state->int_enable) {
		if(now > mac->next_interrupt) {
			generate_interrupt(mac->state, mac->which_interrupt);
			mac->which_interrupt = (mac->which_interrupt == 1)? 2: 1;
			mac->next_interrupt = now + 8000.0;
		}
	}

	double time_elapsed = now - mac->last_timer;
	int cycles_to_run = 2 * time_elapsed;		//for 2MHz => 2 cycles per microsec	
	int cycles_done = 0;

	while(cycles_to_run > cycles_done) {
		unsigned char *opcode = &mac->state->memory[mac->state->pc];	

		if(*opcode == 0xdb) {
			uint8_t port = opcode[1];
			mac->state->a = machine_in(mac, port);
			mac->state->pc += 2;
			cycles_done += 3; 

		} else if(*opcode == 0xd3) {
			uint8_t port = opcode[1];
			machine_out(mac, port, mac->state->a);
			mac->state->pc += 2;
			cycles_done += 3;

		} else {
			emulate_opcode(mac->state);
			cycles_done += mac->state->cycles;
		}
	}	

	mac->last_timer = now;
}
