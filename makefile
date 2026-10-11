CC = g++
CFLAGS = -fPIC -O2 -I. -DLINUX -static-libgcc -static-libstdc++ -Wno-narrowing -Wno-multichar

all: nsf2sapr

nsf2sapr: makefile main.cpp NSF_Core.cpp NSF_Core.h NSF_File.cpp NSF_File.h NSF_6502.cpp Wave_VRC7.cpp fmopl.c
	$(CC) $(CFLAGS) main.cpp NSF_Core.cpp NSF_File.cpp NSF_6502.cpp Wave_VRC7.cpp fmopl.c -o nsf2sapr
