#include <SDL2/SDL.h>
#include "cpu.h"
#include "ppu.h"

extern void _halt();

void init_ppu(gb_ppu_t *ppu, gb_cpu_t *cpu, uint32_t pixels[INTERNAL_SCREEN_HEIGHT][INTERNAL_SCREEN_WIDTH]) {
    ppu->cpu = cpu;
    ppu->oam = cpu->address_space + 0xFE00;
    ppu->vram = cpu->vram;

    static bool color_type = 0; //0 (white-black), 1 (greens), wip
    if (color_type == 0) {
        ppu->color_palette[3] = 0;
        ppu->color_palette[2] = 96;
        ppu->color_palette[1] = 192;
        ppu->color_palette[0] = 255;
    }

    ppu->pixel_buf = pixels;
    ppu->scanline = 0;
}

void draw_screen(gb_ppu_t *ppu) {
    /*uint8_t scx = *ppu->cpu->scx;
    uint8_t scy = *ppu->cpu->scy;
    for (int y = 0; y < 144; y++) {

        for (int x = 0; x < 160; x++) {
            uint16_t offsetX = x + scx;
            uint16_t offsetY = y + scx;
            if (offsetX > 255) {
                offsetX -= 255;
            }
            if (offsetY > 255) {
                offsetY -= 255;
            }
            //uint8_t drawCol = (rand() % 4) & 0xff;
            //ppu->pixel_buf[y][x] = (((0xFF << 24) | (ppu->color_palette[drawCol] << 16) | (ppu->color_palette[drawCol] << 8)) | ppu->color_palette[drawCol]);
            ppu->pixel_buf[y][x] = ppu->background_buf[offsetY][offsetX];
        }
    }*/

    SDL_RenderPresent(ppu->renderer);
}

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')


static inline uint8_t translate_palette_bg(gb_ppu_t *ppu, uint8_t col) {
    return (*ppu->cpu->bgp >> col * 2) & 0b00000011;
}


static uint8_t translate_palette_sprite(gb_ppu_t *ppu, uint8_t col, uint8_t palette) {
    if (col == 0)
        return 0;
    if (palette)
        return (*ppu->cpu->obp1 >> col * 2) & 0b00000011;
    else
        return (*ppu->cpu->obp0 >> col * 2) & 0b00000011;
}

void draw_window_scanline(gb_ppu_t *ppu, uint8_t *tile_data) {
    //printf("wx, wy: %x, %x\n", *ppu->cpu->wx, *ppu->cpu->wy);
    uint8_t *window_tile_map = (*(ppu->cpu->lcdc) >> 6) & 1 ? (uint8_t *)(ppu->vram + 0x1C00) : (uint8_t *)(ppu->vram + 0x1800);
    if (ppu->scanline >= *ppu->cpu->wy) {
        for (int i = 0; i < 0x14; i++) {
            uint8_t offset = *(window_tile_map + (((ppu->scanline) / 8) % 32) * 32 + (((0 / 8) + i) % 32));

            int16_t base = 0;

            if (!((*(ppu->cpu->lcdc) >> 6) & 1)) {
                if (offset > 128) {
                    base = -0x800;
                } else {
                    base = 0x800;
                }
            }

            uint16_t finalOffset = base + offset * 16 + (((ppu->scanline + 0) % 8) * 2);
            uint8_t top = tile_data[finalOffset];
            uint8_t bottom = tile_data[finalOffset + 1];

            for (int x = 0; x < 8; x++) {
                uint8_t pixelCol = (((bottom >> (7 - x)) & 1) << 1) | ((top >> (7 - x)) & 1);
                uint8_t actualCol = translate_palette_bg(ppu, pixelCol);
                //SDL_SetRenderDrawColor(ppu->renderer, color, color, color, 255);
                uint8_t pixelOffset = i * 8 + x;

                //pixel.x = ((i * 8) + x) * 3;
               ppu->pixel_buf[ppu->scanline][pixelOffset] = ((0xFF << 24) | (ppu->color_palette[actualCol] << 16) | (ppu->color_palette[actualCol] << 8) | ppu->color_palette[actualCol]);
            }
        }
    }
}
bool should_draw_window(gb_ppu_t *ppu) {
    bool status = false;
    if ((*ppu->cpu->lcdc >> 5) & 1)
        status = true;
    return status;
}

