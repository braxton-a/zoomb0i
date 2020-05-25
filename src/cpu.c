#include "cpu.h"
#include "utils.h"

//extern void sayHi(void);
//#define DEBUG

void _halt(gb_cpu_t *cpu) {
	if (cpu->ime) {
		cpu->shouldHalt = true;
	} else {
		if ((*cpu->ie & *cpu->ifl) != 0) {
			cpu->halt_bug = true;
		} else {
			cpu->shouldHalt = true;
		}
	}

}

void queue_task(gb_cpu_t *cpu, uint8_t bit, uint8_t val, uint8_t cycles) {
	if (cpu->queued_tasks_data_ptr < 31) {
		cpu->queued_tasks_data_ptr++;
	} else {
		printf("queued tasks full");
		return;
	}
		
	struct queue_task_data *task_data = malloc(sizeof(struct queue_task_data));
	task_data->data = val;
	task_data->cycles_until_execution = cycles;
	cpu->queued_tasks_data[cpu->queued_tasks_data_ptr] = task_data;

	cpu->queued_tasks |= 1UL << bit;
}

static inline void set_flag(gb_cpu_t *cpu, uint8_t bit, uint8_t status) {
	cpu->f ^= (-status ^ cpu->f) & (1UL << bit);
}

static inline uint8_t get_flag_on(gb_cpu_t *cpu, uint8_t bit) {
	return (cpu->f >> bit) & 1;
}

static inline void set_bit_on(uint8_t *value, uint8_t bit, uint8_t status) {
	*value ^= (-status ^ *value) & (1UL << bit);
}

static inline uint8_t get_bit_on(uint8_t value, uint8_t bit) {
	return (value >> bit) & 1;
}

void set_interrupt_flag(gb_cpu_t *cpu, uint8_t bit, uint8_t status) {
	*cpu->ifl ^= ((-status ^ *cpu->ifl) & (1UL << bit));
}

/*
static void attempt_write(gb_cpu_t *cpu, uint8_t *dest, uint8_t *src) {
	static uint16_t unallowed_writes[][2] = {
		{0xFEA0, 0xFEFF},
		{0xE000, 0xFDFF},
		{0x0000, 0x7FFF},
	};

	for (int i = 0; i < sizeof(unallowed_writes)/sizeof(unallowed_writes[0]); i++) {
		if (dest - cpu->addressSpace >= unallowed_writes[i][0] && dest - cpu->addressSpace <= unallowed_writes[i][1]) {
			return;
		}
	}
	*dest = *src;
}*/


inline uint8_t *get_ptr_to(gb_cpu_t *cpu, uint16_t address) {
	return cpu->address_space + address;
}

uint8_t gb_read(gb_cpu_t *cpu, uint16_t address) {
	uint8_t value;//cpu->address_space[address];
	
	switch (address & 0xF000) {
		case 0x0000:
		case 0x1000:
		case 0x2000:
		case 0x3000: {
			value = cpu->address_space[address];
			break;
		}

		case 0x4000:
		case 0x5000:
		case 0x6000:
		case 0x7000: {
			value = cpu->address_space[address];
			break;
		}

		case 0x8000:
		case 0x9000: {
			value = cpu->address_space[address];
			break;
		}

		case 0xA000:
		case 0xB000: {
			value = cpu->address_space[address];
			break;
		}

		case 0xC000:
		case 0xD000: {
			value = cpu->address_space[address];
			break;
		}

		case 0xF000: {
			switch (address & 0x0F00) {
				case 0x0E00: {
					if (address < 0xFEA0) {
						value = cpu->address_space[address];
					} else {
						value = 0;
					}
					break;
				}
				case 0x0F00: {
					if (address < 0xFF7F) {
						value = cpu->address_space[address];
					} else if (address < 0xFFFF) {
						value = cpu->address_space[address];
					} else {
						value = *cpu->ie;
						//value = cpu->ime;
					}
					break;
				}
			}
			break;
		}
		default:
			value = 0;
			break;
	}

	return value;
}

void gb_write(gb_cpu_t *cpu, uint16_t address, uint8_t data, uint8_t size) {
	switch (address & 0xF000) {
		case 0x0000:
		case 0x1000: {
			break;
		}
		case 0x2000:
		case 0x3000: {
			//do nothing as writing to ROM isnt allowed
			if (cpu->mbc1) {
				load_rom_bank(cpu, data);
			}
			break;
		}

		case 0x4000:
		case 0x5000:
		case 0x6000:
		case 0x7000: {
			//do nothing as writing to ROM isnt allowed
			break;
		}

		case 0x8000:
		case 0x9000: {

			//if (cpu->ppu->mode == 3)
				//return;
			cpu->address_space[address] = data;
			break;
		}

		case 0xA000:
		case 0xB000: {
			cpu->address_space[address] = data;
			break;
		}

		case 0xC000:
		case 0xD000: {
			// vram

			cpu->address_space[address] = data;
			break;
		}

		case 0xF000: {
			switch (address & 0x0F00) {
				case 0x0E00: {
					// OAM
					if (address < 0xFFA0) {
						cpu->address_space[address] = data;
					}
					break;
				}
				case 0x0F00: {
					if (address < 0xFF7F) {
						cpu->address_space[address] = data;
						switch (address) {
							case 0xFF04: {
								*cpu->div = 0;
								break;
							}
							case 0xFF41: {
								*cpu->stat &= 0b11111000;
								*cpu->stat |= data;
								break;
							}
							case 0xFF46: {
								//printf("[DEBUG]: Queueing dma transfer with base address: 0x%04x\n", data * 0x100);
								queue_task(cpu, QUEUE_DMA_TRANSFER, data, 1);
								break;
							}
						}
					} else if (address < 0xFFFF) {
						cpu->address_space[address] = data;
					} else {
						*cpu->ie = data;
					}
					break;
				}
			}
			break;
		}
	}
	end:
		NULL;
	//cpu->address_space[address] = data;
}
/*
static void gb_write_byte(gb_cpu_t *cpu, uint8_t *address, uint8_t data) {

}

static void gb_write_word(gb_cpu_t *cpu, uint16_t *address, uint16_t data) {

}
*/
void _push(gb_cpu_t *cpu, uint16_t *wreg) {
	cpu->sp -= 2;
	uint8_t top = (*wreg >> 8) & 0xff;
	uint8_t bottom = *wreg & 0xff;

	gb_write(cpu, cpu->sp, bottom, 1);
	gb_write(cpu, cpu->sp + 1, top, 1);
}

static void _pop(gb_cpu_t *cpu, uint16_t *wreg) {
	uint16_t newt = 0;
	newt = gb_read(cpu, cpu->sp + 1);
	newt <<= 8;
	newt |= gb_read(cpu, cpu->sp);
	if (wreg == cpu->af) {
		*wreg = newt & 0xFFF0;
	} else {
		*wreg = newt;
	}
	cpu->sp += 2;
}

static void _inc(gb_cpu_t *cpu, uint8_t *orig) {
	uint16_t val = *orig + 1;
	if (get_bit_on(val, 8)) {
		set_flag(cpu, C_FLAG, 1);
	} else {
		set_flag(cpu, C_FLAG, 0);
	}
}

static void _ret(gb_cpu_t *cpu) {
	_pop(cpu, &cpu->pc);
}

void dma_transfer(gb_cpu_t *cpu, uint8_t addr) {
	//#ifdef DEBUG
	//printf("[DEBUG]: DMA Transfer Called with base address: %x", addr * 0x100);
	//#endif

	//Check for vblank
	if (cpu->ppu->mode == 1) {
		uint8_t *finalAddr = cpu->address_space + addr * 0x100;
		/*for (uint8_t i = 0; i < 0xA0; i++, finalAddr++) {
			cpu->address_space[0xFE00 + i] = *finalAddr;
		}*/
		memcpy(cpu->address_space + 0xFE00, finalAddr, 0x9E);
		//cpu->instruction_wait_cycles = 671;
	}
}

