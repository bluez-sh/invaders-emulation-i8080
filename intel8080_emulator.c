#include "intel8080_emulator.h"
#include <string.h>
//#include "i8080_tests.c"

//number of cycles for each instruction
//+6 for conditional calls and rets when conditions are met
const uint8_t op_cycles[] = {
//  0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f
    4,  10, 7,  5,  5,  5,  7,  4,  4,  10, 7,  5,  5,  5,  7,  4,  // 0
    4,  10, 7,  5,  5,  5,  7,  4,  4,  10, 7,  5,  5,  5,  7,  4,  // 1
    4,  10, 16, 5,  5,  5,  7,  4,  4,  10, 16, 5,  5,  5,  7,  4,  // 2
    4,  10, 13, 5,  10, 10, 10, 4,  4,  10, 13, 5,  5,  5,  7,  4,  // 3
    5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,  // 4
    5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,  // 5
    5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,  // 6
    7,  7,  7,  7,  7,  7,  7,  7,  5,  5,  5,  5,  5,  5,  7,  5,  // 7
    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,  // 8
    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,  // 9
    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,  // a
    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,  // b
    5,  10, 10, 10, 11, 11, 7,  11, 5,  10, 10, 10, 11, 11, 7,  11, // c
    5,  10, 10, 10, 11, 11, 7,  11, 5,  10, 10, 10, 11, 11, 7,  11, // d
    5,  10, 10, 18, 11, 11, 7,  11, 5,  5,  10, 5,  11, 11, 7,  11, // e
    5,  10, 10, 4,  11, 11, 7,  11, 5,  5,  10, 4,  11, 11, 7,  11  // f
};

State8080* init_8080() {
	State8080* state = (State8080*) malloc(sizeof(State8080));
	state->memory = (uint8_t*) malloc(0x10000); //16k memory
	
	memset(state->memory, 0, 0x10000);

	state->a = 0;
    state->b = state->c = 0;
    state->d = state->e = 0;
    state->h = state->l = 0;
    state->pc = 0;
    state->sp = 0;
    state->int_enable = 0;
	state->cycles = 0;

	return state;
}

void not_implemented(State8080* state) {
    printf("error: unimplemented instruction\n");
	parse_opcode(state->memory, state->pc-1);
	show_state(state);

	free(state->memory);
	free(state);
	exit(1);
}

void show_state(State8080 *state) {
    printf("\tz = %d, s = %d, p = %d, cy = %d\n",
            state->flags.z, state->flags.s,
            state->flags.p, state->flags.cy);

    printf("\ta $%02x b $%02x c $%02x d $%02x e $%02x h $%02x l $%02x sp %04x\n",
            state->a, state->b, state->c, state->d,
            state->e, state->h, state->l, state->sp);
}

int parity(uint8_t n) {        //returns 1 for even and 0 for odd
    int val = 0;
    while(n) {
        val = !val;
        n = n & (n-1);
    }
    return !val;
}

