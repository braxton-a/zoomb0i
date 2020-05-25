#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "cpu.h"
#include "utils.h"

void *read_bytes(FILE *file, uint32_t offset, size_t size) {
	void *buffer = calloc(sizeof(char), size);
	fseek(file, offset, SEEK_SET);
	fread(buffer, size, sizeof(char), file);
	return buffer;
}

void ram_dump(void *cpu) {
	FILE *dump = fopen("ramdump.bin", "wb+");
	for (int i = 0; i < 0x10000; i++) {
		fwrite(((gb_cpu_t *)cpu)->address_space + i, sizeof(uint8_t), 1, dump);
	}
	fclose(dump);
}