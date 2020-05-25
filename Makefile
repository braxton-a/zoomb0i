CC=gcc

main: cpu.c ppu.c main.c utils.c fcns.s
	$(CC) -g -O2 -o zoomb0i $(wildcard *.c) $(wildcard *.s) -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -I.