/*
static void attempt_write_word(gb_cpu_t *cpu, uint8_t *dest, uint8_t *src) {
	static uint16_t unallowed_writes[][2] = {
		{0xFEA0, 0xFEFF},
		{0xE000, 0xFDFF},
		{0x0000, 0x7FFF},
	};

	if ((*cpu->stat & 0b11) == 3 && dest - cpu->addressSpace >= 0x8000 && dest - cpu->addressSpace <= 0x9FFF) {
		return;
	}
	for (int i = 0; i < sizeof(unallowed_writes) / sizeof(unallowed_writes[0]); i++) {
		if (dest - cpu->addressSpace >= unallowed_writes[i][0] && dest - cpu->addressSpace <= unallowed_writes[i][1]) {
			return;
		}
	}
	*dest = *src;
}*/

void init_cpu(gb_cpu_t *cpu, gb_ppu_t *ppu, bool load_bootstrap) {

	cpu->queued_tasks_data_ptr = 0;

	cpu->load_bootstrap = load_bootstrap;
	cpu->ppu = ppu;
	cpu->joyp = get_ptr_to(cpu, 0xFF00);
	cpu->sb = get_ptr_to(cpu, 0xFF01);
	cpu->sc = get_ptr_to(cpu, 0xFF02);
	cpu->div = get_ptr_to(cpu, 0xFF04);
	cpu->tima = get_ptr_to(cpu, 0xFF05);
	cpu->tma = get_ptr_to(cpu, 0xFF06);
	cpu->tac = get_ptr_to(cpu, 0xFF07);
	cpu->lcdc = get_ptr_to(cpu, 0xFF40);
	cpu->stat = get_ptr_to(cpu, 0xFF41);
	cpu->scy = get_ptr_to(cpu, 0xFF42);
	cpu->scx = get_ptr_to(cpu, 0xFF43);
	cpu->ly = get_ptr_to(cpu, 0xFF44);
	cpu->lyc = get_ptr_to(cpu, 0xFF45);
	cpu->dma = get_ptr_to(cpu, 0xFF46);
	cpu->bgp = get_ptr_to(cpu, 0xFF47);
	cpu->obp0 = get_ptr_to(cpu, 0xFF48);
	cpu->obp1 = get_ptr_to(cpu, 0xFF49);
	cpu->wy = get_ptr_to(cpu, 0xFF4A);
	cpu->wx = get_ptr_to(cpu, 0xFF4B);
	cpu->ifl = get_ptr_to(cpu, 0xFF0F);
	cpu->ie = get_ptr_to(cpu, 0xFFFF);
	cpu->oam = get_ptr_to(cpu, 0xFE00);
	cpu->vram = get_ptr_to(cpu, 0x8000);
	cpu->ram = get_ptr_to(cpu, 0xC000);
	*cpu->stat = 2;

	*cpu->ifl = 0;
	cpu->instruction_wait_cycles = 0;
	cpu->run_mode = 0;

	if (cpu->load_bootstrap)
		cpu->pc = 0x0;
	else
		cpu->pc = 0x100;

	uint8_t *registers[] = {&cpu->a, &cpu->f, &cpu->b, &cpu->c, &cpu->d, &cpu->e, &cpu->h, &cpu->l};
	cpu->af = (uint16_t *)&cpu->f;
	cpu->bc = (uint16_t *)&cpu->c;
	cpu->de = (uint16_t *)&cpu->e;
	cpu->hl = (uint16_t *)&cpu->l;
	uint8_t *cbreg[] = {&cpu->b, &cpu->c, &cpu->d, &cpu->e, &cpu->h, &cpu->l, 0, &cpu->a};
	for (int i = 0; i < 8; i++) {
		*registers[i] = 0;
		cpu->registers[i] = cbreg[i];
	}

	// Final values after bootstrap
	if (!cpu->load_bootstrap) {
		cpu->sp = 0xfffe;
		*cpu->af = 0x01b0;
		*cpu->bc = 0x0013;
		*cpu->de = 0x00d8;
		*cpu->hl = 0x014d;
	}

	cpu->halt_bug = false;
	cpu->service_interrupts = true;
}

static void _ram_dump(gb_cpu_t *cpu) {
	FILE *dump = fopen("ramdump.bin", "wb+");
	for (int i = 0; i < 0x10000; i++) {
		fwrite(cpu->address_space + i, sizeof(uint8_t), 1, dump);
	}
	fclose(dump);
}

