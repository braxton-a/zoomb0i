#ifndef ppu_h
#define ppu_h

#define SCALE 3

#define INTERNAL_SCREEN_WIDTH 160
#define INTERNAL_SCREEN_HEIGHT 144

#define VISIBLE_SCREEN_WIDTH INTERNAL_SCREEN_WIDTH*SCALE
#define VISIBLE_SCREEN_HEIGHT INTERNAL_SCREEN_HEIGHT*SCALE


#include <stdio.h>
#include <math.h>
#include <SDL2/SDL.h>
#include "cpu.h"
#include "utils.h"

struct gb_cpu;

struct gb_ppu {
    uint8_t *vram;
    uint8_t *oam;
    uint32_t (*pixel_buf)[INTERNAL_SCREEN_WIDTH];
    //uint32_t win_buf[144][160];
    uint8_t color_palette[4];
    uint8_t mode;
    uint8_t scanline;
    bool during_hblank;
    SDL_Renderer *renderer;
    struct gb_cpu *cpu;
};

typedef struct gb_ppu gb_ppu_t;

void init_ppu(gb_ppu_t *ppu, struct gb_cpu *cpu, uint32_t pixels[INTERNAL_SCREEN_HEIGHT][INTERNAL_SCREEN_WIDTH]);
void draw_screen(gb_ppu_t *ppu);
void perform_scanline(gb_ppu_t *ppu);
#endif