int current_sprite_size(gb_ppu_t *ppu) {
    int status = 0;
    if ((*ppu->cpu->lcdc >> 2) & 1)
        status = 1;
    return status;
}
void perform_scanline(gb_ppu_t *ppu) {
    ppu->during_hblank = false;

    //printf("executing with scanline: %i\n", ppu->scanline);
    uint8_t *tile_data = (*(ppu->cpu->lcdc) >> 4) & 1 ? (uint8_t *)(ppu->vram) : (uint8_t *)(ppu->vram + 0x800);
    uint8_t *bg_tile_map = (*(ppu->cpu->lcdc) >> 3) & 1 ? (uint8_t *)(ppu->vram + 0x1C00) : (uint8_t *)(ppu->vram + 0x1800);

    /*for (int i = 0; i < 0x20; i++) {

        uint8_t offset = *(bg_tile_map + (((ppu->scanline + *ppu->cpu->scy) / 8) % 32) * 32 + (((*ppu->cpu->scx / 8) + i) % 32));
        int16_t base = 0;

        if (!((*(ppu->cpu->lcdc) >> 4) & 1)) {
            if (offset > 128) {
                base = -0x800;
            } else {
                base = 0x800;
            }
        }
        
        uint16_t finalOffset = base + offset * 16 + (((ppu->scanline + *ppu->cpu->scy) % 8) * 2);
        uint8_t top = tile_data[finalOffset];
        uint8_t bottom = tile_data[finalOffset + 1];

        for (int x = 0; x < 8; x++) {
            uint8_t pixelCol = (((bottom >> (7 - x)) & 1) << 1) | ((top >> (7 - x)) & 1);
            uint8_t actualCol = translate_palette_bg(ppu, pixelCol);

            uint8_t pixelOffset = i * 8 + x;
            //pixelOffset = 0;

            uint8_t base = *ppu->cpu->scx + x;
            uint8_t finalOffset = base + 160;
            //scx currently messed, scy works as expected
            //if ((pixelOffset == 0 || (pixelOffset % 160) != 0) && pixelOffset < 160) {
            ppu->pixel_buf[ppu->scanline][pixelOffset] = ((0xFF << 24) | (ppu->color_palette[actualCol] << 16) | (ppu->color_palette[actualCol] << 8) | ppu->color_palette[actualCol]);
            //}
            //if 0 >= 
        }
    }*/
      //  uint8_t *tile_data = (*(ppu->cpu->lcdc) >> 4) & 1 ? (uint8_t *)(ppu->vram) : (uint8_t *)(ppu->vram + 0x800);
    //uint8_t *bg_tile_map = (*(ppu->cpu->lcdc) >> 3) & 1 ? (uint8_t *)(ppu->vram + 0x1C00) : (uint8_t *)(ppu->vram + 0x1800);
    //uint8_t *window_tile_map = (*(ppu->cpu->lcdc) >> 6) & 1 ? (uint8_t *)(ppu->vram + 0x1C00) : (uint8_t *)(ppu->vram + 0x1800);
    
    for (int i = 0; i < 0x20; i++) {

        uint8_t offset = *(bg_tile_map + (((ppu->scanline + *ppu->cpu->scy) / 8) % 32) * 32 + (((*ppu->cpu->scx / 8) + i) % 32));
        int16_t base = 0;

        if (!((*(ppu->cpu->lcdc) >> 4) & 1)) {
            if (offset > 128) {
                base = -0x800;
            } else {
                base = 0x800;
            }
        }
        
        uint16_t finalOffset = base + offset * 16 + (((ppu->scanline + *ppu->cpu->scy) % 8) * 2);
        uint8_t top = tile_data[finalOffset];
        uint8_t bottom = tile_data[finalOffset + 1];

        for (int x = 0; x < 8; x++) {
            uint8_t pixelCol = (((bottom >> (7 - x)) & 1) << 1) | ((top >> (7 - x)) & 1);
            uint8_t actualCol = translate_palette_bg(ppu, pixelCol);

            uint8_t pixelOffset = i * 8 + x;
            //pixelOffset = 0;

            uint8_t base = *ppu->cpu->scx + x;
            uint8_t finalOffset = base + 160;
            //scx currently messed, scy works as expected
            if ((pixelOffset == 0 || (pixelOffset % 160) != 0) && pixelOffset < 160) {
                ppu->pixel_buf[ppu->scanline][pixelOffset] = ((0xFF << 24) | (ppu->color_palette[actualCol] << 16) | (ppu->color_palette[actualCol] << 8) | ppu->color_palette[actualCol]);
            }
            //if 0 >= 
        }
    }
    if (should_draw_window(ppu)) {
       // draw_window_scanline(ppu, tile_data);
    }


    // render sprites
    /*
    for (uint8_t i = 0; i < 40; i += 4) {
        //uint8_t offset = *(bg_tile_map + ((ppu->scanline + *ppu->cpu->scy ) / 8) * 32 + (*ppu->cpu->scx / 8) + i);

        uint8_t attributes = ppu->oam[i + 3];
        //printf("addr: %p\n", ppu->oam - ppu->cpu->addressSpace + i);
        uint8_t spriteY = ppu->oam[i];
        uint8_t spriteX = ppu->oam[i + 1];
        uint8_t tileOffset = ppu->oam[i + 2];

        if (attributes || spriteY || spriteX || tileOffset) {
            bool reverseHorizontal = (attributes >> 5) & 1;
            int finalOffset = tileOffset * 16 + (((ppu->scanline) % 8) * 2);

            uint8_t top = ppu->vram[finalOffset];
            uint8_t bottom = ppu->vram[finalOffset + 1];

            //if (current_sprite_size(ppu) == 0) {   
                if (spriteY >= ppu->scanline + 8 && spriteY <= ppu->scanline + 16) {
                    for (int x = 0; x < 8; x++) {
                        uint8_t transparent = false;
                        //00011111
                        uint8_t pixelCol = (((bottom >> (7 - x)) & 1) << 1) | ((top >> (7 - x)) & 1);
                        uint8_t actualCol = translate_palette_sprite(ppu, pixelCol, (attributes >> 4) & 1);
                        if (actualCol == 0) {
                            transparent = true;
                        }
                        if (!transparent) {
                            uint8_t x_offset = spriteX + x - 8;
                            if (x_offset < 160)
                                ppu->pixel_buf[ppu->scanline][x_offset] = ((0x00 << 24) | (ppu->color_palette[actualCol] << 16) | (ppu->color_palette[actualCol] << 8) | ppu->color_palette[actualCol]);
                        }
                    }
                }
            //} else {
                if (spriteY >= ppu->scanline + 8 && spriteY <= ppu->scanline + 16) {
                    for (int x = 0; x < 8; x++) {
                        uint8_t transparent = false;
                        //00011111
                        uint8_t pixelCol = (((bottom >> (7 - x)) & 1) << 1) | ((top >> (7 - x)) & 1);
                        uint8_t actualCol = translate_palette_sprite(ppu, pixelCol, (attributes >> 4) & 1);
                        if (actualCol == 0) {
                            transparent = true;
                        }
                        if (!transparent) {
                            uint8_t x_offset = spriteX + x - 8;
                            if (x_offset < 160)
                                ppu->pixel_buf[ppu->scanline][x_offset] = ((0x00 << 24) | (ppu->color_palette[actualCol] << 16) | (ppu->color_palette[actualCol] << 8) | ppu->color_palette[actualCol]);
                        }
                    }
                }
            //}
        }
    }*/
    for (uint8_t i = 0; i < 0x9F; i += 4) {
        //uint8_t offset = *(bg_tile_map + ((ppu->scanline + *ppu->cpu->scy ) / 8) * 32 + (*ppu->cpu->scx / 8) + i);

        uint8_t attributes = ppu->oam[i + 3];
        //printf("addr: %p\n", ppu->oam - ppu->cpu->addressSpace + i);
        uint8_t spriteY = ppu->oam[i];
        uint8_t spriteX = ppu->oam[i + 1];
        uint8_t tileOffset = ppu->oam[i + 2];

        if (attributes || spriteY || spriteX || tileOffset) {

            int finalOffset = tileOffset * 16 + (((ppu->scanline + *ppu->cpu->scy) % 8) * 2);

            uint8_t top = ppu->vram[finalOffset];
            uint8_t bottom = ppu->vram[finalOffset + 1];

            if (spriteY - 8 >= ppu->scanline && spriteY - 16 <= ppu->scanline) {
                for (int x = 0; x < 8; x++) {
                    uint8_t transparent = false;
                    //00011111
                    uint8_t pixelCol = (((bottom >> (7 - x)) & 1) << 1) | ((top >> (7 - x)) & 1);
                    uint8_t actualCol = translate_palette_sprite(ppu, pixelCol, (attributes >> 4) & 1);
                    if (actualCol == 0) {
                       transparent = true;
                    }
                    if (!transparent)
                        ppu->pixel_buf[ppu->scanline][(spriteX + x - 8) % 160] = ((0x00 << 24) | (ppu->color_palette[actualCol] << 16) | (ppu->color_palette[actualCol] << 8) | ppu->color_palette[actualCol]);
                }
            }
        }
    }
    // next draw window
    ppu->during_hblank = true; 
}