uint32_t fetch_opcode(gb_cpu_t *cpu) {

	/* 0 is either a nonexistant opcode or it indicates 
	an opcode with a variable length cycle count, 
	which is handled in the instruction implementation instead */

	static uint8_t instruction_cycle_count[16][16] = {
		//0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
		{4, 12, 8, 8, 4, 4, 8, 4, 20, 8, 8, 8, 4, 4, 8, 4},
		{4, 12, 8, 8, 4, 4, 8, 4, 12, 8, 8, 8, 4, 4, 8, 4},
		{0, 12, 8, 8, 4, 4, 8, 4, 0, 8, 8, 8, 4, 4, 8, 4},
		{0, 12, 8, 8, 12, 12, 12, 4, 0, 8, 8, 8, 4, 4, 8, 4},
		{4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4},
		{4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4},
		{4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4},
		{8, 8, 8, 8, 8, 8, 4, 8, 4, 4, 4, 4, 4, 4, 8, 4},
		{4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4},
		{4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4},
		{4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4},
		{4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4},
		{0, 12, 0, 16, 0, 16, 8, 16, 0, 16, 0, 4, 0, 24, 8, 16},
		{0, 12, 0, 0, 0, 16, 8, 16, 0, 16, 0, 0, 0, 0, 8, 16},
		{12, 12, 8, 0, 0, 16, 8, 16, 16, 4, 16, 0, 0, 0, 8, 16},
		{12, 12, 8, 4, 0, 16, 8, 16, 12, 8, 16, 4, 0, 0, 8, 16}
	};

	static uint8_t instruction_byte_size[16][16] = {
		//0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
		{1, 3, 1, 1, 1, 1, 2, 1, 3, 1, 1, 1, 1, 1, 2, 1},
		{1, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1},
		{2, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1},
		{2, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 3, 3, 3, 1, 2, 1, 1, 1, 3, 2, 3, 3, 2, 1},
		{1, 1, 3, 0, 3, 1, 2, 1, 1, 1, 3, 0, 3, 0, 2, 1},
		{2, 1, 1, 0, 0, 1, 2, 1, 2, 1, 3, 0, 0, 0, 2, 1},
		{2, 1, 1, 1, 0, 1, 2, 1, 2, 1, 3, 1, 0, 0, 2, 1}
	};

	uint8_t opcode = cpu->address_space[cpu->pc];

	uint8_t instruction_size = *((uint8_t *)&instruction_byte_size + opcode);
	cpu->instruction_wait_cycles = *((uint8_t *)&instruction_cycle_count + opcode);

	uint32_t final = opcode << 24;

	if (instruction_size > 1) {
		if (instruction_size == 3)
			final |= (cpu->address_space[cpu->pc + 2] << 16 | cpu->address_space[cpu->pc + 1] << 8);
		else
			final |= cpu->address_space[cpu->pc + 1] << 16;
	}
	if (!cpu->halt_bug)
		cpu->pc += instruction_size;
	else
		cpu->halt_bug = false;

	return final;
}
#ifdef DEBUG
static char *_assembly_translation[16][16] = {
	{"nop", "ld bc, d16", "ld (bc), a", "inc bc", "inc b", "dec b", "ld b, d8", "rlca", "ld (a16), sp", "add hl, bc", "ld a, (bc)", "dec bc", "inc c", "dec c", "ld c, d8", "rrca"},
	{"stop", "ld de, d16", "ld (de), a", "inc de", "inc d", "dec d", "ld d, d8", "rla", "jr r8", "add hl, de", "ld a, (de)", "dec de", "inc e", "dec e", "ld e, d8", "rra"},
	{"jr nz, r8", "ld hl, d16", "ld (hl+), a", "inc hl", "inc h", "dec h", "ld h, d8", "daa", "jr z, r8", "add hl, hl", "ld a, (hl+)", "dec hl", "inc l", "dec l", "ld l, d8", "cpl"},
	{"jr nc, r8", "ld sp, d16", "ld (hl-), a", "inc sp", "inc (hl)", "dec (hl)", "ld (hl), d8", "scf", "jr c, r8", "add hl, sp", "ld a, (hl-)", "dec sp", "inc a", "dec a", "ld a, d8", "ccf"},
	{"ld b, b", "ld b, c", "ld b, d", "ld b, e", "ld b, h", "ld b, l", "ld b, (hl)", "ld b, a", "ld c, b", "ld c, c", "ld c, d", "ld c, e", "ld c, h", "ld c, l", "ld c, (hl)", "ld b, a"},
	{"ld d, b", "ld d, c", "ld d, d", "ld d, e", "ld d, h", "ld d, l", "ld d, (hl)", "ld d, a", "ld e, b", "ld e, c", "ld e, d", "ld e, e", "ld e, h", "ld e, l", "ld e, (hl)", "ld e, a"},
	{"ld h, b", "ld h, c", "ld h, d", "ld h, e", "ld h, h", "ld h, l", "ld h, (hl)", "ld h, a", "ld l, b", "ld l, c", "ld l, d", "ld l, e", "ld l, h", "ld l, l", "ld l, (hl)", "ld l, a"},
	{"ld (hl), b", "ld (hl), c", "ld (hl), d", "ld (hl), e", "ld (hl), h", "ld (hl), l", "halt", "ld (hl), a", "ld a, b", "ld a, c", "ld a, d", "ld a, e", "ld a, h", "ld a, l", "ld a, (hl)", "ld a, a"},
	{"add a, b", "add a, c", "add a, d", "add a, e", "add a, h", "add a, l", "add a, (hl)", "add a, a", "adc a, b", "adc a, c", "adc a, d", "adc a, e", "adc h", "adc a, l", "adc (hl)", "adc a, a"},
	{"sub b", "sub c", "sub d", "sub e", "sub h", "sub l", "sub (hl)", "sub a", "sbc b", "sbc c", "sbc d", "sbc e", "sbc h", "sbc l", "sbc (hl)", "sbc a"},
	{"and b", "and c", "and d", "and e", "and h", "and l", "and (hl)", "and a", "xor b", "xor c", "xor d", "xor e", "xor h", "xor l", "xor (hl)", "xor a"},
	{"or b", "or c", "or d", "or e", "or h", "or l", "or (hl)", "or a", "cp b", "cp c", "cp d", "cp e", "cp h", "cp l", "cp (hl)", "cp a"},
	{"ret nz", "pop bc", "jp nz, a16", "jp a16", "call nz, a16", "push bc", "add a, d8", "rst 0x0", "ret z", "ret", "jp z, a16", "(cb prefix)", "call z, a16", "call a16", "adc a, d8", "rst 0x8"},
	{"ret nc", "pop de", "jp nc, a16", "(null)", "call nc, a16", "push de", ", sub d8", ", rst 0x10", "ret c", "reti", "jp c, a16", "(null)", "call c, a16", "(null)", "sbc a, d8", "rst 0x18"},
	{"ldh (a8), a", "pop hl", "ld (c), a", "(null)", "(null)", "push hl", "and d8", "rst 0x20", "add sp, r8", "jp (hl)", "ld (a16), a", "(null)", "(null)", "(null)", "xor d8", "rst 0x28"},
	{"ldh a, (a8)", "pop af", "ld a, (c)", "di", "(null)", "push af", "or d8", "rst 0x30", "ld hl, sp+r8", "ld sp, hp", "ld a, (a16)", "ei", "(null)", "(null)", "cp d8", "rst 0x38"},
};
#endif

void execute_instruction(gb_cpu_t *cpu, uint32_t instruction, FILE *assembly_dump) {
	#ifdef DEBUG
	uint8_t n1 = (instruction & 0xf0000000) >> 28;
	uint8_t n2 = (instruction & 0x0f000000) >> 24;
	static uint8_t *regNames = "facbedlh";
	/*fprintf(assembly_dump, "pc: 0x%x, sp:%04x, instruction(0x%06x): %s\n", cpu->pc, cpu->sp, instruction >> 8, _assembly_translation[n1][n2]);
	for (int i = 0; i < 8; i++) {
	 	fprintf(assembly_dump, "%c: 0x%x, ", regNames[i], *((uint8_t*)&cpu->f + i));
	}*/
	fprintf(assembly_dump, "pc: 0x%x, sp:%04x, instruction(0x%06x): %s\naf: 0x%04x, bc: 0x%04x, de: 0x%04x, hl: 0x%04x, ime: 0x%02x, if: 0x%02x, ie: 0x%02x\n\n", cpu->pc, cpu->sp, instruction >> 8, _assembly_translation[n1][n2], *cpu->af, *cpu->bc, *cpu->de, *cpu->hl, cpu->ime, *cpu->ifl, *cpu->ie);
	#endif

	if ((instruction & 0xff000000) == 0xcb000000)
		execute_prefix_instruction(cpu, (instruction & 0x00ff0000) >> 16);
	else
		execute_main_instruction(cpu, instruction);
}

/*
big thanks to this

https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html
*/