int parse_opcode(unsigned char *buffer, int offset) {
    unsigned char *code = buffer + offset;
    int op_bytes = 1;
    printf("%04x ", offset);

    switch(*code) {
        case 0x00: printf("nop"); break;
        case 0x01: printf("lxi    b,#$%02x%02x", code[2], code[1]); op_bytes=3; break;
        case 0x02: printf("stax   b"); break;
        case 0x03: printf("inx    b"); break;
        case 0x04: printf("inr    b"); break;
        case 0x05: printf("dcr    b"); break;
        case 0x06: printf("mvi    b,#$%02x", code[1]); op_bytes=2; break;
        case 0x07: printf("rlc"); break;
        case 0x08: printf("nop"); break;
        case 0x09: printf("dad    b"); break;
        case 0x0a: printf("ldax   b"); break;
        case 0x0b: printf("dcx    b"); break;
        case 0x0c: printf("inr    c"); break;
        case 0x0d: printf("dcr    c"); break;
        case 0x0e: printf("mvi    c,#$%02x", code[1]); op_bytes=2; break;
        case 0x0f: printf("rrc"); break;
        case 0x10: printf("nop"); break;
        case 0x11: printf("lxi    d,#$%02x%02x", code[2], code[1]); op_bytes=3; break;
        case 0x12: printf("stax   d"); break;
        case 0x13: printf("inx    d"); break;
        case 0x14: printf("inr    d"); break;
        case 0x15: printf("dcr    d"); break;
        case 0x16: printf("mvi    d,#$%02x", code[1]); op_bytes=2; break;
        case 0x17: printf("ral"); break;
        case 0x18: printf("nop"); break;
        case 0x19: printf("dad    d"); break;
        case 0x1a: printf("ldax   d"); break;
        case 0x1b: printf("dcx    d"); break;
        case 0x1c: printf("inr    e"); break;
        case 0x1d: printf("dcr    e"); break;
        case 0x1e: printf("mvi    e,#$%02x", code[1]); op_bytes=2; break;
        case 0x1f: printf("rar"); break;
        case 0x20: printf("rim"); break;
        case 0x21: printf("lxi    h,#$%02x%02x", code[2], code[1]); op_bytes=3; break;
        case 0x22: printf("shld   $%02x%02x", code[2], code[1]); op_bytes=3; break;
        case 0x23: printf("inx    h"); break;
        case 0x24: printf("inr    h"); break;
        case 0x25: printf("dcr    h"); break;
        case 0x26: printf("mvi    h,#$%02x", code[1]); op_bytes=2; break;
        case 0x27: printf("daa"); break;
        case 0x28: printf("nop"); break;
        case 0x29: printf("dad    h"); break;
        case 0x2a: printf("lhld   $%02x%02x", code[2], code[1]); op_bytes=3; break;
        case 0x2b: printf("dcx    h"); break;
        case 0x2c: printf("inr    l"); break;
        case 0x2d: printf("dcr    l"); break;
        case 0x2e: printf("mvi    l,#$%02x", code[1]); op_bytes=2; break;
        case 0x2f: printf("cma"); break;
        case 0x30: printf("sim"); break;
        case 0x31: printf("lxi    sp,#$%02x%02x", code[2], code[1]); op_bytes=3; break;
        case 0x32: printf("sta    $%02x%02x", code[2], code[1]); op_bytes=3; break;
        case 0x33: printf("inx    sp"); break;
        case 0x34: printf("inr    m"); break;
        case 0x35: printf("dcr    m"); break;
        case 0x36: printf("mvi    m,#$%02x", code[1]); op_bytes=2; break;
        case 0x37: printf("stc"); break;
        case 0x38: printf("nop"); break;
        case 0x39: printf("dad    sp"); break;
        case 0x3a: printf("lda    $%02x%02x", code[2], code[1]); op_bytes=3; break;
        case 0x3b: printf("dcx    sp"); break;
        case 0x3c: printf("inr    a"); break;
        case 0x3d: printf("dcr    a"); break;
        case 0x3e: printf("mvi    a,#0x%02x", code[1]); op_bytes=2; break;
        case 0x3f: printf("cmc"); break;
        case 0x40: printf("mov    b,b"); break;
        case 0x41: printf("mov    b,c"); break;
        case 0x42: printf("mov    b,d"); break;
        case 0x43: printf("mov    b,e"); break;
        case 0x44: printf("mov    b,h"); break;
        case 0x45: printf("mov    b,l"); break;
        case 0x46: printf("mov    b,m"); break;
        case 0x47: printf("mov    b,a"); break;
        case 0x48: printf("mov    c,b"); break;
        case 0x49: printf("mov    c,c"); break;
        case 0x4a: printf("mov    c,d"); break;
        case 0x4b: printf("mov    c,e"); break;
        case 0x4c: printf("mov    c,h"); break;
        case 0x4d: printf("mov    c,l"); break;
        case 0x4e: printf("mov    c,m"); break;
        case 0x4f: printf("mov    c,a"); break;
        case 0x50: printf("mov    d,b"); break;
        case 0x51: printf("mov    d,c"); break;
        case 0x52: printf("mov    d,d"); break;
        case 0x53: printf("mov    d,e"); break;
        case 0x54: printf("mov    d,h"); break;
        case 0x55: printf("mov    d,l"); break;
        case 0x56: printf("mov    d,m"); break;
        case 0x57: printf("mov    d,a"); break;
        case 0x58: printf("mov    e,b"); break;
        case 0x59: printf("mov    e,c"); break;
        case 0x5a: printf("mov    e,d"); break;
        case 0x5b: printf("mov    e,e"); break;
        case 0x5c: printf("mov    e,h"); break;
        case 0x5d: printf("mov    e,l"); break;
        case 0x5e: printf("mov    e,m"); break;
        case 0x5f: printf("mov    e,a"); break;
        case 0x60: printf("mov    h,b"); break;
        case 0x61: printf("mov    h,c"); break;
        case 0x62: printf("mov    h,d"); break;
        case 0x63: printf("mov    h,e"); break;
        case 0x64: printf("mov    h,h"); break;
        case 0x65: printf("mov    h,l"); break;
        case 0x66: printf("mov    h,m"); break;
        case 0x67: printf("mov    h,a"); break;
        case 0x68: printf("mov    l,b"); break;
        case 0x69: printf("mov    l,c"); break;
        case 0x6a: printf("mov    l,d"); break;
        case 0x6b: printf("mov    l,e"); break;
        case 0x6c: printf("mov    l,h"); break;
        case 0x6d: printf("mov    l,l"); break;
        case 0x6e: printf("mov    l,m"); break;
        case 0x6f: printf("mov    l,a"); break;
        case 0x70: printf("mov    m,b"); break;
        case 0x71: printf("mov    m,c"); break;
        case 0x72: printf("mov    m,d"); break;
        case 0x73: printf("mov    m,e"); break;
        case 0x74: printf("mov    m,h"); break;
        case 0x75: printf("mov    m,l"); break;
        case 0x76: printf("hlt"); break;
        case 0x77: printf("mov    m,a"); break;
        case 0x78: printf("mov    a, b"); break;
        case 0x79: printf("mov    a, c"); break;
        case 0x7a: printf("mov    a, d"); break;
        case 0x7b: printf("mov    a, e"); break;
        case 0x7c: printf("mov    a, h"); break;
        case 0x7d: printf("mov    a, l"); break;
        case 0x7e: printf("mov    a, m"); break;
        case 0x7f: printf("mov    a, a"); break;
        case 0x80: printf("add    b"); break;
        case 0x81: printf("add    c"); break;
        case 0x82: printf("add    d"); break;
        case 0x83: printf("add    e"); break;
        case 0x84: printf("add    h"); break;
        case 0x85: printf("add    l"); break;
        case 0x86: printf("add    m"); break;
        case 0x87: printf("add    a"); break;
        case 0x88: printf("adc    b"); break;
        case 0x89: printf("adc    c"); break;
        case 0x8a: printf("adc    d"); break;
        case 0x8b: printf("adc    e"); break;
        case 0x8c: printf("adc    h"); break;
        case 0x8d: printf("adc    l"); break;
        case 0x8e: printf("adc    m"); break;
        case 0x8f: printf("adc    a"); break;
        case 0x90: printf("sub    b"); break;
        case 0x91: printf("sub    c"); break;
        case 0x92: printf("sub    d"); break;
        case 0x93: printf("sub    e"); break;
        case 0x94: printf("sub    h"); break;
        case 0x95: printf("sub    l"); break;
        case 0x96: printf("sub    m"); break;
        case 0x97: printf("sub    a"); break;
        case 0x98: printf("sbb    b"); break;
        case 0x99: printf("sbb    c"); break;
        case 0x9a: printf("sbb    d"); break;
        case 0x9b: printf("sbb    e"); break;
        case 0x9c: printf("sbb    h"); break;
        case 0x9d: printf("sbb    l"); break;
        case 0x9e: printf("sbb    m"); break;
        case 0x9f: printf("sbb    a"); break;
        case 0xa0: printf("ana    b"); break;
        case 0xa1: printf("ana    c"); break;
        case 0xa2: printf("ana    d"); break;
        case 0xa3: printf("ana    e"); break;
        case 0xa4: printf("ana    h"); break;
        case 0xa5: printf("ana    l"); break;
        case 0xa6: printf("ana    m"); break;
        case 0xa7: printf("ana    a"); break;
        case 0xa8: printf("xra    b"); break;
        case 0xa9: printf("xra    c"); break;
        case 0xaa: printf("xra    d"); break;
        case 0xab: printf("xra    e"); break;
        case 0xac: printf("xra    h"); break;
        case 0xad: printf("xra    l"); break;
        case 0xae: printf("xra    m"); break;
        case 0xaf: printf("xra    a"); break;
        case 0xb0: printf("ora    b"); break;
        case 0xb1: printf("ora    c"); break;
        case 0xb2: printf("ora    d"); break;
        case 0xb3: printf("ora    e"); break;
        case 0xb4: printf("ora    h"); break;
        case 0xb5: printf("ora    l"); break;
        case 0xb6: printf("ora    m"); break;
        case 0xb7: printf("ora    a"); break;
        case 0xb8: printf("xra    b"); break;
        case 0xb9: printf("cmp    c"); break;
        case 0xba: printf("cmp    d"); break;
        case 0xbb: printf("cmp    e"); break;
        case 0xbc: printf("cmp    h"); break;
        case 0xbd: printf("cmp    l"); break;
        case 0xbe: printf("cmp    m"); break;
        case 0xbf: printf("cmp    a"); break;
        case 0xc0: printf("rnz"); break;
        case 0xc1: printf("pop    b"); break;
        case 0xc2: printf("jnz    $%02x%02x", code[2], code[1]); op_bytes=3; break;
        case 0xc3: printf("jmp    $%02x%02x", code[2], code[1]); op_bytes=3; break;
        case 0xc4: printf("cnz    $%02x%02x", code[2], code[1]); op_bytes=3; break;
        case 0xc5: printf("push   b"); break;
        case 0xc6: printf("adi    #$%02x", code[1]); op_bytes=2; break;
        case 0xc7: printf("rst    0"); break;
        case 0xc8: printf("rz"); break;
        case 0xc9: printf("ret"); break;
        case 0xca: printf("jz     $%02x%02x", code[2], code[1]); op_bytes=3; break;
        case 0xcb: printf("nop"); break;
        case 0xcc: printf("cz     $%02x%02x", code[2], code[1]); op_bytes=3; break;
        case 0xcd: printf("call   $%02x%02x", code[2], code[1]); op_bytes=3; break;
        case 0xce: printf("aci    #$%02x", code[1]); op_bytes=2; break;
        case 0xcf: printf("rst    1"); break;
        case 0xd0: printf("rnc"); break;
        case 0xd1: printf("pop    d"); break;
        case 0xd2: printf("jnc    $%02x%02x", code[2], code[1]); op_bytes=3; break;
        case 0xd3: printf("out    #$%02x", code[1]); op_bytes=2; break;
        case 0xd4: printf("cnc    $%02x%02x", code[2], code[1]); op_bytes=3; break;
        case 0xd5: printf("push   d"); break;
        case 0xd6: printf("sui    #$%02x", code[1]); op_bytes=2; break;
        case 0xd7: printf("rst    2"); break;
        case 0xd8: printf("rc"); break;
        case 0xd9: printf("nop"); break;
        case 0xda: printf("jc     $%02x%02x", code[2], code[1]); op_bytes=3; break;
        case 0xdb: printf("in     #$%02x", code[1]); op_bytes=2; break;
        case 0xdc: printf("cc     $%02x%02x", code[2], code[1]); op_bytes=3; break;
        case 0xdd: printf("nop"); break;
        case 0xde: printf("sbi    #$%02x", code[1]); op_bytes=2; break;
        case 0xdf: printf("rst    3"); break;
        case 0xe0: printf("rpo"); break;
        case 0xe1: printf("pop    h"); break;
        case 0xe2: printf("jpo    $%02x%02x", code[2], code[1]); op_bytes=3; break;
        case 0xe3: printf("xthl"); break;
        case 0xe4: printf("cpo    $%02x%02x", code[2], code[1]); op_bytes=3; break;
        case 0xe5: printf("push   h"); break;
        case 0xe6: printf("ani    #$%02x", code[1]); op_bytes=2; break;
        case 0xe7: printf("rst    4"); break;
        case 0xe8: printf("rpe"); break;
        case 0xe9: printf("pchl"); break;
        case 0xea: printf("jpe    $%02x%02x", code[2], code[1]); op_bytes=3; break;
        case 0xeb: printf("xchg"); break;
        case 0xec: printf("cpe    $%02x%02x", code[2], code[1]); op_bytes=3; break;
        case 0xed: printf("nop"); break;
        case 0xee: printf("xri    #$%02x", code[1]); op_bytes=2; break;
        case 0xef: printf("rst    5"); break;
        case 0xf0: printf("rp"); break;
        case 0xf1: printf("pop    psw"); break;
        case 0xf2: printf("jp     $%02x%02x", code[2], code[1]); op_bytes=3; break;
        case 0xf3: printf("di"); break;
        case 0xf4: printf("cp     $%02x%02x", code[2], code[1]); op_bytes=3; break;
        case 0xf5: printf("push   psw"); break;
        case 0xf6: printf("ori    #$%02x", code[1]); op_bytes=2; break;
        case 0xf7: printf("rst    6"); break;
        case 0xf8: printf("rm"); break;
        case 0xf9: printf("sphl"); break;
        case 0xfa: printf("jm     $%02x%02x", code[2], code[1]); op_bytes=3; break;
        case 0xfb: printf("ei"); break;
        case 0xfc: printf("cm     $%02x%02x", code[2], code[1]); op_bytes=3; break;
        case 0xfd: printf("nop"); break;
        case 0xfe: printf("cpi    #$%02x", code[1]); op_bytes=2; break;
        case 0xff: printf("rst    7"); break;
    }

    printf("\n");
    return op_bytes;
}

