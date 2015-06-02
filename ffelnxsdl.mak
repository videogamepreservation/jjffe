# Suffices used in implicit rules
.SUFFIXES: .asm .cpp .c .d

OBJDIR = linux/
OUTFILE = ffelnxsdl
CONVFILE = $(OBJDIR)lnxconv

# Search paths for source files
NONSYSDIRS = 
SYSDIRS = linux sdl

# Source files
NONSYSFILES = ffelnx.asm ffebmp.asm ffemisc.c ffecfg.c
SYSFILES =  sdlvideo.cpp sdlsound.cpp sdlinput.cpp sdltimer.cpp linuxdir.c

# Modify this for different systems
SRCDIRS = $(NONSYSDIRS) $(SYSDIRS)
SRCFILES = $(NONSYSFILES) $(SYSFILES)
SYSFLAGS = -I.

vpath %.asm $(SRCDIRS)
vpath %.c $(SRCDIRS)
vpath %.cpp $(SRCDIRS)

ASMFILES = $(filter %.asm,$(SRCFILES))
CFILES = $(filter %.c,$(SRCFILES))
CPPFILES = $(filter %.cpp,$(SRCFILES))
OBJFILES = $(ASMFILES:%.asm=$(OBJDIR)%.o) $(CFILES:%.c=$(OBJDIR)%.o)\
	$(CPPFILES:%.cpp=$(OBJDIR)%.o)
DEPFILES = $(CFILES:%.c=$(OBJDIR)%.d)

# Flags variables
override CFLAGS += $(SYSFLAGS) -Wall -D__FFELNXSDL__
LINKFLAG = -Wl,-Map,map.log
NASMFLAGS = -f elf -i.
CC = gcc
CXX = g++
XTRALIBS = -lstdc++ $(shell sdl-config --libs) -lGL
override CFLAGS += $(shell sdl-config --cflags)

# Targets
all: $(OUTFILE) 
debug:
	$(MAKE) CFLAGS=-g
profile:
	$(MAKE) CFLAGS=-pg
opt:
	$(MAKE) CFLAGS=-O3
clean:
	rm -f $(OBJDIR)*.o
	rm -f $(OBJDIR)ffelnx.asm
	rm -f $(OUTFILE)
	rm -f $(CONVFILE)

# Nasty ffelnx.asm creation
$(OBJDIR)ffelnx.asm: ffe.asm $(CONVFILE)
	$(CONVFILE) ffe.asm $@
$(OBJDIR)ffelnx.o : $(OBJDIR)ffelnx.asm
	nasm $(NASMFLAGS) -o $@ $<
$(CONVFILE): lnxconv.c
	$(CC) $(CFLAGS) $(LINKFLAG) -o $@ $^

# Implicit rules
$(OBJDIR)%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<
$(OBJDIR)%.o : %.cpp
	$(CXX) $(CFLAGS) -c -o $@ $<
$(OBJDIR)%.o : %.asm
	nasm $(NASMFLAGS) -o $@ $<

# Executables
$(OUTFILE): $(OBJFILES)
	$(CXX) $(CFLAGS) $(LINKFLAG) -o $@ $^ $(XTRALIBS)