static void execute_main_instruction(gb_cpu_t *cpu, uint32_t instruction) {
	uint8_t n1 = ((instruction & 0xf0000000) >> 28);
	uint8_t n2 = ((instruction & 0x0f000000) >> 24);
	
	uint16_t immw = ((instruction & 0x00ffff00) >> 8);
	uint8_t imm1 = ((instruction & 0x00ff0000) >> 16);
	int8_t simm1 = (int8_t)imm1;

	switch (n1) {
		case 0x0: {
			switch (n2) {
				case 0x0: {
					// nop
					break;
				}
				case 0x1: {
					//confirmed good
					//*cpu->bc = immw;
					*cpu->bc = immw;
					break;
				}
				case 0x2: {
					//confirmed good
					//queue_task
					//attempt_write(cpu, cpu->addressSpace + *cpu->bc, &cpu->a);
					gb_write(cpu, *cpu->bc, cpu->a, 1);
					break;
				}
				case 0x3: {
					//confirmed good
					(*cpu->bc)++;
					
					break;
				}
				case 0x4: {
					//confirmed good (possibly H flag)
					if ((cpu->b++ & 0xf) == 0xf)
						set_flag(cpu, H_FLAG, 1);
					else
						set_flag(cpu, H_FLAG, 0);
					
					set_flag(cpu, N_FLAG, 0);
					set_flag(cpu, Z_FLAG, cpu->b == 0);
					break;
				}
				case 0x5: {
					//confirmed good (possibly H flag)
					if ((--cpu->b & 0xf) == 0xf)
						set_flag(cpu, H_FLAG, 1);
					else
						set_flag(cpu, H_FLAG, 0);
					
					set_flag(cpu, N_FLAG, 1);
					set_flag(cpu, Z_FLAG, cpu->b == 0);
					break;
				}
				case 0x6: {
					//confirmed good
					//cpu->b = imm1;
					cpu->b = imm1;
					break;
				}
				case 0x7: {
					//confirmed good
					bool bit = get_bit_on(cpu->a, 7);
					set_flag(cpu, C_FLAG, bit);
					set_flag(cpu, N_FLAG, 0);
					set_flag(cpu, H_FLAG, 0);
					cpu->a <<= 1;
					cpu->a |= bit;
					set_flag(cpu, Z_FLAG, 0);
					break;
				}
				case 0x8: {
					//confirmed good (possible endianness)
					uint8_t b1 = (cpu->sp >> 8) & 0xff;
					uint8_t b2 = cpu->sp & 0xff;

					uint8_t w1 = (immw >> 8) & 0xff;
					uint8_t w2 = immw & 0xff;
					gb_write(cpu, immw + 1, b1, 1);
					gb_write(cpu, immw, b2, 1);
					break;
				}
				case 0x9: {
					//probably good
					uint32_t val = (*cpu->hl + *cpu->bc);
					set_flag(cpu, C_FLAG, (val & 0x10000) == 0x10000);
					set_flag(cpu, H_FLAG, ((*cpu->hl & 0xfff) + (*cpu->bc & 0xfff) & 0x1000) == 0x1000);

					*cpu->hl = val & 0xffff;

					set_flag(cpu, N_FLAG, 0);
					set_flag(cpu, N_FLAG, 0);
					break;
				}
				case 0xA: {
					//confirmed good
					//cpu->a = cpu->addressSpace[*cpu->bc];
					cpu->a = gb_read(cpu, *cpu->bc);

					break;
				}
				case 0xB: {
					//confirmed good
					(*cpu->bc)--;
					break;
				}
				case 0xC: {
					//confirmed good (possible h flag)
					if ((cpu->c++ & 0xf) == 0xf)
						set_flag(cpu, H_FLAG, 1);
					else
						set_flag(cpu, H_FLAG, 0);

					set_flag(cpu, N_FLAG, 0);
					set_flag(cpu, Z_FLAG, cpu->c == 0);
					break;
				}
				case 0xD: {
					//confirmed good (possible h flag)
					if ((--cpu->c & 0xf) == 0xf)
						set_flag(cpu, H_FLAG, 1);
					else
						set_flag(cpu, H_FLAG, 0);

					set_flag(cpu, N_FLAG, 1);
					set_flag(cpu, Z_FLAG, cpu->c == 0);
					break;
				}
				case 0xE: {
					//confirmed good
					cpu->c = imm1;
					break;
				}
				case 0xF: {
					//confirmed good
					bool bit = get_bit_on(cpu->a, 0);
					set_flag(cpu, C_FLAG, bit);
					set_flag(cpu, H_FLAG, 0);
					set_flag(cpu, N_FLAG, 0);
					cpu->a >>= 1;
					cpu->a |= bit << 7;
					set_flag(cpu, Z_FLAG, 0);
					break;
					
				}
			}
			break;
		}
		case 0x1: {
			switch (n2) {
				case 0x0: {
					//confirmed good
					cpu->mode = 2;
					break;
				}
				case 0x1: {
					//confirmed good
					*cpu->de = immw;
					break;
				}
				case 0x2: {
					//confirmed good

					gb_write(cpu, *cpu->de, cpu->a, 1);
					break;
				}
				case 0x3: {
					//confirmed good
					(*cpu->de)++;
					break;
				}
				case 0x4: {
					//confirmed good (possible h flag)
					if ((cpu->d++ & 0xf) == 0xf)
						set_flag(cpu, H_FLAG, 1);
					else
						set_flag(cpu, H_FLAG, 0);
					
					set_flag(cpu, N_FLAG, 0);
					set_flag(cpu, Z_FLAG, cpu->d == 0);
					break;
				}
				case 0x5: {
					//confirmed good (possible h flag)
					if ((--cpu->d & 0xf) == 0xf)
						set_flag(cpu, H_FLAG, 1);
					else
						set_flag(cpu, H_FLAG, 0);
					
					set_flag(cpu, N_FLAG, 1);
					set_flag(cpu, Z_FLAG, cpu->d == 0);
					break;
				}
				case 0x6: {
					//confirmed good
					cpu->d = imm1;
					break;
				}
				case 0x7: {
					//confirmed good
					bool bit = get_bit_on(cpu->a, 7);
					bool newBit = get_flag_on(cpu, C_FLAG);
					set_flag(cpu, C_FLAG, bit);
					set_flag(cpu, N_FLAG, 0);
					set_flag(cpu, H_FLAG, 0);
					cpu->a <<= 1;
					cpu->a |= newBit;
					set_flag(cpu, Z_FLAG, 0);
					break;
				}
				case 0x8: {
					//confirmed good
					cpu->pc += simm1;
					break;
				}
				case 0x9: {
					//confirmed good (possible h flag)
					uint32_t val = (*cpu->hl + *cpu->de);
					set_flag(cpu, C_FLAG, (val & 0x10000) == 0x10000);
					set_flag(cpu, H_FLAG, ((*cpu->hl & 0xfff) + (*cpu->de & 0xfff) & 0x1000) == 0x1000);

					*cpu->hl = val & 0xffff;

					set_flag(cpu, N_FLAG, 0);
					break;
				}
				case 0xA: {
					//confirmed good
					cpu->a = gb_read(cpu, *cpu->de);//*(cpu->addressSpace + *cpu->de);
					break;
				}
				case 0xB: {
					//confirmed good
					(*cpu->de)--;
					break;
				}
				case 0xC: {
					//confirmed good (possible h flag)
					if ((cpu->e++ & 0xf) == 0xf)
						set_flag(cpu, H_FLAG, 1);
					else
						set_flag(cpu, H_FLAG, 0);
					
					set_flag(cpu, N_FLAG, 0);
					set_flag(cpu, Z_FLAG, cpu->e == 0);
					
					break;
				}
				case 0xD: {
					//confirmed good (possible h flag)
					if ((--cpu->e & 0xf) == 0xf)
						set_flag(cpu, H_FLAG, 1);
					else
						set_flag(cpu, H_FLAG, 0);
					
					set_flag(cpu, N_FLAG, 1);
					set_flag(cpu, Z_FLAG, cpu->e == 0);
					break;
				}
				case 0xE: {
					//confirmed good
					cpu->e = imm1;
					break;
				}
				case 0xF: {
					//confirmed good
					bool bit = get_flag_on(cpu, C_FLAG);
					set_flag(cpu, C_FLAG, get_bit_on(cpu->a, 0));
					set_flag(cpu, H_FLAG, 0);
					set_flag(cpu, N_FLAG, 0);
					cpu->a >>= 1;
					cpu->a |= bit << 7;
					set_flag(cpu, Z_FLAG, 0);
				}
			}
			break;
		}
		case 0x2: {
			switch (n2) {
				case 0x0: {
					//confirmed good
					if (!get_flag_on(cpu, Z_FLAG)) {
						//printf("simm1 is %i\n", simm1);
						//exit(-1);
					
						cpu->pc += simm1;
						cpu->instruction_wait_cycles = 12;
					} else {
						cpu->instruction_wait_cycles = 8;
					}

					break;
				}
				case 0x1: {
					//confirmed good
					*cpu->hl = immw;
					break;
				}
				case 0x2: {
					//confirmed good
					gb_write(cpu, *cpu->hl, cpu->a, 1);
					(*cpu->hl)++;
					break;
				}
				case 0x3: {
					//confirmed good
					(*cpu->hl)++;
					break;
				}
				case 0x4: {
					//confirmed good (h)
					if ((cpu->h++ & 0xf) == 0xf)
						set_flag(cpu, H_FLAG, 1);
					else
						set_flag(cpu, H_FLAG, 0);
					
					set_flag(cpu, N_FLAG, 0);
					set_flag(cpu, Z_FLAG, cpu->h == 0);
					break;
				}
				case 0x5: {
					//confirmed good (h)
					if ((--cpu->h & 0xf) == 0xf)
						set_flag(cpu, H_FLAG, 1);
					else
						set_flag(cpu, H_FLAG, 0);
					
					set_flag(cpu, N_FLAG, 1);
					set_flag(cpu, Z_FLAG, cpu->h == 0);
					break;
				}
				case 0x6: {
					//confirmed good
					cpu->h = imm1;
					break;
				}
				case 0x7: {
					uint8_t change = 0;
					if (get_flag_on(cpu, N_FLAG)) {
						// subtraction
						if (get_flag_on(cpu, C_FLAG)) {
							cpu->a -= 0x60;
						}
						if (get_flag_on(cpu, H_FLAG)) {
							cpu->a -= 0x6;
						}
					} else {
						// addition
						if (get_flag_on(cpu, C_FLAG) || cpu->a > 0x99) {

							cpu->a += 0x60;
							set_flag(cpu, C_FLAG, 1);
						}
						if (get_flag_on(cpu, H_FLAG) || (cpu->a & 0xf) > 0x9) {
							cpu->a += 0x6;
						}
					}
					set_flag(cpu, Z_FLAG, cpu->a == 0);
					set_flag(cpu, H_FLAG, 0);
					break;
				}
				case 0x8: {
					//confirmed good
					if (get_flag_on(cpu, Z_FLAG)) {
						cpu->pc += simm1;
						cpu->instruction_wait_cycles = 12;
					} else {
						cpu->instruction_wait_cycles = 8;
					}
						
					break;
				}
				case 0x9: {
					//confirmed good probably (flags)
					uint32_t val = (*cpu->hl + *cpu->hl);
					set_flag(cpu, C_FLAG, (val & 0x10000) == 0x10000);
					set_flag(cpu, H_FLAG, ((*cpu->hl & 0xfff) + (*cpu->hl & 0xfff) & 0x1000) == 0x1000);

					*cpu->hl = val & 0xffff;

					set_flag(cpu, N_FLAG, 0);
					break;
				}//0000 1000 0000 0000
				case 0xA: {
					//confirmed good
					cpu->a = gb_read(cpu, *cpu->hl);//cpu->addressSpace[*cpu->hl];
					(*cpu->hl)++;
					break;
				}
				case 0xB: {
					//confirmed good
					(*cpu->hl)--;
					break;
				}
				case 0xC: {
					//confirmed good (h)
					if ((cpu->l++ & 0xf) == 0xf)
						set_flag(cpu, H_FLAG, 1);
					else
						set_flag(cpu, H_FLAG, 0);
					
					set_flag(cpu, N_FLAG, 0);
					set_flag(cpu, Z_FLAG, cpu->l == 0);
					break;
				}
				case 0xD: {
					//confirmed good (h)
					if ((--cpu->l & 0xf) == 0xf)
						set_flag(cpu, H_FLAG, 1);
					else
						set_flag(cpu, H_FLAG, 0);
					
					set_flag(cpu, N_FLAG, 1);
					set_flag(cpu, Z_FLAG, cpu->l == 0);
					break;
				}
				case 0xE: {
					//confirmed good
					cpu->l = imm1;
					break;
				}
				case 0xF: {
					//confirmed good
					cpu->a = ~cpu->a;
					set_flag(cpu, N_FLAG, 1);
					set_flag(cpu, H_FLAG, 1);
					break;
				}
			}
			break;
		}
		case 0x3: {
			switch (n2) {
				case 0x0: {
					//confirmed good
					if (!get_flag_on(cpu, C_FLAG)) {
						cpu->pc += simm1;
						cpu->instruction_wait_cycles = 12;
					} else {
						cpu->instruction_wait_cycles = 8;
					}

					break;
				}
				case 0x1: {
					//confirmed good
					cpu->sp = immw;
					break;
				}
				case 0x2: {
					//confirmed good
					//attempt_write(cpu, cpu->addressSpace + *cpu->hl, &cpu->a);
					gb_write(cpu, *cpu->hl, cpu->a, 1);
					(*cpu->hl)--;
					break;
				}
				case 0x3: {
					//confirmed good
					cpu->sp++;
					break;
				}
				case 0x4: {
					//confirmed good (h)
					if ((( cpu->address_space[*cpu->hl] )++ & 0xf) == 0xf)
						set_flag(cpu, H_FLAG, 1);
					else
						set_flag(cpu, H_FLAG, 0);

					set_flag(cpu, N_FLAG, 0);
					set_flag(cpu, Z_FLAG, gb_read(cpu, *cpu->hl) == 0);
					break;
				}
				case 0x5: {
					//confirmed good (h)
					if ((--(cpu->address_space[*cpu->hl]) & 0xf) == 0xf)
						set_flag(cpu, H_FLAG, 1);
					else
						set_flag(cpu, H_FLAG, 0);

					set_flag(cpu, N_FLAG, 1);
					set_flag(cpu, Z_FLAG, gb_read(cpu, *cpu->hl) == 0);
					break;
				}
				case 0x6: {
					//confirmed good
					gb_write(cpu, *cpu->hl, imm1, 1);
					break;
				}
				case 0x7: {
					//confirmed good
					set_flag(cpu, C_FLAG, 1);
					set_flag(cpu, N_FLAG, 0);
					set_flag(cpu, H_FLAG, 0);
					break;
				}
				case 0x8: {
					//confirmed good
					if (get_flag_on(cpu, C_FLAG)) {
						cpu->pc += simm1;
						cpu->instruction_wait_cycles = 12;
					} else {
						cpu->instruction_wait_cycles = 8;
					}
						
					break;
				}
				case 0x9: {
					//confirmed good (flags maybe)
					uint32_t val = (*cpu->hl + cpu->sp);
					set_flag(cpu, C_FLAG, (val & 0x10000) == 0x10000);
					set_flag(cpu, H_FLAG, ((*cpu->hl & 0xfff) + (cpu->sp & 0xfff) & 0x1000) == 0x1000);

					*cpu->hl = val & 0xffff;

					set_flag(cpu, N_FLAG, 0);
					break;
				}
				case 0xA: {
					//confirmed good
					cpu->a = gb_read(cpu, *cpu->hl);//cpu->addressSpace[*cpu->hl];
					(*cpu->hl)--;
					break;
				}
				case 0xB: {
					//confirmed good
					cpu->sp--;

					break;
				}
				case 0xC: {
					//confirmed good (flags)
					if ((cpu->a++ & 0xf) == 0xf)
						set_flag(cpu, H_FLAG, 1);
					else
						set_flag(cpu, H_FLAG, 0);

					set_flag(cpu, N_FLAG, 0);
					set_flag(cpu, Z_FLAG, cpu->a == 0);
					break;
				}
				case 0xD: {
					//confirmed good (FLAGS)
					if ((--cpu->a & 0xf) == 0xf)
						set_flag(cpu, H_FLAG, 1);
					else
						set_flag(cpu, H_FLAG, 0);

					set_flag(cpu, N_FLAG, 1);
					set_flag(cpu, Z_FLAG, cpu->a == 0);
					break;
				}
				case 0xE: {
					//confirmed good
					cpu->a = imm1;
					break;
				}
				case 0xF: {
					//confirmed good
					set_flag(cpu, C_FLAG, !get_flag_on(cpu, C_FLAG));
					set_flag(cpu, N_FLAG, 0);
					set_flag(cpu, H_FLAG, 0);
					break;
				}
			}
			break;
		}
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7: {
			//confirmed good
			if (n1 == 0x7 && n2 == 0x6) {
				_halt(cpu);
				break;
			}
			uint8_t reg1index = (n1 - 0x4) * 2;
			if (n2 > 0x7)
				reg1index++;

			uint8_t *reg1, *reg2;
			if (reg1index == 6) {
				reg1 = NULL;
			} else {
				reg1 = cpu->registers[reg1index];
			}

			uint8_t reg2index = n2 % 8;
			if (reg2index == 6) {
				reg2 = NULL;
			} else {
				reg2 = cpu->registers[reg2index];
			}

			if (reg1index == 6) {
				gb_write(cpu, *cpu->hl, *reg2, 1);
				break;
			}
			if (reg2index == 6) {
				*reg1 = gb_read(cpu, *cpu->hl);
				break;
			}
			*reg1 = *reg2;

			break;
		}
		
		case 0x8: {
			//confirmed good possibly flags
			uint8_t *reg = cpu->registers[n2 % 8];
			
			if (n2 < 0x8) {
				uint16_t val;
				if (n2 % 8 == 6) {
					uint8_t memVal = gb_read(cpu, *cpu->hl);
					val = cpu->a + memVal;
					set_flag(cpu, C_FLAG, (val & 0x100) == 0x100);
					set_flag(cpu, H_FLAG, (((cpu->a & 0xf) + (memVal & 0xf)) & 0x10) == 0x10);
				} else {
					uint8_t memVal = *reg;
					val = cpu->a + memVal;
					set_flag(cpu, C_FLAG, (val & 0x100) == 0x100);
					set_flag(cpu, H_FLAG, (((cpu->a & 0xf) + (memVal & 0xf)) & 0x10) == 0x10);
				}
				cpu->a = val & 0xff;
			} else {
				uint16_t val;
				if (n2 % 8 == 6) {
					uint8_t memVal = gb_read(cpu, *cpu->hl);
					val = cpu->a + memVal + get_flag_on(cpu, C_FLAG);
					uint8_t low_nibble_a = cpu->a & 0xf;
					uint8_t low_nibble_nc = memVal & 0xf;
					set_flag(cpu, H_FLAG, (((low_nibble_a + low_nibble_nc + get_flag_on(cpu, C_FLAG)) & 0x10)) == 0x10);
					set_flag(cpu, C_FLAG, (val & 0x100) == 0x100);
				} else {
					uint8_t memVal = *reg;
					val = cpu->a + memVal + get_flag_on(cpu, C_FLAG);
					uint8_t low_nibble_a = cpu->a & 0xf;
					uint8_t low_nibble_nc = memVal & 0xf;
					set_flag(cpu, H_FLAG, (((low_nibble_a + low_nibble_nc + get_flag_on(cpu, C_FLAG)) & 0x10)) == 0x10);
					set_flag(cpu, C_FLAG, (val & 0x100) == 0x100);
				}
				cpu->a = val & 0xff;
			}
			set_flag(cpu, N_FLAG, 0);
			set_flag(cpu, Z_FLAG, cpu->a == 0);
			break;
		}

		case 0x9: {

			//confirmed good possibly flags
			uint8_t *reg = cpu->registers[n2 % 8];
			
			if (n2 < 0x8) {
				uint16_t val;
				if (n2 % 8 == 6) {
					uint8_t memVal = gb_read(cpu, *cpu->hl);
					val = cpu->a - memVal;
					set_flag(cpu, C_FLAG, cpu->a < memVal);
					set_flag(cpu, H_FLAG, ((cpu->a & 0xf) - (memVal & 0xf)) < 0);
				} else {
					val = cpu->a - *reg;
					set_flag(cpu, C_FLAG, cpu->a < *reg);
					set_flag(cpu, H_FLAG, ((cpu->a & 0xf) - (*reg & 0xf)) < 0);
				}
				cpu->a = val & 0xff;
			} else {
				uint16_t nc;
				if (n2 % 8 == 6) {
					nc = gb_read(cpu, *cpu->hl) + get_flag_on(cpu, C_FLAG);
				} else {
					nc = *reg + get_flag_on(cpu, C_FLAG);
				}

				uint16_t val = cpu->a - nc;
				set_flag(cpu, H_FLAG, ((cpu->a & 0xf) - (val & 0xf) - get_flag_on(cpu, C_FLAG)) < 0);
				set_flag(cpu, C_FLAG, cpu->a < nc);
				cpu->a = val & 0xff;
			}

			set_flag(cpu, N_FLAG, 1);
			set_flag(cpu, Z_FLAG, cpu->a == 0);
			break;
		}
		case 0xA: {
			//confirmed good
			uint8_t *reg = cpu->registers[n2 % 8];
			
			if (n2 < 0x8) {
				set_flag(cpu, H_FLAG, 1);
				if (n2 % 8 == 0x6) {
					cpu->a &= gb_read(cpu, *cpu->hl);
				} else {
					cpu->a &= *reg;
				}
			} else {
				set_flag(cpu, H_FLAG, 0);
				if (n2 % 8 == 0x6) {
					cpu->a ^= gb_read(cpu, *cpu->hl);
				} else {
					cpu->a ^= *reg;
				}
			}
			
			set_flag(cpu, Z_FLAG, cpu->a == 0);
			set_flag(cpu, N_FLAG, 0);
			set_flag(cpu, C_FLAG, 0);
			break;
		}
		case 0xB: {
			//confirmed good
			uint8_t *reg = cpu->registers[n2 % 8];
			
			if (n2 < 0x8) {
				set_flag(cpu, N_FLAG, 0);
				set_flag(cpu, H_FLAG, 0);
				set_flag(cpu, C_FLAG, 0);

				uint8_t val;
				if (n2 % 8 == 6)
					val = gb_read(cpu, *cpu->hl);
				else
					val = *reg;

				cpu->a |= val;
				set_flag(cpu, Z_FLAG, cpu->a == 0);
			} else {
				uint8_t val;
				if (n2 % 8 == 6)
					val = gb_read(cpu, *cpu->hl);
				else
					val = *reg;

				if ((cpu->a & 0xf) < (val & 0xf))
					set_flag(cpu, H_FLAG, 1);
				else
					set_flag(cpu, H_FLAG, 0);

				if (cpu->a < val)
					set_flag(cpu, C_FLAG, 1);
				else
					set_flag(cpu, C_FLAG, 0);

				set_flag(cpu, Z_FLAG, cpu->a == val);
				set_flag(cpu, N_FLAG, 1);
			}
			break;
		}
		case 0xC: {
			
			switch (n2) {
				case 0x0: {
					//confirmed good
					if (!get_flag_on(cpu, Z_FLAG)) {
						_ret(cpu);
						cpu->instruction_wait_cycles = 20;
					} else {
						cpu->instruction_wait_cycles = 8;
					}

					break;
				}
				case 0x1: {
					//confirmed good
					_pop(cpu, cpu->bc);
					break;
				}
				case 0x2: {
					//confirmed good
					if (!get_flag_on(cpu, Z_FLAG)) {
						cpu->pc = immw;
						cpu->instruction_wait_cycles = 16;
					} else {
						cpu->instruction_wait_cycles = 12;
					}
						
					break;
				}
				case 0x3: {
					//confirmed good
					cpu->pc = immw;
					break;
				}
				case 0x4: {
					//confirmed good
					if (!get_flag_on(cpu, Z_FLAG)) {
						_push(cpu, &cpu->pc);
						cpu->pc = immw;
						cpu->instruction_wait_cycles = 24;
					} else {
						cpu->instruction_wait_cycles = 12;
					}

					break;
				}
				case 0x5: {
					//confirmed good
					_push(cpu, cpu->bc);
					break;
				}
				case 0x6: {
					//confirmed good (possible flags)
					//uint16_t val = cpu->a + imm1;
					uint16_t val = cpu->a + imm1;
					set_flag(cpu, C_FLAG, (val & 0x100) == 0x100);
					set_flag(cpu, H_FLAG, (((cpu->a & 0xf) + (imm1 & 0xf)) & 0x10) == 0x10);
					cpu->a = val & 0xff;
					set_flag(cpu, N_FLAG, 0);
					set_flag(cpu, Z_FLAG, cpu->a == 0);
					break;
				}
				case 0x7: {
					//confirmed good
					_push(cpu, &cpu->pc);
					cpu->pc = 0;
					break;
				}
				case 0x8: {
					//confirmed good
					if (get_flag_on(cpu, Z_FLAG)) {
						_ret(cpu);
						cpu->instruction_wait_cycles = 20;
					} else {
						cpu->instruction_wait_cycles = 8;
					}
					break;
				}
				case 0x9: {
					//confirmed good
					_ret(cpu);
					break;
				}
				case 0xA: {
					//confirmed good
					if (get_flag_on(cpu, Z_FLAG)) {
						cpu->pc = immw;
						cpu->instruction_wait_cycles = 16;
					} else {
						cpu->instruction_wait_cycles = 12;
					}
						
					break;
				}
				case 0xB: {
					// cb, should never get here
					break;
				}
				case 0xC: {
					//confirmed good
					if (get_flag_on(cpu, Z_FLAG)) {
						_push(cpu, &cpu->pc);
						cpu->pc = immw;
						cpu->instruction_wait_cycles = 24;
					} else {
						cpu->instruction_wait_cycles = 12;
					}

					break;
				}
				case 0xD: {
					//confirmed good
					_push(cpu, &cpu->pc);
					cpu->pc = immw;
					break;
				}
				case 0xE: {
					uint16_t val = cpu->a + imm1 + get_flag_on(cpu, C_FLAG);
					uint8_t low_nibble_a = cpu->a & 0xf;
					uint8_t low_nibble_nc = imm1 & 0xf;
					set_flag(cpu, H_FLAG, (((low_nibble_a + low_nibble_nc + get_flag_on(cpu, C_FLAG)) & 0x10)) == 0x10);
					set_flag(cpu, C_FLAG, (val & 0x100) == 0x100);
					cpu->a = val & 0xff;
					set_flag(cpu, Z_FLAG, cpu->a == 0);
					set_flag(cpu, N_FLAG, 0);
					break;
				}
				case 0xF: {
					//confirmed good
					_push(cpu, &cpu->pc);
					cpu->pc = 0x8;
					break;
				}
			}
			break;
		}
		case 0xD: {
			switch (n2) {
				case 0x0: {
					//confirmed good
					if (!get_flag_on(cpu, C_FLAG)) {
						_ret(cpu);
						cpu->instruction_wait_cycles = 20;
					} else {
						cpu->instruction_wait_cycles = 8;
					}
					break;
				}
				case 0x1: {
					//confirmed good
					_pop(cpu, cpu->de);
					break;
				}
				case 0x2: {
					//confirmed good
					if (!get_flag_on(cpu, C_FLAG)) {
						cpu->pc = immw;
						cpu->instruction_wait_cycles = 16;
					} else {
						cpu->instruction_wait_cycles = 12;
					}

					break;
				}
				case 0x3: {
					// nothing
					break;
				}
				case 0x4: {
					//confirmed good
					if (!get_flag_on(cpu, C_FLAG)) {
						_push(cpu, &cpu->pc);
						cpu->pc = immw;
						cpu->instruction_wait_cycles = 24;
					} else {
						cpu->instruction_wait_cycles = 12;
					}
					break;
				}
				case 0x5: {
					//confirmed good
					_push(cpu, cpu->de);
					break;
				}
				case 0x6: {
					//confirmed good (flags)
					uint16_t val = cpu->a - imm1;

					set_flag(cpu, C_FLAG, cpu->a < imm1);
					set_flag(cpu, H_FLAG, (((cpu->a & 0xf) - (imm1 & 0xf)) & 0x10) == 0x10);

					cpu->a = val;

					set_flag(cpu, N_FLAG, 1);
					set_flag(cpu, Z_FLAG, cpu->a == 0);
					break;
				}
				case 0x7: {
					//confirmed good
					_push(cpu, &cpu->pc);
					cpu->pc = 0x10;
					break;
				}
				case 0x8: {
					//confirmed good
					if (get_flag_on(cpu, C_FLAG)) {
						_ret(cpu);
						cpu->instruction_wait_cycles = 20;
					} else {
						cpu->instruction_wait_cycles = 8;
					}

					break;
				}
				case 0x9: {
					//confirmed good
					cpu->ime = true;
					_ret(cpu);
					break;
				}
				case 0xA: {
					//confirmed good
					if (get_flag_on(cpu, C_FLAG)) {
						cpu->pc = immw;
						cpu->instruction_wait_cycles = 16;
					} else {
						cpu->instruction_wait_cycles = 12;
					}
					break;
				}
				case 0xB: {
					// nothing
					break;
				}
				case 0xC: {
					//confirmed good
					if (get_flag_on(cpu, C_FLAG)) {
						_push(cpu, &cpu->pc);
						cpu->pc = immw;
						cpu->instruction_wait_cycles = 24;
					} else {
						cpu->instruction_wait_cycles = 12;
					}

					break;
				}
				case 0xD: {
					// nothing
					break;
				}
				case 0xE: {
				//confirmed good (flags)
					uint16_t nc = imm1 + get_flag_on(cpu, C_FLAG);
					uint16_t val = cpu->a - nc;
					set_flag(cpu, H_FLAG, (((cpu->a & 0xf) - (imm1 & 0xf) - get_flag_on(cpu, C_FLAG)) & 0x10) == 0x10);
					set_flag(cpu, C_FLAG, cpu->a < nc);
					cpu->a = val & 0xff;

					set_flag(cpu, Z_FLAG, cpu->a == 0);
					set_flag(cpu, N_FLAG, 1);
					break;
				}
				case 0xF: {
					//confirmed good
					_push(cpu, &cpu->pc);
					cpu->pc = 0x18;
					break;
				}
			}
			break;
		}
		case 0xE: {
			switch (n2) {
				case 0x0: {
					//confirmed good
					//attempt_write(cpu, cpu->addressSpace + 0xFF00 + imm1, &cpu->a);
					gb_write(cpu, 0xFF00 + imm1, cpu->a, 1);
					break;
				}
				case 0x1: {
					//confirmed good
					_pop(cpu, cpu->hl);
					break;
				}
				case 0x2: {
					//confirmed good
					//attempt_write(cpu, cpu->addressSpace + 0xFF00 + cpu->c, &cpu->a);
					gb_write(cpu, 0xFF00 + cpu->c, cpu->a, 1);
					break;
				}
				case 0x3: {
					// nothing
					break;
				}
				case 0x4: {
					// nothing
					break;
				}
				case 0x5: {
					//confirmed good
					_push(cpu, cpu->hl);
					break;
				}

				case 0x6: {
					// and d8
					//exit(-1);
					set_flag(cpu, H_FLAG, 1);
					set_flag(cpu, N_FLAG, 0);
					set_flag(cpu, C_FLAG, 0);
					cpu->a &= imm1;
					set_flag(cpu, Z_FLAG, cpu->a == 0);
					break;
				}
				case 0x7: {
					_push(cpu, &cpu->pc);
					cpu->pc = 0x20;
					break;
				}
				case 0x8: {
					//confirmed good (flags)
					uint32_t val = cpu->sp + simm1;
					if (simm1 > 0) {
						set_flag(cpu, C_FLAG, (((cpu->sp & 0xff) + simm1) & 0x100) == 0x100);
						set_flag(cpu, H_FLAG, (((cpu->sp & 0xf) + (simm1 & 0xf)) & 0x10) == 0x10); 
					} else {
						set_flag(cpu, C_FLAG, (val & 0xff) < (cpu->sp & 0xff));
						set_flag(cpu, H_FLAG, (val & 0xf) < (cpu->sp & 0xf));
					}

					cpu->sp = val & 0xffff;
					set_flag(cpu, Z_FLAG, 0);
					set_flag(cpu, N_FLAG, 0);
					break;
				}
				case 0x9: {
					//confirmed good
					cpu->pc = *cpu->hl;
					break;
				}
				case 0xA: {
					//confirmed good
					//attempt_write(cpu, (uint8_t *)cpu->addressSpace + immw, &cpu->a);
					gb_write(cpu, immw, cpu->a, 1);
					break;
				}
				case 0xB: {
					// nothing
					break;
				}
				case 0xC: {
					// nothing
					break;
				}
				case 0xD: {
					// nothing
					break;
				}
				case 0xE: {
					//confirmed good
					set_flag(cpu, N_FLAG, 0);
					set_flag(cpu, H_FLAG, 0);
					set_flag(cpu, C_FLAG, 0);
					cpu->a ^= imm1;
					set_flag(cpu, Z_FLAG, cpu->a == 0);
					break;
				}
				case 0xF: {
					//confirmed good
					_push(cpu, &cpu->pc);
					cpu->pc = 0x28;
					break;
				}
			}
			break;
		}
		case 0xF: {
			switch (n2) {
				case 0x0: {
					//confirmed good
					cpu->a = gb_read(cpu, 0xFF00 + imm1);
					break;

				}
				case 0x1: {
					//confirmed good
					_pop(cpu, cpu->af);
					break;
				}
				case 0x2: {
					//cpu->addressSpace[cpu->c] = cpu->a;
					//confirmed good
					cpu->a = gb_read(cpu, 0xFF00 + cpu->c);
					break;
				}
				case 0x3: {
					//confirmed good
					cpu->ime = false;
					break;
				}
				case 0x4: {
					// nothing
					break;
				}
				case 0x5: {
					//confirmed good
					_push(cpu, cpu->af);
					break;
				}
				case 0x6: {
					// or d8
					set_flag(cpu, N_FLAG, 0);
					set_flag(cpu, H_FLAG, 0);
					set_flag(cpu, C_FLAG, 0);
					cpu->a |= imm1;
					set_flag(cpu, Z_FLAG, cpu->a == 0);
					break;
				}
				case 0x7: {
					//confirmed good
					_push(cpu, &cpu->pc);
					cpu->pc = 0x30;
					break;
				}
				case 0x8: {
					uint32_t val = cpu->sp + simm1;

					if (simm1 > 0) {
						set_flag(cpu, C_FLAG, (((cpu->sp & 0xff) + simm1) & 0x100) == 0x100);
						set_flag(cpu, H_FLAG, (((cpu->sp & 0xf) + (simm1 & 0xf)) & 0x10) == 0x10); 
					} else {
						set_flag(cpu, C_FLAG, (val & 0xff) < (cpu->sp & 0xff));
						set_flag(cpu, H_FLAG, (val & 0xf) < (cpu->sp & 0xf));
					}

					*cpu->hl = val & 0xffff;

					set_flag(cpu, Z_FLAG, 0);
					set_flag(cpu, N_FLAG, 0);
					break;
				}
				case 0x9: {
					//confirmed good
					cpu->sp = *cpu->hl;
					break;
				}
				case 0xA: {
					//confirmed good
					cpu->a = gb_read(cpu, immw);
					//cpu->a = cpu->addressSpace[immw];
					break;
				}
				case 0xB: {
					//confirmed good
					//cpu->ime = true;
					queue_task(cpu, QUEUE_INTERRUPT_ENABLE, 0, 1);
					break;
				}
				case 0xC: {
					// nothing
					break;
				}
				case 0xD: {
					// nothing
					break;
				}
				case 0xE: {
					//confirmed good (flags)
					if ((cpu->a & 0xf) < (imm1 & 0xf))
						set_flag(cpu, H_FLAG, 1);
					else
						set_flag(cpu, H_FLAG, 0);

					if (cpu->a < imm1)
						set_flag(cpu, C_FLAG, 1);
					else
						set_flag(cpu, C_FLAG, 0);
					
					
					/* 
						**same thing**
						
						set_flag(cpu, H_FLAG, (cpu->a & 0xf) < (imm1 & 0xf));
						set_flag(cpu, C_FLAG, cpu->a < imm1);
					*/

					set_flag(cpu, Z_FLAG, cpu->a == imm1);
					set_flag(cpu, N_FLAG, 1);
					break;
				}
				case 0xF: {
					//confirmed good
					_push(cpu, &cpu->pc);
					cpu->pc = 0x38;
					break;
				}
			}
			break;
		}
	}
	//cpu->instruction_wait_cycles /= 4;
}

