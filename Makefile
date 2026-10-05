# Builds on macOS, Linux and Windows (MinGW / MSYS2: `make CC=gcc`)

CFLAGS ?= -O2
CFLAGS += -std=c99 -Wall -Wextra -pedantic

# Single-threaded app without extensions: no need for -lpthread / -ldl
SQLITE_FLAGS = -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION

TARGET = weapondb
ifeq ($(OS),Windows_NT)
    TARGET := $(TARGET).exe
endif

OBJS = main.o db.o menu.o tables.o crud.o platform.o sqlite3.o

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

sqlite3.o: sqlite3.c sqlite3.h
	$(CC) -O2 $(SQLITE_FLAGS) -c sqlite3.c -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

main.o:     main.c db.h menu.h sqlite3.h
db.o:       db.c db.h sqlite3.h
menu.o:     menu.c menu.h crud.h tables.h platform.h sqlite3.h
tables.o:   tables.c tables.h sqlite3.h
crud.o:     crud.c crud.h sqlite3.h
platform.o: platform.c platform.h

run: $(TARGET)
	./$(TARGET) Weapon.db

clean:
	rm -f $(OBJS) weapondb weapondb.exe

.PHONY: run clean
