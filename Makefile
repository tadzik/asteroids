CC = clang
CFLAGS = -ggdb -std=c99 -pedantic -Wall -Wextra $(shell pkg-config --cflags sdl)
LIBS = $(shell pkg-config --libs sdl SDL_gfx) -lm -lSDL_ttf -lSDL_mixer

main: main.c
	$(CC) -c $(CFLAGS) main.c
	$(CC) $(LIBS) main.o -o main

clean:
	rm -f *.o main
