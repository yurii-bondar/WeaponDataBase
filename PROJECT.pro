TEMPLATE = app
CONFIG   += console
CONFIG   -= app_bundle
CONFIG   -= qt

TARGET   = weapondb

DEFINES  += SQLITE_THREADSAFE=0 SQLITE_OMIT_LOAD_EXTENSION

SOURCES  += \
    sqlite3.c \
    main.c \
    db.c \
    menu.c \
    tables.c \
    crud.c \
    platform.c

HEADERS  += \
    sqlite3.h \
    db.h \
    menu.h \
    tables.h \
    crud.h \
    platform.h
