SRCS = $(wildcard src/*.c)

CFLAGS  ?=
LDLIBS   = -lraylib -lgdi32 -lwinmm
INCLUDES = -I src -I include -L lib

default: bin/main.exe

bin/main.exe: $(SRCS)
	gcc $(CFLAGS) -o $@ $(SRCS) $(INCLUDES) $(LDLIBS)
