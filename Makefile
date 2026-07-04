SRCS = $(wildcard src/*.c)

CFLAGS  ?=
LDLIBS   = -lraylib -lgdi32 -lwinmm
INCLUDES = -I src -I include -L lib

default: bin/main.exe

bin/main.exe: $(SRCS) bin/icon.o
	gcc $(CFLAGS) -o $@ $(SRCS) bin/icon.o $(INCLUDES) $(LDLIBS)

bin/icon.o: assets/icon.rc assets/icon.ico
	windres assets/icon.rc -o $@