void emulate_opcode(State8080* state) {
    unsigned char *opcode = &state->memory[state->pc];
    //parse_opcode(state->memory, state->pc);

	state->cycles = op_cycles[*opcode];

	state->pc++;

    switch(*opcode) {
        case 0x00: break;
        case 0x01:
            state->c = opcode[1];
            state->b = opcode[2];
            state->pc += 2;
            break;
        case 0x02: {
            uint16_t addr = (state->b << 8) | state->c;
            state->memory[addr] = state->a;
            break;
        }
        case 0x03: {
            uint16_t x = (state->b << 8) | state->c;
            x++;
            state->b = (x >> 8) & 0xff;
            state->c = x & 0xff;
			break;
		} 
        case 0x04:
			state->b++;

            state->flags.z = (state->b == 0);
            state->flags.s = ((state->b & 0x80) == 0x80);
            state->flags.p = parity(state->b);
            break;
        case 0x05:
            state->b--;

            state->flags.z = (state->b == 0);
            state->flags.s = ((state->b & 0x80) == 0x80);
            state->flags.p = parity(state->b);
            break;
        case 0x06:
            state->b = opcode[1];
            state->pc += 1;
            break;
        case 0x07: {
			uint8_t x = state->a;
			state->a = ((x & 0x80) >> 7) | (x << 1);
			state->flags.cy = ((x & 0x80) == 0x80);
			break;
		}
        case 0x08: break;
        case 0x09: {
			uint32_t hl = (state->h << 8) | (state->l);
			uint32_t bc = (state->b << 8) | (state->c);

			uint32_t result = hl + bc;

			state->h = (result >> 8) & 0xff;
			state->l = result & 0xff;
			state->flags.cy = (result > 0xffff);
			break;
		}
        case 0x0a: {
			uint16_t addr = (state->b << 8) | state->c;	
			state->a = state->memory[addr];
			break;
		}
        case 0x0b: {
			uint16_t x = (state->b << 8) | state->c;
			x--;
			state->b = (x >> 8) & 0xff;
			state->c = x & 0xff;
			break;
		}
        case 0x0c: 
			state->c++;

			state->flags.z = ((state->c & 0xff) == 0);
			state->flags.s = ((state->c & 0x80) == 0x80);
			state->flags.p = parity(state->c);
			break;
        case 0x0d:
			state->c--;

			state->flags.z = ((state->c & 0xff) == 0);
			state->flags.s = ((state->c & 0x80) == 0x80);
			state->flags.p = parity(state->c);
			break;
        case 0x0e: 
			state->c = opcode[1];
			state->pc++;
			break;
        case 0x0f: {
            uint8_t result = state->a;
            state->a = ((result & 1) << 7) | (result >> 1);
            state->flags.cy = ((result & 1) == 1);
            break;
        }
        case 0x10: break;
        case 0x11:
            state->d = opcode[2];
            state->e = opcode[1];
            state->pc += 2;
            break;
        case 0x12: {
			uint16_t addr = (state->d << 8) | state->e;
			state->memory[addr] = state->a;
			break;
		}
        case 0x13: {
            uint16_t x = (state->d << 8) | state->e;
            x++;
            state->d = (x >> 8) & 0xff;
            state->e = x & 0xff;
            break;
        }
        case 0x14: 
		    state->d++;

			state->flags.z = ((state->d & 0xff) == 0);
			state->flags.s = ((state->d & 0x80) == 0x80);
			state->flags.p = parity(state->d);
			break;
        case 0x15: 
			state->d--;

			state->flags.z = ((state->d & 0xff) == 0);
			state->flags.s = ((state->d & 0x80) == 0x80);
			state->flags.p = parity(state->d);
			break;
        case 0x16:
			state->d = opcode[1];
			state->pc++;
			break;
		case 0x17: {
			uint8_t x = state->a;
			state->a = state->flags.cy | (x << 1);
			state->flags.cy = ((x & 0x80) == 0x80);
			break;
		}
        case 0x18: break;
        case 0x19: {
            uint32_t x = (state->h << 8) | state->l;
            uint32_t y = (state->d << 8) | state->e;

            x += y;
			state->h = ((x >> 8) & 0xff);
			state->l = x & 0xff;

            state->flags.cy = (x > 0xffff);
            break;
        }
        case 0x1a: {
            uint16_t addr = (state->d << 8) | state->e;
            state->a = state->memory[addr];
            break;
        }
        case 0x1b: {
			uint16_t x = (state->d << 8) | state->e;
			x--;
			state->d = (x >> 8) & 0xff;
			state->e = x & 0xff;
			break;
		}
        case 0x1c: 
			state->e++;

			state->flags.z = ((state->e & 0xff) == 0);
			state->flags.s = ((state->e & 0x80) == 0x80);
			state->flags.p = parity(state->e);
			break;
        case 0x1d: 
			state->e--;

			state->flags.z = ((state->e & 0xff) == 0);
			state->flags.s = ((state->e & 0x80) == 0x80);
			state->flags.p = parity(state->e);
			break;
        case 0x1e:
			state->e = opcode[1];
			state->pc++;
			break;
        case 0x1f: {
            uint8_t result = state->a;
            state->a = ((state->flags.cy & 1) << 7) | (result >> 1);
            state->flags.cy = ((result & 1) == 1);
            break;
        }
        case 0x20: break;
        case 0x21:
            state->h = opcode[2];
            state->l = opcode[1];
            state->pc += 2;
            break;
        case 0x22: {
			uint16_t addr = (opcode[2] << 8) | opcode[1];
			state->memory[addr] = state->l;
			state->memory[addr+1] = state->h;
			state->pc += 2;
			break;
		} 
        case 0x23: {
            uint16_t x = (state->h << 8) | state->l;
            x++;
            state->h = (x >> 8) & 0xff;
            state->l = x & 0xff;
            break;
        }
        case 0x24: 
			state->h++;

			state->flags.z = ((state->h & 0xff) == 0);
			state->flags.s = ((state->h & 0x80) != 0);
			state->flags.p = parity(state->h);
			break;
        case 0x25: 
			state->h--;

			state->flags.z = ((state->h & 0xff) == 0);
			state->flags.s = ((state->h & 0x80) != 0);
			state->flags.p = parity(state->h);
			break;
		case 0x26:
			state->h = opcode[1];
			state->pc++;
			break;
        case 0x27: {
			uint8_t result = 0;

			uint8_t lsb = state->a & 0x0f;
			uint8_t msb = state->a >> 4;

			if((rand()%2) || lsb > 9)
				result += 0x06;
			if(state->flags.cy || msb > 9 || (msb >= 9 && lsb > 9)) {
				result += 0x60;
				state->flags.cy = 1;
			}
			state->a += result;
			state->flags.z = ((state->a & 0xff) == 0);
			state->flags.s = ((state->a & 0x80) == 0x80);
			state->flags.p = parity(state->a);
			break;
		}
        case 0x28: break;
        case 0x29: {
			uint32_t hl = (state->h << 8) | (state->l);

			uint32_t result = hl + hl;

			state->h = (result >> 8) & 0xff;
			state->l = result & 0xff;
			state->flags.cy = (result > 0xffff);
			break;
		}
        case 0x2a: {
			uint16_t addr = (opcode[2] << 8) | opcode[1];
			state->l = state->memory[addr];
			state->h = state->memory[addr+1];
			state->pc += 2;
			break;
		}
        case 0x2b: {
			uint16_t x = (state->h << 8) | state->l;
			x--;
			state->h = ((x >> 8) & 0xff);
			state->l = x & 0xff;
			break;
		}
        case 0x2c: 
			state->l++;

			state->flags.z = ((state->l & 0xff) == 0);
            state->flags.s = ((state->l & 0x80) != 0);
            state->flags.p = parity(state->l);
			break;
        case 0x2d: 
			state->l--;

			state->flags.z = ((state->l & 0xff) == 0);
            state->flags.s = ((state->l & 0x80) != 0);
            state->flags.p = parity(state->l);
			break;
        case 0x2e:
			state->l = opcode[1];
			state->pc++;
			break;
        case 0x2f: state->a = ~state->a; break;
        case 0x30: break;
        case 0x31: {
            state->sp = (opcode[2] << 8) | opcode[1];
            state->pc += 2;
            break;
        }
        case 0x32: {
            uint16_t addr = (opcode[2] << 8) | opcode[1];
            state->memory[addr] = state->a;
            state->pc += 2;
            break;
        }
        case 0x33: state->sp++; break; 
        case 0x34: {
			uint16_t addr = (state->h << 8) | state->l;
			uint8_t result = state->memory[addr];

			result++;

			state->flags.z = ((result & 0xff) == 0);
            state->flags.s = ((result & 0x80) != 0);
            state->flags.p = parity(result);

			state->memory[addr] = result;	
            break;
		}
        case 0x35: {
			uint16_t addr = (state->h << 8) | state->l;
			uint8_t result = state->memory[addr];

			result--;

			state->flags.z = ((result & 0xff) == 0);
            state->flags.s = ((result & 0x80) != 0);
            state->flags.p = parity(result);

			state->memory[addr] = result;	
            break;
		}
        case 0x36: {
			uint16_t addr = (state->h << 8) | state->l;
			state->memory[addr] = opcode[1];
			state->pc++;
			break;
		}			 
        case 0x37: state->flags.cy = 1; break;
        case 0x38: break;
        case 0x39: {
			uint32_t hl = (state->h << 8) | (state->l);

			uint32_t result = hl + state->sp;

			state->h = (result >> 8) & 0xff;
			state->l = result & 0xff;
			state->flags.cy = (result > 0xffff);
			break;
		}
        case 0x3a: {
            uint16_t addr = (opcode[2] << 8) | opcode[1];
            state->a = state->memory[addr];
            state->pc += 2;
            break;
        }
        case 0x3b: state->sp--; break;
        case 0x3c:
			state->a++;	
			
			state->flags.z = ((state->a & 0xff) == 0);
            state->flags.s = ((state->a & 0x80) != 0);
            state->flags.p = parity(state->a);
			break;
        case 0x3d:
			state->a--;	
			
			state->flags.z = ((state->a & 0xff) == 0);
            state->flags.s = ((state->a & 0x80) != 0);
            state->flags.p = parity(state->a);
			break;
        case 0x3e:
            state->a = opcode[1];
            state->pc++;
            break;
        case 0x3f: state->flags.cy = 0; break;
        case 0x40: state->b = state->b; break;
        case 0x41: state->b = state->c; break;
        case 0x42: state->b = state->d; break;
        case 0x43: state->b = state->e; break;
        case 0x44: state->b = state->h; break;
        case 0x45: state->b = state->l; break;
        case 0x46: {
			uint16_t addr = (state->h << 8) | state->l;
			state->b = state->memory[addr];
			break;
		}
        case 0x47: state->b = state->a; break;
        case 0x48: state->c = state->b; break;
        case 0x49: state->c = state->c; break;
        case 0x4a: state->c = state->d; break;
        case 0x4b: state->c = state->e; break;
        case 0x4c: state->c = state->h; break;
        case 0x4d: state->c = state->l; break;
        case 0x4e: {
			uint16_t addr = (state->h << 8) | state->l;
			state->c = state->memory[addr];
			break;
		}
        case 0x4f: state->c = state->a; break;
        case 0x50: state->d = state->b; break;
        case 0x51: state->d = state->c; break;
        case 0x52: state->d = state->d; break;
        case 0x53: state->d = state->e; break;
        case 0x54: state->d = state->h; break;
        case 0x55: state->d = state->l; break;
        case 0x56: {
			uint16_t addr = (state->h << 8) | state->l;
			state->d = state->memory[addr];
			break;
		}
        case 0x57: state->d = state->a; break;
        case 0x58: state->e = state->b; break;
        case 0x59: state->e = state->c; break;
        case 0x5a: state->e = state->d; break;
        case 0x5b: state->e = state->e; break;
        case 0x5c: state->e = state->h; break;
        case 0x5d: state->e = state->l; break;
        case 0x5e: {
		    uint16_t addr = (state->h << 8) | state->l;
			state->e = state->memory[addr];
			break;
		}
        case 0x5f: state->e = state->a; break;
        case 0x60: state->h = state->b; break;
        case 0x61: state->h = state->c; break;
        case 0x62: state->h = state->d; break;
        case 0x63: state->h = state->e; break;
        case 0x64: state->h = state->h; break;
        case 0x65: state->h = state->l;break;
        case 0x66: {
			uint16_t addr = (state->h << 8) | state->l;
			state->h = state->memory[addr];
			break;
		}
        case 0x67: state->h = state->a;break;
        case 0x68: state->l = state->b; break;
        case 0x69: state->l = state->c;break;
        case 0x6a: state->l = state->d;break;
        case 0x6b: state->l = state->e;break;
        case 0x6c: state->l = state->h;break;
        case 0x6d: state->l = state->l;break;
        case 0x6e: {
			uint16_t addr = (state->h << 8) | state->l;
			state->l = state->memory[addr];
			break;
		}
        case 0x6f: state->l = state->a; break;
        case 0x70: {
			uint16_t addr = (state->h << 8) | state->l;
			state->memory[addr] = state->b;
			break;
		}
		case 0x71: {
			uint16_t addr = (state->h << 8) | state->l;
			state->memory[addr] = state->c;
			break;
		}
		case 0x72: {
			uint16_t addr = (state->h << 8) | state->l;
			state->memory[addr] = state->d;
			break;
		}
        case 0x73: {
			uint16_t addr = (state->h << 8) | state->l;
			state->memory[addr] = state->e;
			break;
		}
        case 0x74: {
			uint16_t addr = (state->h << 8) | state->l;
			state->memory[addr] = state->h;
			break;
		}
		case 0x75: {
			uint16_t addr = (state->h << 8) | state->l;
			state->memory[addr] = state->l;
			break;
		}
        case 0x76: state->pc--; break;
        case 0x77: {
            uint16_t addr = (state->h << 8) | state->l;
            state->memory[addr] = state->a;
            break;
        }
        case 0x78: state->a = state->b; break;
        case 0x79: state->a = state->c; break;
        case 0x7a: state->a = state->d; break;
        case 0x7b: state->a = state->e; break;
        case 0x7c: state->a = state->h; break;
		case 0x7d: state->a = state->l; break;
        case 0x7e: {
			uint16_t addr = (state->h << 8) | state->l;
			state->a = state->memory[addr];
			break;
		}
        case 0x7f: break;
        case 0x80: {
            uint16_t result = (uint16_t) state->a + (uint16_t) state->b;
            state->flags.z = ((result & 0xff) == 0);
            state->flags.s = ((result & 0x80) != 0);
            state->flags.cy = (result > 0xff);
            state->flags.p = parity(result & 0xff);
            state->a = result & 0xff;
            break;
        }
        case 0x81: {
            uint16_t result = (uint16_t) state->a + (uint16_t) state->c;
            state->flags.z = ((result & 0xff) == 0);
            state->flags.s = ((result & 0x80) != 0);
            state->flags.cy = (result > 0xff);
            state->flags.p = parity(result & 0xff);
            state->a = result & 0xff;
            break;
        }
        case 0x82: {
            uint16_t result = (uint16_t) state->a + (uint16_t) state->d;
            state->a = result & 0xff;

            state->flags.z = ((result & 0xff) == 0);
            state->flags.s = ((result & 0x80) != 0);
            state->flags.cy = (result > 0xff);
            state->flags.p = parity(result & 0xff);
			break; 
		} 
        case 0x83: {
            uint16_t result = (uint16_t) state->a + (uint16_t) state->e;
            state->a = result & 0xff;

            state->flags.z = ((result & 0xff) == 0);
            state->flags.s = ((result & 0x80) != 0);
            state->flags.cy = (result > 0xff);
            state->flags.p = parity(result & 0xff);
			break; 
		} 
        case 0x84: {
            uint16_t result = (uint16_t) state->a + (uint16_t) state->h;
            state->a = result & 0xff;

            state->flags.z = ((result & 0xff) == 0);
            state->flags.s = ((result & 0x80) != 0);
            state->flags.cy = (result > 0xff);
            state->flags.p = parity(result & 0xff);
			break; 
		}
        case 0x85: {
            uint16_t result = (uint16_t) state->a + (uint16_t) state->l;
            state->a = result & 0xff;

            state->flags.z = ((result & 0xff) == 0);
            state->flags.s = ((result & 0x80) != 0);
            state->flags.cy = (result > 0xff);
            state->flags.p = parity(result & 0xff);
			break; 
		}
        case 0x86: {
            uint16_t offset = (state->h << 8) | (state->l);
            uint16_t result = (uint16_t) state->a + state->memory[offset];
            state->flags.z = ((result & 0xff) == 0);
            state->flags.s = ((result & 0x80) != 0);
            state->flags.cy = (result > 0xff);
            state->flags.p = parity(result & 0xff);
            state->a = result & 0xff;
            break;
        }
        case 0x87: {
            uint16_t result = (uint16_t) state->a + (uint16_t) state->a;
            state->a = result & 0xff;

            state->flags.z = ((result & 0xff) == 0);
            state->flags.s = ((result & 0x80) != 0);
            state->flags.cy = (result > 0xff);
            state->flags.p = parity(result & 0xff);
			break; 
		}
        case 0x88: {
			uint16_t x = (uint16_t) state->a + (uint16_t) state->b + state->flags.cy; 
			state->a = x & 0xff;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;
		}
        case 0x89: {
			uint16_t x = (uint16_t) state->a + (uint16_t) state->c + state->flags.cy; 
			state->a = x & 0xff;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;
		}
        case 0x8a: {
			uint16_t x = (uint16_t) state->a + (uint16_t) state->d + state->flags.cy; 
			state->a = x & 0xff;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;
		}
        case 0x8b: {	
			uint16_t x = (uint16_t) state->a + (uint16_t) state->e + state->flags.cy; 
			state->a = x & 0xff;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;
		}
        case 0x8c: {
			uint16_t x = (uint16_t) state->a + (uint16_t) state->h + state->flags.cy; 
			state->a = x & 0xff;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;
		}
        case 0x8d: {
			uint16_t x = (uint16_t) state->a + (uint16_t) state->l + state->flags.cy; 
			state->a = x & 0xff;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;
		}
        case 0x8e: {
			uint16_t addr = (state->h << 8) | state->l;
			uint16_t x = (uint16_t) state->a + (uint16_t) state->memory[addr] + state->flags.cy;
			state->a = x & 0xff;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;
		} 
        case 0x8f: {	
			uint16_t x = (uint16_t) state->a + (uint16_t) state->a + state->flags.cy; 
			state->a = x & 0xff;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;
		}
        case 0x90: {
			uint16_t x = (uint16_t) state->a - (uint16_t) state->b;
			state->a = x & 0xff;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;	
		} 
        case 0x91: {
			uint16_t x = (uint16_t) state->a - (uint16_t) state->c;
			state->a = x & 0xff;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;	
		}
        case 0x92: {
			uint16_t x = (uint16_t) state->a - (uint16_t) state->d;
			state->a = x & 0xff;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;	
		}
        case 0x93: {
			uint16_t x = (uint16_t) state->a - (uint16_t) state->e;
			state->a = x & 0xff;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;	
		}
        case 0x94: {
			uint16_t x = (uint16_t) state->a - (uint16_t) state->h;
			state->a = x & 0xff;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;	
		}
        case 0x95: {
			uint16_t x = (uint16_t) state->a - (uint16_t) state->l;
			state->a = x & 0xff;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;	
		}
        case 0x96: {
			uint16_t addr = (state->h << 8) | state->l;
			uint16_t x = (uint16_t) state->a - (uint16_t) state->memory[addr];
			state->a = x & 0xff;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;
		} 
        case 0x97: {
			uint16_t x = (uint16_t) state->a - (uint16_t) state->a;
			state->a = x & 0xff;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;	
		} 
        case 0x98: {
			uint16_t x = (uint16_t) state->a - (uint16_t) state->b - state->flags.cy;
			state->a = x & 0xff;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;	
		}
        case 0x99: {
			uint16_t x = (uint16_t) state->a - (uint16_t) state->c - state->flags.cy;
			state->a = x & 0xff;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;	
		}
        case 0x9a: {
			uint16_t x = (uint16_t) state->a - (uint16_t) state->d - state->flags.cy;
			state->a = x & 0xff;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;	
		}
        case 0x9b: {
			uint16_t x = (uint16_t) state->a - (uint16_t) state->e - state->flags.cy;
			state->a = x & 0xff;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;	
		}
        case 0x9c: {
			uint16_t x = (uint16_t) state->a - (uint16_t) state->h - state->flags.cy;
			state->a = x & 0xff;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;	
		}
        case 0x9d: {
			uint16_t x = (uint16_t) state->a - (uint16_t) state->l - state->flags.cy;
			state->a = x & 0xff;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;	
		}
        case 0x9e: {
			uint16_t addr = (state->h << 8) | state->l;
			uint16_t x = (uint16_t) state->a - (uint16_t) state->memory[addr] - state->flags.cy;
			state->a = x & 0xff;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;	
		}
        case 0x9f: {
			uint16_t x = (uint16_t) state->a - (uint16_t) state->a - state->flags.cy;
			state->a = x & 0xff;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;	
		}
        case 0xa0: {
            uint8_t x = state->a & state->b;
            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.p = parity(x);
            state->flags.cy = 0;
            state->a = x;
            break;
		}
        case 0xa1: {
            uint8_t x = state->a & state->c;
            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.p = parity(x);
            state->flags.cy = 0;
            state->a = x;
            break;
		}
        case 0xa2: {
            uint8_t x = state->a & state->d;
            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.p = parity(x);
            state->flags.cy = 0;
            state->a = x;
            break;
		}
        case 0xa3: {
            uint8_t x = state->a & state->e;
            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.p = parity(x);
            state->flags.cy = 0;
            state->a = x;
            break;
		}
        case 0xa4: {
            uint8_t x = state->a & state->h;
            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.p = parity(x);
            state->flags.cy = 0;
            state->a = x;
            break;
		}
        case 0xa5: {
            uint8_t x = state->a & state->l;
            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.p = parity(x);
            state->flags.cy = 0;
            state->a = x;
            break;
		}
        case 0xa6: {
			uint16_t addr = (state->h << 8) | state->l;
            uint8_t x = state->a & state->memory[addr];
            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.p = parity(x);
            state->flags.cy = 0;
            state->a = x;
            break;
		}
        case 0xa7: {
            uint8_t x = state->a & state->a;
            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.p = parity(x & 0xff);
            state->flags.cy = 0;
            state->a = x;
            break;
        }
        case 0xa8: {
			uint8_t x = state->a ^ state->b;
			state->a = x;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.p = parity(x);
            state->flags.cy = 0; 
			break;
		}
        case 0xa9: {
			uint8_t x = state->a ^ state->c;
			state->a = x;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.p = parity(x);
            state->flags.cy = 0; 
			break;
		}
        case 0xaa: {
			uint8_t x = state->a ^ state->d;
			state->a = x;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.p = parity(x);
            state->flags.cy = 0; 
			break;
		} 
        case 0xab: {
			uint8_t x = state->a ^ state->e;
			state->a = x;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.p = parity(x);
            state->flags.cy = 0; 
			break;
		}
        case 0xac: {
			uint8_t x = state->a ^ state->h;
			state->a = x;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.p = parity(x);
            state->flags.cy = 0; 
			break;
		}
        case 0xad: {
			uint8_t x = state->a ^ state->l;
			state->a = x;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.p = parity(x);
            state->flags.cy = 0; 
			break;
		}
        case 0xae: {
			uint16_t addr = (state->h << 8) | state->l;
			uint8_t x = state->a ^ state->memory[addr];
			state->a = x;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.p = parity(x);
            state->flags.cy = 0; 
			break;
		}
        case 0xaf: {
            uint8_t x = state->a ^ state->a;
            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
			state->flags.p = parity(x);
            state->flags.cy = 0;
            state->a = x;
            break;
        }
        case 0xb0: {
			uint8_t x = state->a | state->b;
			state->a = x;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.p = parity(x);
			state->flags.cy = 0;
			break;
		}
        case 0xb1: {
			uint8_t x = state->a | state->c;
			state->a = x;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.p = parity(x);
			state->flags.cy = 0;
			break;
		}
        case 0xb2: {
			uint8_t x = state->a | state->d;
			state->a = x;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.p = parity(x);
			state->flags.cy = 0;
			break;
		}
        case 0xb3: {
			uint8_t x = state->a | state->e;
			state->a = x;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.p = parity(x);
			state->flags.cy = 0;
			break;
		}
        case 0xb4: {
			uint8_t x = state->a | state->h;
			state->a = x;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.p = parity(x);
			state->flags.cy = 0;
			break;
		}
        case 0xb5: {
			uint8_t x = state->a | state->l;
			state->a = x;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.p = parity(x);
			state->flags.cy = 0;
			break;
		}
        case 0xb6: {
			uint16_t addr = (state->h << 8) | state->l;
			uint8_t x = state->a | state->memory[addr];
			state->a = x;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.p = parity(x);
			state->flags.cy = 0;
			break;
		}
        case 0xb7: {
			uint8_t x = state->a | state->a;
			state->a = x;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.p = parity(x);
			state->flags.cy = 0;
			break;
		}
        case 0xb8: {
			uint16_t x = (uint16_t) state->a - (uint16_t) state->b; 

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;
		}
        case 0xb9: {
			uint16_t x = (uint16_t) state->a - (uint16_t) state->c; 

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;
		}
        case 0xba: {
			uint16_t x = (uint16_t) state->a - (uint16_t) state->d; 

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;
		}
        case 0xbb: {
			uint16_t x = (uint16_t) state->a - (uint16_t) state->e; 

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;
		}
        case 0xbc: {
			uint16_t x = (uint16_t) state->a - (uint16_t) state->h; 

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;
		} 
        case 0xbd: {
			uint16_t x = (uint16_t) state->a - (uint16_t) state->l; 

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;
		}
        case 0xbe: {
			uint16_t addr = (state->h << 8) | state->l;
			uint16_t x = (uint16_t) state->a - (uint16_t) state->memory[addr]; 

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;
		}
        case 0xbf: {
			uint16_t x = (uint16_t) state->a - (uint16_t) state->a; 

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);
			break;
		}
		case 0xc0:
			if(state->flags.z == 0) {
				state->pc = (state->memory[state->sp+1] << 8) | state->memory[state->sp];	
				state->sp += 2;
				state->cycles += 6;		
			}
			break;
        case 0xc1: {
            state->c = state->memory[state->sp];
            state->b = state->memory[state->sp+1];
            state->sp += 2;
            break;
        }
        case 0xc2:
            if(state->flags.z == 0) 
                state->pc = (opcode[2] << 8) | opcode[1];
            else
                state->pc += 2;
            break;
        case 0xc3:
            state->pc = (opcode[2] << 8) | opcode[1];
			break;
        case 0xc4: {
			if(state->flags.z == 0) {
				uint16_t ret = state->pc + 2;
				
				state->memory[state->sp-1] = (ret >> 8) & 0xff;
				state->memory[state->sp-2] = ret & 0xff;
				state->sp -= 2;

				state->pc = (opcode[2] << 8) | opcode[1];

				state->cycles += 6;
			} else {
				state->pc += 2;
			}
			break;
		}
        case 0xc5: {
            state->memory[state->sp-1] = state->b;
            state->memory[state->sp-2] = state->c;
            state->sp -= 2;
            break;
        }
        case 0xc6: {
            uint16_t result = (uint16_t) state->a + (uint16_t) opcode[1];
            state->flags.z = ((result & 0xff) == 0);
            state->flags.s = ((result & 0x80) != 0);
            state->flags.cy = (result > 0xff);
            state->flags.p = parity(result & 0xff);
            state->a = result & 0xff;
            state->pc += 1;
            break;
        }
        case 0xc7: {
			uint16_t ret = state->pc + 2;

			state->memory[state->sp-1] = (ret >> 8) & 0xff;
			state->memory[state->sp-2] = ret & 0xff;
			state->sp -= 2;

			state->pc = 0x0000;
			break;
		}
        case 0xc8: 
			if(state->flags.z) {
				state->pc = (state->memory[state->sp+1] << 8) | state->memory[state->sp];	
				state->sp += 2;
				state->cycles += 6;		
			}
			break;
        case 0xc9: {
            state->pc = (state->memory[state->sp+1] << 8) | state->memory[state->sp];
            state->sp += 2;
			break;
        }
        case 0xca:
			if(state->flags.z)
				state->pc = (opcode[2] << 8) | opcode[1];
			else
				state->pc += 2;
			break;
        case 0xcb: break;
        case 0xcc: {
			if(state->flags.z == 1) {
				uint16_t ret = state->pc + 2;
				
				state->memory[state->sp-1] = (ret >> 8) & 0xff;
				state->memory[state->sp-2] = ret & 0xff;
				state->sp -= 2;

				state->pc = (opcode[2] << 8) | opcode[1];

				state->cycles += 6;
			} else {
				state->pc += 2;
			}
			break;
		} 
        case 0xcd:
			/*if(((opcode[2] << 8) | opcode[1]) == 5) {
				if(state->c == 9) {
					uint16_t offset = (state->d << 8) | state->e;
					unsigned char *str = &state->memory[offset+3];
					
					while(*str != '$')
						printf("%c", *str++);

					printf("\n");
				} else if(state->c == 2) {
					printf("print char routine called\n");
				}
			} else if(((opcode[2] << 8) | opcode[1]) == 0) {
				exit(0);
			} else*/ {
				uint16_t ret = state->pc + 2;
				state->memory[state->sp - 1] = (ret >> 8) & 0xff;
				state->memory[state->sp - 2] = ret & 0xff;
				state->sp -= 2;
				state->pc = (opcode[2] << 8) | opcode[1];
			}
			break;
        case 0xce: {
			uint16_t x = (uint16_t) state->a + opcode[1] + state->flags.cy;
			state->a = x & 0xff;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) != 0);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);

			state->pc++;
			break;
		} 
        case 0xcf: {
			uint16_t ret = state->pc + 2;

			state->memory[state->sp-1] = (ret >> 8) & 0xff;
			state->memory[state->sp-2] = ret & 0xff;
			state->sp -= 2;

			state->pc = 0x0008;
			break;
		}
        case 0xd0: {
			if(state->flags.cy == 0) {
				state->pc = (state->memory[state->sp+1] << 8) | state->memory[state->sp];
				state->sp += 2;	

				state->cycles += 6;
			}
			break;
		}
        case 0xd1: {
            state->e = state->memory[state->sp];
            state->d = state->memory[state->sp+1];
            state->sp += 2;
            break;
        }
        case 0xd2: 
			if(state->flags.cy == 0)
				state->pc = (opcode[2] << 8) | opcode[1];
			else
				state->pc += 2;
			break;
        case 0xd3: break;
        case 0xd4: {
			if(state->flags.cy == 0) {
				uint16_t ret = state->pc + 2;

				state->memory[state->sp-1] = (ret >> 8) & 0xff;
				state->memory[state->sp-2] = ret & 0xff;
				state->sp -= 2;

				state->pc = (opcode[2] << 8) | opcode[1];

				state->cycles += 6;
			} else {
				state->pc += 2;
			}	
			break;
		}
        case 0xd5: {
            state->memory[state->sp-1] = state->d;
            state->memory[state->sp-2] = state->e;
            state->sp -= 2;
            break;
        }
        case 0xd6: {
			uint8_t x = state->a - opcode[1];

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) != 0);
            state->flags.p = parity(x);
            state->flags.cy = (state->a < opcode[1]);

			state->a = x;
			state->pc++;
			break;
		} 
        case 0xd7: {
			uint16_t ret = state->pc + 2;

			state->memory[state->sp-1] = (ret >> 8) & 0xff;
			state->memory[state->sp-2] = ret & 0xff;
			state->sp -= 2;

			state->pc = 0x10;
			break;
		}
        case 0xd8: {
			if(state->flags.cy != 0) {
				state->pc = (state->memory[state->sp+1] << 8) | state->memory[state->sp];	
				state->sp += 2;
				state->cycles += 6;		
			}
			break;
		}
        case 0xd9: break;
        case 0xda: {
			if(state->flags.cy != 0)
				state->pc = (opcode[2] << 8) | opcode[1];
			else
				state->pc += 2;
			break;
		}
		case 0xdb: break;
        case 0xdc: { 
			if(state->flags.cy != 0) {
				uint16_t ret = state->pc + 2;
				state->memory[state->sp - 1] = (ret >> 8) & 0xff;
				state->memory[state->sp - 2] = ret & 0xff;
				state->sp -= 2;
				state->pc = (opcode[2] << 8) | opcode[1];
			} else {
				state->pc += 2;
			}
			break;
		}
        case 0xdd: break;
        case 0xde: {
			uint16_t x = (uint16_t) state->a - opcode[1] - state->flags.cy;
			state->a = x & 0xff;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) != 0);
            state->flags.cy = (x > 0xff);
            state->flags.p = parity(x & 0xff);

			state->pc++;
			break;
		}
        case 0xdf: {
			uint16_t ret = state->pc + 2;

			state->memory[state->sp-1] = (ret >> 8) & 0xff;
			state->memory[state->sp-2] = ret & 0xff;
			state->sp -= 2;

			state->pc = 0x18;
			break;
		}
        case 0xe0: {
			if(state->flags.p == 0) {
				state->pc = (state->memory[state->sp+1] << 8) | state->memory[state->sp];	
				state->sp += 2;
				state->cycles += 6;		
			}
			break;
		}
        case 0xe1: {
            state->l = state->memory[state->sp];
            state->h = state->memory[state->sp+1];
            state->sp += 2;
            break;
        }
        case 0xe2:
			if(state->flags.p == 0)
				state->pc = (opcode[2] << 8) | opcode[1];
			else
				state->pc += 2;
			break;
        case 0xe3: {
            uint8_t temp;

            temp = state->l;
            state->l = state->memory[state->sp];
            state->memory[state->sp] = temp;

            temp = state->h;
            state->h = state->memory[state->sp+1];
            state->memory[state->sp+1] = temp;
            break;
        }
        case 0xe4: {
			if(state->flags.p == 0) {
				uint16_t ret = state->pc + 2;
				
				state->memory[state->sp-1] = (ret >> 8) & 0xff;
				state->memory[state->sp-2] = ret & 0xff;
				state->sp -= 2;

				state->pc = (opcode[2] << 8) | opcode[1];
				
				state->cycles += 6;
			} else {
				state->pc += 2;
			}
			break;
		}
        case 0xe5: {
            state->memory[state->sp-1] = state->h;
            state->memory[state->sp-2] = state->l;
            state->sp -= 2;
            break;
        }
        case 0xe6: {
            uint8_t result = state->a & opcode[1];
            state->flags.z = (result == 0x00);
            state->flags.s = ((result & 0x80) == 0x80);
            state->flags.p = parity(result);
            state->flags.cy = 0;
            state->a = result;
            state->pc++;
            break;
        }
        case 0xe7: {
			uint16_t ret = state->pc + 2;

			state->memory[state->sp-1] = (ret >> 8) & 0xff;
			state->memory[state->sp-2] = ret & 0xff;
			state->sp -= 2;

			state->pc = 0x20;
			break;
		}
        case 0xe8: {
			if(state->flags.p != 0) {
				state->pc = (state->memory[state->sp+1] << 8) | state->memory[state->sp];	
				state->sp += 2;
				state->cycles += 6;		
			}
			break;
		}
        case 0xe9: state->pc = (state->h << 8) | state->l; break;
        case 0xea:
			if(state->flags.p == 1)
				state->pc = (opcode[2] << 8) | opcode[1];
			else
				state->pc += 2;
			break;
        case 0xeb: {
			uint8_t temp1 = state->d;
			uint8_t temp2 = state->e;	
			state->d = state->h;
			state->e = state->l;
			state->h = temp1;
			state->l = temp2;
			break;
		}
        case 0xec: {
			if(state->flags.p != 0) {
				uint16_t ret = state->pc + 2;
				
				state->memory[state->sp-1] = (ret >> 8) & 0xff;
				state->memory[state->sp-2] = ret & 0xff;
				state->sp -= 2;

				state->pc = (opcode[2] << 8) | opcode[1];
				
				state->cycles += 6;
			} else {
				state->pc += 2;
			}
			break;
		} 
        case 0xed: break;
        case 0xee: {
			uint8_t x = state->a ^ opcode[1];
			state->a = x;

            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) != 0);
            state->flags.p = parity(x);
			state->flags.cy = 0;

			state->pc++;
			break;
		}
        case 0xef: {
			uint16_t ret = state->pc + 2;

			state->memory[state->sp-1] = (ret >> 8) & 0xff;
			state->memory[state->sp-2] = ret & 0xff;
			state->sp -= 2;

			state->pc = 0x28;
			break;
		}
        case 0xf0: {
			if(state->flags.s == 0) {
				state->pc = (state->memory[state->sp+1] << 8) | state->memory[state->sp];	
				state->sp += 2;
				state->cycles += 6;		
			}
			break;
		}
        case 0xf1: {
            state->a = state->memory[state->sp+1];
            uint8_t psw = state->memory[state->sp];
            state->flags.z = ((psw & 0x01) == 0x01);
            state->flags.s = ((psw & 0x02) == 0x02);
            state->flags.p = ((psw & 0x04) == 0x04);
            state->flags.cy = ((psw & 0x08) == 0x08);
            state->flags.ac = ((psw & 0x10) == 0x10);
            state->sp += 2;
            break;
        }
        case 0xf2: 
			if(state->flags.s == 0)
				state->pc = (opcode[2] << 8) | opcode[1];
			else
				state->pc += 2;
			break;
        case 0xf3: state->int_enable = 0; break;
        case 0xf4: {
			if(state->flags.s == 0) {
				uint16_t ret = state->pc + 2;

				state->memory[state->sp-1] = ((ret >> 8) & 0xff);
				state->memory[state->sp-2] = ret & 0xff;
				state->sp -= 2;

				state->pc = (opcode[2] << 8) | opcode[1];
				
				state->cycles += 6;
			} else {
				state->pc += 2;
			}
			break;
		}
        case 0xf5: {
            state->memory[state->sp-1] = state->a;
            uint8_t psw = (state->flags.z |
                state->flags.s << 1 |
                state->flags.p << 2 |
                state->flags.cy << 3 |
                state->flags.ac << 4);
            state->memory[state->sp-2] = psw;
            state->sp -= 2;
            break;
        }
        case 0xf6: {
			uint8_t x = state->a | opcode[1];
			state->a = x;

			state->flags.z = ((x & 0xff) == 0);
			state->flags.s = ((x & 0x80) == 0x80);
			state->flags.p = parity(x);
			state->flags.cy = 0;

			state->pc++;	
			break;
		}
        case 0xf7: {
			uint16_t ret = state->pc + 2;

			state->memory[state->sp-1] = (ret >> 8) & 0xff;
			state->memory[state->sp-2] = ret & 0xff;
			state->sp -= 2;

			state->pc = 0x30;
			break;
		}
        case 0xf8: {
			if(state->flags.s != 0) {
				state->pc = (state->memory[state->sp+1] << 8) | state->memory[state->sp];
				state->sp += 2;

				state->cycles += 6;
			}	
			break;
		}
        case 0xf9: state->sp = (state->h << 8) | (state->l); break;
        case 0xfa: 
			if(state->flags.s == 1)
				state->pc = (opcode[2] << 8) | opcode[1];
			else
				state->pc += 2;
			break;
        case 0xfb: state->int_enable = 1; break;
        case 0xfc: {
			if(state->flags.s != 0) {
				uint16_t ret = state->pc + 2;

				state->memory[state->sp-1] = ((ret >> 8) & 0xff);
				state->memory[state->sp-2] = ret & 0xff;
				state->sp -= 2;

				state->pc = (opcode[2] << 8) | opcode[1];
				
				state->cycles += 6;
			} else {
				state->pc += 2;
			}
			break;
		} 
        case 0xfd: break;
        case 0xfe: {
            uint16_t x = state->a - opcode[1];
            state->flags.z = ((x & 0xff) == 0);
            state->flags.s = ((x & 0x80) == 0x80);
            state->flags.p = parity(x & 0xff);
            state->flags.cy = (x > 0xff);
            state->pc++;
            break;
        }
        case 0xff: {
			uint16_t ret = state->pc + 2;

			state->memory[state->sp-1] = (ret >> 8) & 0xff;
			state->memory[state->sp-2] = ret & 0xff;
			state->sp -= 2;

			state->pc = 0x38;
			break;
		}
    }
}

