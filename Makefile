CC = cc
EXE = i3tg
LIBS += $(shell pkg-config --libs --cflags libsystemd)
CFLAGS += -O2 -std=c11 -Wall -pedantic $(LIBS)

.PHONY: fmt clean valgrind

i3tg: main.c
	$(CC) $(CFLAGS) -o $@ $<

fmt:
	clang-format -i *.c

clean:
	rm -f $(EXE)

valgrind: i3tg
	valgrind ./i3tg

