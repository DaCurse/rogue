CC      ?= gcc
CFLAGS  += -Wall -Wextra -I"./include"
LDFLAGS +=
LDLIBS  += -lncursesw
SRCS = rogue.c terminal.c color.c enemy.c floor.c game.c item.c systems.c utils.c world.c
OBJS = $(SRCS:.c=.o)

NCURSES_INC ?=
NCURSES_LIB ?=

ifeq ($(OS),Windows_NT)
    CFLAGS += -DNCURSES_STATIC -DNCURSES_WIDECHAR
    LDFLAGS += -static -static-libgcc
endif

ifneq ($(NCURSES_INC),)
    CFLAGS += -I"$(NCURSES_INC)"
endif

ifneq ($(NCURSES_LIB),)
    LDFLAGS += -L"$(NCURSES_LIB)"
endif

ifdef reveal_map
    CFLAGS += -DDEBUG_REVEAL_MAP
endif

ifdef paint_roads
    CFLAGS += -DDEBUG_PAINT_ROADS
endif

rogue: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

release: CFLAGS += -O2 -DNDEBUG -flto -s -ffunction-sections -fdata-sections
release: LDFLAGS += -Wl,--gc-sections
release: rogue

.PHONY: clean
clean:
	rm -f rogue rogue.exe *.o
