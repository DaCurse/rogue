CC      ?= gcc
CFLAGS  += -Wall -Wextra -I"./include"
LDFLAGS +=

NCURSES_INC ?=
NCURSES_LIB ?= 

ifeq ($(OS),Windows_NT)
    CFLAGS += -DNCURSES_STATIC -DNCURSES_WIDECHAR
endif

ifneq ($(NCURSES_INC),)
    CFLAGS += -I"$(NCURSES_INC)"
endif

ifneq ($(NCURSES_LIB),)
	LDFLAGS += -L"$(NCURSES_LIB)"
endif

rogue: rogue.c
	$(CC) rogue.c $(CFLAGS) $(LDFLAGS) -lncurses -o $@

.PHONY: clean
clean:
	rm -f rogue rogue.exe
