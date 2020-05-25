#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "rom.h"
#include "cpu.h"
#include "ppu.h"
#include "utils.h"
#include "input.h"

#define SCALE 3


#define INTERNAL_SCREEN_WIDTH 160
#define INTERNAL_SCREEN_HEIGHT 144

//#define DEBUG

//#define GB_DEBUGGER

static void check_interrupts(gb_cpu_t *cpu) {
	for (int b = 0; b < 5; b++) {
		uint8_t int_enabled = ((*cpu->ie) >> b) & 1;
		uint8_t bit = ((*cpu->ifl) >> b) & 1;
		if (int_enabled) {
			if (bit) {
				if (cpu->ime) {
					#ifdef DEBUG
					static char *strs[] = {"VBLANK", "LCD STAT", "TIMER", "SERIAL", "JOY"};
					printf("interrupt %s requested\n", strs[b]);
					#endif
					_push(cpu, &cpu->pc);
					cpu->pc = 0x40 + (b * 0x8);
					*cpu->ifl &= ~(1UL << b);
					cpu->ime = 0;
					cpu->instruction_wait_cycles = 8;
					cpu->shouldHalt = false;
				} else {
					cpu->shouldHalt = false;
					//cpu.pc++;
				}
			}
		}
	}
}

#ifdef DEBUG
static void update_overlay(gb_cpu_t *cpu, gb_ppu_t *ppu) {
	static SDL_Rect debugOverlayRect = {0, 0, 160, 80};

	SDL_SetRenderDrawColor(ppu->renderer, 50, 50, 50, 180);
	SDL_RenderFillRect(ppu->renderer, &debugOverlayRect);

	static TTF_Font *debugFont;
	bool hasRun = false;
	if (!hasRun) {
		debugFont = TTF_OpenFont("debugfont.ttf", 12);
		hasRun = true;
	}

	static SDL_Color White = {255, 255, 255};
	static char buf[0x10];

	uint16_t pc = cpu->pc;
	sprintf(buf, "pc: 0x%x", pc);
	SDL_Surface *surfacePC = TTF_RenderText_Blended(debugFont, buf, White); // as TTF_RenderText_Solid could only be used on SDL_Surface then you have to create the surface first
	SDL_Texture *textPC = SDL_CreateTextureFromSurface(ppu->renderer, surfacePC); //now you can convert it into a texture

	uint16_t af = *cpu->af;
	sprintf(buf, "af: 0x%x", af);
	SDL_Surface *surfaceAF = TTF_RenderText_Blended(debugFont, buf, White); // as TTF_RenderText_Solid could only be used on SDL_Surface then you have to create the surface first
	SDL_Texture *textAF = SDL_CreateTextureFromSurface(ppu->renderer, surfaceAF); //now you can convert it into a texture

	uint16_t bc = *cpu->bc;
	sprintf(buf, "bc: 0x%x", bc);
	SDL_Surface *surfaceBC = TTF_RenderText_Blended(debugFont, buf, White); // as TTF_RenderText_Solid could only be used on SDL_Surface then you have to create the surface first
	SDL_Texture *textBC = SDL_CreateTextureFromSurface(ppu->renderer, surfaceBC);

	int w, h;
	SDL_QueryTexture(textAF, NULL, NULL, &w, &h);

	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = w;
	rect.h = h;

	SDL_RenderCopy(ppu->renderer, textPC, NULL, &rect); 
	{
		int w, h;
		SDL_QueryTexture(textAF, NULL, NULL, &w, &h);

		rect.y = 30;
		rect.w = w;
		rect.h = h;
	}
	SDL_RenderCopy(ppu->renderer, textAF, NULL, &rect);
	{
		int w, h;
		SDL_QueryTexture(textAF, NULL, NULL, &w, &h);

		rect.y = 60;
		rect.w = w;
		rect.h = h;
	}
	SDL_RenderCopy(ppu->renderer, textBC, NULL, &rect);

	SDL_FreeSurface(surfacePC);
	SDL_DestroyTexture(textPC);
	SDL_FreeSurface(surfaceAF);
	SDL_DestroyTexture(textAF);
	TTF_CloseFont(debugFont);
	draw_screen(ppu);
}

