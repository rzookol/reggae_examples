# $VER: makefile 50.6 (10.2.2007)
 
CC = ppc-morphos-gcc
CFLAGS += -noixemul -nostartfiles -s -O2 -fomit-frame-pointer -mno-prototype -mcpu=604e
CFLAGS += -W -Wall -Wpointer-arith -Wno-parentheses
LD = ppc-morphos-gcc
LDFLAGS = -noixemul 
LIBS = -ldebug
STRIP = ppc-morphos-strip -g --strip-unneeded --remove-section .comment
OUTPUT = ProgressiveTester
OBJS = progressive_tester.o 

#===============================================================================

.PHONY: all dist clean install install-iso sdk

all: $(OUTPUT) $(OUTPUT).db

dist: $(OUTPUT) $(OUTPUT).db

clean:
	-rm -rf $(OBJS) *.bak *.s *.db $(OUTPUT)

install:
	echo "Disabled for now."

install-iso:
	echo "Disabled for now."

sdk:

#===============================================================================

$(OUTPUT).db: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o $(OUTPUT).db

$(OUTPUT): $(OBJS)
	$(LD) -s $(LDFLAGS) $(OBJS) $(LIBS) -o $(OUTPUT).tmp
	$(STRIP) $(OUTPUT).tmp -o $(OUTPUT)
	rm $(OUTPUT).tmp
	chmod u+x $(OUTPUT)

%.o:
	@echo "Compiling $@..."
	@$(CC) -c $(CFLAGS) -o $@ $<

progressive_tester.o: progressive_tester.c
