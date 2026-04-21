SRCS = $(wildcard src/*.c)

default:
	gcc -o bin/main.exe $(SRCS) -I src -I include -L lib -lraylib -lgdi32 -lwinmm