void printfbin(uint8_t byte) {

    for (int i = 0; i < 8; i++) {
        putc(byte & (0x80 >> i) ? '1' : '0', stdout);
    }
    puts("");
}

#endif

static void update_display(gb_cpu_t *cpu, gb_ppu_t *ppu, SDL_Texture *display_texture, uint32_t pixels[INTERNAL_SCREEN_HEIGHT][INTERNAL_SCREEN_WIDTH], bool screen_on) {
	if (screen_on) {
		SDL_SetRenderDrawColor(ppu->renderer, 0, 0, 0, 255);
		SDL_RenderClear(ppu->renderer);
		SDL_UpdateTexture(display_texture, NULL, pixels, INTERNAL_SCREEN_WIDTH * sizeof(uint32_t));
		SDL_Rect main_text_rect;
		{
			int w, h;
			SDL_QueryTexture(display_texture, NULL, NULL, &w, &h);
			main_text_rect.x = 0;
			main_text_rect.y = 0;
			main_text_rect.w = w * SCALE;
			main_text_rect.h = h * SCALE;
		}
		SDL_RenderCopy(ppu->renderer, display_texture, NULL, &main_text_rect);
			
		#ifdef DEBUG
		update_overlay(cpu, ppu);
		#endif
		draw_screen(ppu);
	}
}

static void last_component(const char *string, char *dest) {
	if (string == NULL || dest == NULL)
		return;

	for (int i = strlen(string); i > 0; i--) {
		if (string[i] == '\\' || string[i] == '/') {
			strcpy(dest, string + i + 1);
			break;
		}
	}
}