static void execute_prefix_instruction(gb_cpu_t *cpu, uint8_t opcode) {
	uint8_t n1 = (opcode & 0xf0) >> 4;
	uint8_t n2 = opcode & 0x0f;
	uint8_t *reg = cpu->registers[n2 % 8];
	//printf("instr: %x\n", opcode);

	if ((n2 % 8) == 6) {
		reg = get_ptr_to(cpu, *cpu->hl);
		cpu->instruction_wait_cycles = 16;
	} else {
		cpu->instruction_wait_cycles = 8;
	}

	//TODO: do write checks for *cpu->hl instances

	switch (n1) {
		case 0x0: {
			//confirmed good
			set_flag(cpu, N_FLAG, 0);
			set_flag(cpu, H_FLAG, 0);

			if (n2 < 0x8) {
				bool bit = get_bit_on(*reg, 7);
				set_flag(cpu, C_FLAG, bit);
				*reg <<= 1;
				*reg |= bit;
			} else {
				bool bit = get_bit_on(*reg, 0);
				set_flag(cpu, C_FLAG, bit);
				*reg >>= 1;
				*reg |= bit << 7;
			}
			
			set_flag(cpu, Z_FLAG, *reg == 0);
			break;
		}
		case 0x1: {
			//confirmed good
			set_flag(cpu, N_FLAG, 0);
			set_flag(cpu, H_FLAG, 0);
			if (n2 < 0x8) {

				bool bit = get_flag_on(cpu, C_FLAG);
				set_flag(cpu, C_FLAG, get_bit_on(*reg, 7));
				*reg <<= 1;
				*reg |= bit;
			} else {
				bool bit = get_bit_on(*reg, 0);
				*reg >>= 1;
				*reg |= get_flag_on(cpu, C_FLAG) << 7;
				set_flag(cpu, C_FLAG, bit);
			}

			set_flag(cpu, Z_FLAG, (*reg) == 0);
			break;
		}
		case 0x2: {
			//confirmed good
			set_flag(cpu, N_FLAG, 0);
			set_flag(cpu, H_FLAG, 0);
			if (n2 < 0x8) {
				set_flag(cpu, C_FLAG, get_bit_on(*reg, 7));
				*reg <<= 1;
				set_bit_on(reg, 0, 0);
			} else {
				bool bit = get_bit_on(*reg, 7);
				set_flag(cpu, C_FLAG, get_bit_on(*reg, 0));
				*reg >>= 1;
				set_bit_on(reg, 7, bit);
			}
			set_flag(cpu, Z_FLAG, *reg == 0);
			break;
		}
		case 0x3: {
			//confirmed good
			set_flag(cpu, N_FLAG, 0);
			set_flag(cpu, H_FLAG, 0);
			if (n2 < 0x8) {
				set_flag(cpu, C_FLAG, 0);
				uint8_t newValue = ( (*reg & 0x0F) << 4 | (*reg & 0xF0) >> 4 );
				*reg = newValue;
			} else {
				bool bit = get_bit_on(*reg, 0);
				set_flag(cpu, C_FLAG, bit);
				*reg >>= 1;
				set_bit_on(reg, 7, 0);

			}
			set_flag(cpu, Z_FLAG, *reg == 0);
			break;
		}
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7: {
			//confirmed good
			if ((n2 % 8) == 6 && (n2 == 0x6 || n2 == 0xE)) {
				cpu->instruction_wait_cycles = 12;
			}
			uint8_t bit = (n1 - 0x4) * 2;
			if (n2 > 0x7)
				bit++;
			bool status = get_bit_on(*reg, bit);
			set_flag(cpu, N_FLAG, 0);
			set_flag(cpu, H_FLAG, 1);
			set_flag(cpu, Z_FLAG, status == 0);
			break;
		}
		case 0x8:
		case 0x9:
		case 0xA:
		case 0xB: {
			//confirmed good
			uint8_t bit = (n1 - 0x8) * 2;
			if (n2 > 0x7)
				bit++;
			set_bit_on(reg, bit, 0);
			break;
		}

		case 0xC:
		case 0xD:
		case 0xE:
		case 0xF: {
			//confirmed good
			uint8_t bit = (n1 - 0xC) * 2;
			if (n2 > 0x7)
				bit++;
			set_bit_on(reg, bit, 1);
			break;
		}
	}
}