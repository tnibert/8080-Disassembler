OBJS = emulator.c
#
all:
	gcc $(OBJS) -g -O0 -Wall -o 8080emu