void generate_interrupt(State8080* state, int int_num) {
	//push pc
	state->memory[state->sp-1] = (state->pc >> 8) & 0xff;
	state->memory[state->sp-2] = (state->pc & 0xff);
	state->sp -= 2;

	state->pc = 8 * int_num;

	state->int_enable = 0;
}

/*int main(int argc, char** argv) {*/
	/*assert(argc == 2);*/
    /*FILE *file = fopen(argv[1], "rb");*/

    /*if(file == NULL) {*/
        /*printf("Error: Cannot open %s!\n", argv[1]);*/
        /*exit(1);*/
    /*}*/

    /*fseek(file, 0L, SEEK_END);*/
    /*long fsize = ftell(file);*/
    /*fseek(file, 0L, SEEK_SET);*/

	/*State8080 *state = init_8080();*/
    /*unsigned char *buffer = &state->memory[0x100]; */

    /*fread(buffer, fsize, 1, file);*/
    /*fclose(file);*/

	/*//First instruction => JMP 0x100*/
	/*[>buffer[0] = 0xc3;<]*/
	/*[>buffer[1] = 0x00;<]*/
	/*[>buffer[2] = 0x01;<]*/
	/*srand(time(0));*/

	/*state->pc = 0x100;*/

	/*state->memory[5] = 0xc9;*/
	
	/*[>state->memory[368] = 0x7;<]*/

	/*[>state->memory[0x59c] = 0xc3;<]*/
	/*[>state->memory[0x59d] = 0xc2;<]*/
	/*[>state->memory[0x59e] = 0x05;<]*/

    /*i8080 cpu;*/
    /*cpu.read_byte = rb;*/
    /*cpu.write_byte = wb;*/
    /*cpu.userdata = NULL;*/

    /*init(&cpu);*/
    /*memset(memory, 0, sizeof(memory));*/
    /*load_file_into_memory("TST8080.COM", 0x100);*/

	/*cpu.pc = 0x100; // the test roms all start at 0x100*/
    /*cpu.write_byte(cpu.userdata, 5, 0xC9); // inject RET at 0x5 to handle "CALL 5", needed*/
                          /*// for the test roms*/

	/*while(1) {*/
		/*if(state->memory[state->pc] == 0x76) {*/
			/*printf("HLT at %04x\n", state->pc);*/
		/*}*/

		/*if(state->pc == 5) {*/
			/*if(state->c == 9) {*/
				/*uint16_t de = (state->d << 8) | state->e;*/
				/*unsigned char* str = &state->memory[de];*/
				
				/*do {*/
					/*printf("%c", *str++);*/
				/*} while(*str != 0x24);*/
			/*}*/

			/*if(state->c == 2) {*/
				/*printf("%c", state->e);*/
			/*}*/
		/*}*/

		/*uint16_t prev_pc = state->pc;*/
		/*//parse_opcode(state->memory, state->pc);*/
			
		/*emulate_opcode(state);*/
        /*testrom(&cpu);*/

		/*//debug_output(&cpu);*/
		/*//show_state(state);*/

		/*if(state->a != cpu.a |*/
				/*state->b != cpu.b |*/
				/*state->c != cpu.c |*/
				/*state->d != cpu.d |*/
				/*state->e != cpu.e |*/
				/*state->h != cpu.h |*/
				/*state->l != cpu.l |*/
				/*[>state->pc != cpu.pc |<]*/
				/*state->sp != cpu.sp |*/
				/*state->int_enable != cpu.iff |*/
				/*state->flags.z != (int) cpu.z |*/
				/*state->flags.s != (int) cpu.s |*/
				/*state->flags.p != (int) cpu.p |*/
				/*state->flags.cy != (int) cpu.cy) {*/

			/*[>parse_opcode(state->memory, prev_pc);<]*/
			/*[>printf("*");<]*/
			/*[>debug_output(&cpu);<]*/
			/*[>show_state(state);<]*/
			/*break;*/
		/*}	*/
		

		/*if(state->pc == 0) {*/
			/*printf("\nJumped to 0x0000 from %04x\n", prev_pc);*/
			/*break;*/
		/*}*/
	/*}*/

	/*//Fix stack pointer*/
	/*[>state->memory[368] = 0x7;<]*/

	/*//Skip DAA test*/
	/*[>buffer[0x59c] = 0xc3;<]*/
	/*[>buffer[0x59d] = 0xc2;<]*/
	/*[>buffer[0x59e] = 0x05;<]*/

    /*[>while(state->pc < fsize) {<]*/
        /*[>emulate_opcode(state);<]*/
    /*[>}<]*/

	/*[>free(state->memory);<]*/
	/*[>free(state);<]*/
    /*return 0;*/
/*}*/
