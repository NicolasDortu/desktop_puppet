SRCS = $(wildcard src/*.c)
CFLAGS ?=

default:
	gcc $(CFLAGS) -o bin/main.exe $(SRCS) -I src -I include -L lib -lraylib -lgdi32 -lwinmm