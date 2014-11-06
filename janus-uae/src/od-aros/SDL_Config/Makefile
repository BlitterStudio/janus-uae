# Project: SDL_Config
# Makefile created by Dev-C++ 4.9.9.2

CPP  = i386-aros-g++
CC   = i386-aros-gcc
WINDRES = 
RES  = 
OBJ  = SDL_config.o $(RES)
TESTOBJ = initest.o
TESTEXE = /home/oli/svn/v0/AROS/bin/linux-i386/AROS/initest
LINKOBJ  = SDL_config.o $(RES)
LIBS =   
INCS = -I/home/oli/svn/v0/AROS/bin/linux-i386/gen/include/SDL
CXXINCS =  -I/home/oli/svn/v0/AROS/bin/linux-i386/gen/include/SDL
BIN  = libSDL_Config.a
CPPFLAGS = $(INCS) -fexpensive-optimizations -O3
RM = rm -f


.PHONY: all all-before all-after clean clean-custom

all: $(BIN)

test: $(BIN) $(TESTOBJ)
	$(CPP) $(CPPFLAGS) $(TESTOBJ) -L. -lSDL_Config -lSDL -o $(TESTEXE) 

clean:
	${RM} $(OBJ) $(BIN) $(TESTOBJ)

$(BIN): $(LINKOBJ)
	ar r $(BIN) $(LINKOBJ)
	ranlib $(BIN)

%.o: %.c
	$(CPP) $(CPPFLAGS) -o $@ -c $*.c
