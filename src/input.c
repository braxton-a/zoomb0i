#include "input.h"

static inline void set_bit_on(uint8_t *value, uint8_t bit, uint8_t status) {
	*value ^= (-status ^ *value) & (1UL << bit);
}


void handle_keypress(SDL_KeyboardEvent *e, gb_cpu_t *cpu) {
    int key = e->keysym.sym;
    static int keys[] = {SDLK_a, SDLK_b, SDLK_o, SDLK_p, SDLK_RIGHT, SDLK_LEFT, SDLK_UP, SDLK_DOWN};
    uint8_t keyStatus = e->type == SDL_KEYDOWN ? 1 : 0;
    /*if (key == SDLK_RIGHT || key == SDLK_LEFT || key == SDLK_UP || key == SDLK_DOWN) {
        //*cpu->joyp |= 
    }*/
    uint8_t joyp = *cpu->joyp;

    int mode;
    if ((joyp >> 4) & 1) {
        mode = 1;
    } else {
        mode = 0;
    }

    int type = 1; 
    if (e->type == SDL_KEYDOWN) {
        type = 0;
    }
    int used = 0;
    for (int i = 0; i < 0x8; i++) {
        if (key == keys[i]) {;
            set_bit_on(&cpu->key_status, i, type);
        } else {
            set_bit_on(&cpu->key_status, i, 1);
        }
    }
    /*if (mode == 0) {
        *cpu->joyp &= 0xf0;
        *cpu->joyp |= cpu->key_status & 0xf;
    } else {
        *cpu->joyp &= 0xf0;
        *cpu->joyp |= (cpu->key_status & 0xf0) >> 4;
    }
    *cpu->joyp = 0b00010111;*/
}

