#ifndef rom_h
#define rom_h

#include <stdint.h>

struct rom_header {
	uint8_t entry_point[4];
	uint8_t nintendo_logo[48];
	char title[16];
	uint16_t license_code;
	uint8_t sgb_flag;
	uint8_t cartridge_type;
	uint8_t rom_size;
	uint8_t ram_size;
	uint8_t destination_code;
	uint8_t old_license_code;
	uint8_t header_checksum;
	uint16_t rom_checksum;
};

#endif

void load_rom_bank(void *cpu, uint8_t data);
/*
if (!get_flag_on(cpu, N)) {
						if (cpu->a > 0x99 || get_flag_on(cpu, C)) {
							cpu->a += 0x60;
							set_flag(cpu, C, 1);
						}
						if ((cpu->a & 0xf) > 0x9 || get_flag_on(cpu, H)) {
							cpu->a += 0x6;
						}

					} else {
						if (get_flag_on(cpu, C)) {
							cpu->a -= 0x60;
						}
						if (get_flag_on(cpu, H)) {
							cpu->a -= 0x6;
						}
					}
					//cpu->a &= 0xFF;

					set_flag(cpu, Z, cpu->a == 0);
					set_flag(cpu, H, 0);*/