int main(int argc, char *argv[]) {
	if (argc < 2) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Usage: \zoomb0i <rom file>", NULL);
		return 1;
	}

	
	FILE *rom = fopen(argv[1], "rb");
	if (!rom) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Failed to open rom file.", NULL);
		return 1;
	}

	const bool debug_enabled = false;
	const bool load_bootstrap = false;
	const bool start_pause = false;


	srand(time(NULL));
	SDL_Init(SDL_INIT_VIDEO);
	TTF_Init();

	const float build_number = 3.4f;
	const double target_frame_rate = 59.7275;
	const double target_delay_time = 1000 / target_frame_rate;

	char window_title[128];
	char *game_name = malloc(strlen(argv[1]) + 1);

	last_component(argv[1], game_name);
	sprintf(window_title, "%s - %s", "zoomb0i 0.1~alpha", game_name);
	SDL_Window *main_window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, VISIBLE_SCREEN_WIDTH, VISIBLE_SCREEN_HEIGHT, 0);
	free(game_name);

	if (!main_window) {
		err("Failed to create window.\n");
		return 1;
	}

	uint32_t pixels[INTERNAL_SCREEN_HEIGHT][INTERNAL_SCREEN_WIDTH];

	for (int i = 0; i < INTERNAL_SCREEN_HEIGHT*INTERNAL_SCREEN_WIDTH; i++) {
		*((uint32_t *)pixels + i) = 0;
	}

	gb_cpu_t cpu;
	gb_ppu_t ppu;
	init_cpu(&cpu, &ppu, load_bootstrap);
	init_ppu(&ppu, &cpu, pixels);
	printf("zoomb0i emulator\n\n\n");
	ppu.renderer = SDL_CreateRenderer(main_window, -1, SDL_RENDERER_ACCELERATED);
	//SDL_SetRenderDrawBlendMode(ppu.renderer, SDL_BLENDMODE_BLEND);
	struct rom_header *header = read_bytes(rom, 0x100, sizeof(struct rom_header));
	if (header->old_license_code == 0x33) {
		if (header->sgb_flag == 0x03)
			cpu.run_mode = 2;
		else
			cpu.run_mode = 1;
	}
	if (header->cartridge_type == 0x1 || header->cartridge_type == 0x2 || header->cartridge_type == 0x3) {
		cpu.mbc1 = true;
	}
	uint32_t rom_size = 0;
	if (header->rom_size <= 0x8)
		rom_size = 32768 << header->rom_size;
	else if (header->rom_size == 0x52)
		rom_size = 72 * 32768;
	else if (header->rom_size == 0x53)
		rom_size = 80 * 32768;
	else
		rom_size = 96 * 32768;
		
	free(header);

	fseek(rom, 0, SEEK_SET);
	fread(cpu.address_space, sizeof(char), 0x8000, rom);

	if (load_bootstrap) {
		FILE *bootloader = fopen("bootloader.bin", "rb");
		if (bootloader)
			fread(cpu.address_space, 1, 0x100, bootloader);
		fclose(bootloader);
	}

	cpu.romptr = rom;

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

	SDL_Texture *texture = SDL_CreateTexture(ppu.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, INTERNAL_SCREEN_WIDTH, INTERNAL_SCREEN_HEIGHT);


	cpu.key_status = 0xff;
	bool bootloader_mapped = false;
	bool running = true;
	

	SDL_Color white = {255, 255, 255, 255};
	#ifdef DEBUG
	FILE *someDump = fopen("debug/asmdump.txt", "wb+");
	FILE *adump = fopen("debug/oamdump", "w+");
	#endif


	uint32_t frame_start, 
			 frame_time, 
			 dot_clock_wait, 
			 total_time_waited;

	bool pause = false;
	if (start_pause)
		pause = true;

	dot_clock_wait = 0;

	uint64_t iteration = 0;

	while (running) {
		frame_start = SDL_GetTicks();
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				running = false;
			} else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
				handle_keypress(&e.key, &cpu);
				int key = e.key.keysym.sym;
				if (key == SDLK_d) {
					printf("Dumping ram...\n");
					ram_dump(&cpu);
					printf("Done dumping ram! Exiting...\n");
					exit(-1);
				}
				/*if (key == SDLK_p) {
					pause = !pause;
				}
				if (key == SDLK_j && pause) {
					printf("\r[REGISTERS]: PC: 0x%x, SP: 0x%x, AF: 0x%04x, BC: 0x%04x, DE: 0x%04x, HL: 0x%04x", cpu.pc, cpu.sp, *cpu.af, *cpu.bc, *cpu.de, *cpu.hl);
    				fflush(stdout);
					iteration++;
					goto nextInstr;
				}*/
				
			}
		}
		
		if (pause)
			continue;

		nextInstr:
			NULL;
		static int frame_count_sec = 0;
		bool screen_on = ((*cpu.lcdc) >> 7) & 1;

		// 70224 instructions per frame (59.7275 frames per second on DMG)
		for (int i = 0; i < 70224; i++) {
			if ((*cpu.joyp >> 4) & 1) {
				*cpu.joyp &= 0xf0;
				*cpu.joyp |= cpu.key_status & 0xf;
			} else {
				*cpu.joyp &= 0xf0;
				*cpu.joyp = (cpu.key_status & 0xf0) >> 4;
			}
			

			if ((frame_count_sec * 70224 + i) % 256 == 0) {
				(*cpu.div)++;
			}

			uint8_t interval = (*cpu.tac) & 0b00000011;
			uint16_t moduloInterval = 1024;
			if (interval > 0)
				moduloInterval >>= (8 - interval * 2);


			if (((frame_count_sec * 70224 + i) % moduloInterval == 0) && ((*cpu.tac >> 2) & 1)) {
				uint16_t val = *cpu.tima;
				if (val > 0 && val + 1 == 256) {
					*cpu.tima = *cpu.tma;
					set_interrupt_flag(&cpu, TIMER_IFLAG, 1);
				} else {
					(*cpu.tima)++;
				}
			}

			//#ifdef DEBUG
			/*if ((frame_count_sec * 70224 + i) % 132 == 0) {
				if (gb_read(&cpu, 0xFF02) == 0x81) {
					printf("%c", gb_read(&cpu, 0xFF01));
				}
			}*/
			//#endif

			if (load_bootstrap && !bootloader_mapped && cpu.address_space[0xFF50] == 1) {
				void *rom_start = read_bytes(rom, 0, 0x100);
				fseek(rom, 0, SEEK_SET);
				fread(cpu.address_space, 1, 0x100, rom);
				free(rom_start);

				bootloader_mapped = true;
			}

			
			if (cpu.instruction_wait_cycles == 0) {
				if (!cpu.shouldHalt) {
					if (cpu.queued_tasks) {
						for (unsigned char b = 0; b < 32; b++) {
							bool bit = (cpu.queued_tasks >> b) & 1;
							if (bit) {
								struct queue_task_data *task_data = cpu.queued_tasks_data[cpu.queued_tasks_data_ptr];
								
								if (task_data->cycles_until_execution == 0) {
									if (b == QUEUE_DMA_TRANSFER) {
										dma_transfer(&cpu, task_data->data);
									} else if (b == QUEUE_INTERRUPT_ENABLE) {
										cpu.ime = true;
									}
									free(task_data);
									cpu.queued_tasks &= ~(1UL << b);
									cpu.queued_tasks_data_ptr--;
								} else {
									task_data->cycles_until_execution--;
								}
							}
						}
					}
					uint32_t instruction = fetch_opcode(&cpu);
					//fprintf(someDump, "Executing instruction %06x, ", instruction);
					#ifdef DEBUG
					execute_instruction(&cpu, instruction, someDump);
					#else
					execute_instruction(&cpu, instruction, NULL);
					
					//printf("\r[REGISTERS]: PC: 0x%x, SP: 0x%x, AF: 0x%04x, BC: 0x%04x, DE: 0x%04x, HL: 0x%04x", cpu.pc, cpu.sp, *cpu.af, *cpu.bc, *cpu.de, *cpu.hl);
    				//fflush(stdout);
					#endif
					//fprintf(someDump, "cycle count delay %i\n", cpu.instruction_wait_cycles);
				}
			}
			check_interrupts(&cpu);
			cpu.instruction_wait_cycles--;
			if (dot_clock_wait > 1) {
				dot_clock_wait--;
				continue;
			}

			uint8_t vmode = *cpu.stat & 0b11;
			switch (vmode) {
				case 2: {
					set_interrupt_flag(&cpu, LCD_STAT_IFLAG, 1);
					dot_clock_wait = 80;
					vmode = 3;
					//ppu.mode = 2;
					break;
				}
				case 3: {
					set_interrupt_flag(&cpu, LCD_STAT_IFLAG, 1);
					
					dot_clock_wait = 172;
					vmode = 0;
					//ppu.mode = 3;
					break;
				}
				case 0: {
					set_interrupt_flag(&cpu, LCD_STAT_IFLAG, 1);
					perform_scanline(&ppu);
					dot_clock_wait = 204;
					vmode = 2;
					//ppu.mode = 0;

					if (ppu.scanline == 143) {
						set_interrupt_flag(&cpu, VBLANK_IFLAG, 1);
						update_display(&cpu, &ppu, texture, pixels, screen_on);
						//printfbin(*cpu.joyp);
						vmode = 1;
						//ppu.mode = 1;
						break;
					}
					ppu.scanline++;
					break;
				}
				case 1: {
					dot_clock_wait = 456;
					ppu.scanline++;
					if (ppu.scanline > 153) {
						vmode = 2;
						ppu.scanline = 0;
					}
					break;
				}
				
			}

			*cpu.stat &= 0b11111100;
			*cpu.stat |= vmode & 0b11;

			ppu.mode = vmode;
			*cpu.ly = ppu.scanline;
		}
		frame_count_sec++;
		if (frame_count_sec >= target_frame_rate) {
			frame_count_sec = 0;
		}

		frame_time = SDL_GetTicks() - frame_start;

		if (target_delay_time > frame_time) {
			SDL_Delay(target_delay_time - frame_time);
		}
	}

	#ifdef DEBUG
	fclose(someDump);
	fclose(adump);
	#endif
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(ppu.renderer);
	SDL_DestroyWindow(main_window);
	SDL_Quit();
	TTF_Quit();

	return 0;
}