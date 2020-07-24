#ifndef INPUT_INCLUDE
#define INPUT_INCLUDE
#include <stdio.h>
#include <SDL2/SDL.h>
#include "cpu.h"
#ifdef __cplusplus
 extern "C"
void handle_keypress(SDL_KeyboardEvent *e, gb_cpu_t *cpu);
#endif
