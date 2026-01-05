CC      ?= gcc
CFLAGS  += -Wall -Wextra -I"./include"
LDFLAGS +=

NCURSES_INC ?=
NCURSES_LIB ?= -lncurses

ifeq ($(OS),Windows_NT)
    CFLAGS += -DNCURSES_STATIC -DNCURSES_WIDECHAR
endif

ifneq ($(NCURSES_INC),)
    CFLAGS += -I"$(NCURSES_INC)"
endif

LDFLAGS += -L"$(NCURSES_LIB)"

rogue: rogue.c
	$(CC) rogue.c $(CFLAGS) $(LDFLAGS) -lncurses -o $@

.PHONY: clean
clean:
	rm -f rogue rogue.exe
