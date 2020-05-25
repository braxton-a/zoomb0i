#include "rom.h"
#include "cpu.h"

void load_rom_bank(void *cpu, uint8_t data) {
	FILE *rom = ((gb_cpu_t *)cpu)->romptr;

	if (data == 0)
		data = 1;

	// seek to correct ROM bank
	fseek(rom, data * 0x4000, SEEK_SET);

	// write the 16KB to the 2nd 16KB rom bank (0x4000 - 0x7FFF)
	fread(((gb_cpu_t *)cpu)->address_space + 0x4000, sizeof(char), 0x4000, rom);
}