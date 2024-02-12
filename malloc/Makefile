CC = gcc
CPPFLAGS = -D_DEFAULT_SOURCE
CFLAGS = -Wall -Wextra -Werror -std=c99 -Wvla
LDFLAGS = -shared
VPATH = src

TARGET_LIB = libmalloc.so
OBJS = src/malloc.o src/utils.o
OBJMAIN = src/main.o
TESTNAME = tests/tests.sh

all: library

library: $(TARGET_LIB)
$(TARGET_LIB): CFLAGS += -pedantic -fvisibility=hidden -fPIC
$(TARGET_LIB): LDFLAGS += -Wl,--no-undefined
$(TARGET_LIB): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

debug: CFLAGS += -g
debug: clean $(TARGET_LIB)

mainlib: library
	$(CC) src/main.c $(CFLAGS) -L. -lmalloc -o main

maindeb: debug
	$(CC) src/main.c $(CFLAGS) -g -fsanitize=address -L. -lmalloc -o main

check:
	./$(TESTNAME)

clean:
	$(RM) $(TARGET_LIB) $(OBJS) $(OBJMAIN) main

.PHONY: all $(TARGET_LIB) clean
