#include <stdio.h>

#include "db.h"

#define SCHEMA_VERSION 1

#define CREATE_TABLES                                                         \
    "CREATE TABLE Categories ("                                               \
    "    ID   INTEGER PRIMARY KEY AUTOINCREMENT,"                             \
    "    Name TEXT NOT NULL UNIQUE COLLATE NOCASE"                                        \
    ");"                                                                      \
    "CREATE TABLE Items ("                                                    \
    "    ID         INTEGER PRIMARY KEY AUTOINCREMENT,"                       \
    "    CategoryID INTEGER NOT NULL REFERENCES Categories(ID),"              \
    "    Name       TEXT    NOT NULL,"                                        \
    "    Price      INTEGER NOT NULL CHECK (Price >= 0)"                      \
    ");"

#define CREATE_BASKET                                                         \
    "CREATE TABLE Basket ("                                                   \
    "    ID     INTEGER PRIMARY KEY AUTOINCREMENT,"                           \
    "    ItemID INTEGER NOT NULL REFERENCES Items(ID)"                        \
    ");"

/* Empty database file -> fresh schema */
static const char *SCHEMA_NEW =
    "BEGIN;"
    CREATE_TABLES
    CREATE_BASKET
    "PRAGMA user_version = 1;"
    "COMMIT;";

/* Version 0: one table per category, item names padded like "Name-------$" */
#define COPY_ITEMS(category, table)                                           \
    "INSERT INTO Items (ID, CategoryID, Name, Price) "                        \
    "SELECT ID, " #category ", trim(rtrim(Name, '-$')), Price "               \
    "FROM " table " ORDER BY rowid;"

static const char *SCHEMA_MIGRATE_V0 =
    "BEGIN;"
    CREATE_TABLES
    "INSERT INTO Categories (ID, Name) VALUES"
    "    (1, 'Throwing weapon'),"
    "    (2, 'Pneumatic weapon'),"
    "    (3, 'Traumatic weapon'),"
    "    (4, 'Hunting weapon'),"
    "    (5, 'Military weapon'),"
    "    (6, 'Arrows & Bullets'),"
    "    (7, 'Sights');"
    COPY_ITEMS(1, "Throwing_weapon")
    COPY_ITEMS(2, "Pneumatic_weapon")
    COPY_ITEMS(3, "Traumatic_weapon")
    COPY_ITEMS(4, "Hunting_weapon")
    COPY_ITEMS(5, "Military_weapon")
    COPY_ITEMS(6, "Ammunition")
    COPY_ITEMS(7, "Sights")
    "ALTER TABLE Basket RENAME TO Basket_old;"
    CREATE_BASKET
    "INSERT INTO Basket (ItemID) "
    "SELECT ID FROM Basket_old WHERE ID IN (SELECT ID FROM Items) ORDER BY rowid;"
    "DROP TABLE Basket_old;"
    "DROP TABLE Throwing_weapon;"
    "DROP TABLE Pneumatic_weapon;"
    "DROP TABLE Traumatic_weapon;"
    "DROP TABLE Hunting_weapon;"
    "DROP TABLE Military_weapon;"
    "DROP TABLE Ammunition;"
    "DROP TABLE Sights;"
    "PRAGMA user_version = 1;"
    "COMMIT;";

static int query_int(sqlite3 *db, const char *sql, int *out)
{
    sqlite3_stmt *stmt;
    int rc;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
        return -1;

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
        *out = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    return rc == SQLITE_ROW ? 0 : -1;
}

static int run_script(sqlite3 *db, const char *sql)
{
    char *err = NULL;

    if (sqlite3_exec(db, sql, NULL, NULL, &err) == SQLITE_OK)
        return 0;

    fprintf(stderr, "Database migration failed: %s\n", err ? err : sqlite3_errmsg(db));
    sqlite3_free(err);
    sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
    return -1;
}

static int migrate(sqlite3 *db)
{
    int version, legacy;

    if (query_int(db, "PRAGMA user_version", &version) != 0)
        return -1;

    if (version == SCHEMA_VERSION)
        return 0;

    if (version != 0) {
        fprintf(stderr, "Unknown database version %d (expected %d)\n", version, SCHEMA_VERSION);
        return -1;
    }

    if (query_int(db,
                  "SELECT count(*) FROM sqlite_master "
                  "WHERE type = 'table' AND name = 'Throwing_weapon'",
                  &legacy) != 0)
        return -1;

    return run_script(db, legacy ? SCHEMA_MIGRATE_V0 : SCHEMA_NEW);
}

sqlite3 *db_open(const char *path)
{
    sqlite3 *db = NULL;
    int rc;

    /* READWRITE without CREATE: a wrong path is an error, not a new empty database */
    rc = sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Can't open database '%s': %s\n",
                path, db ? sqlite3_errmsg(db) : sqlite3_errstr(rc));
        sqlite3_close(db);   /* must be released even when open failed */
        return NULL;
    }

    if (sqlite3_exec(db, "PRAGMA foreign_keys = ON", NULL, NULL, NULL) != SQLITE_OK
        || migrate(db) != 0) {
        fprintf(stderr, "Can't prepare database '%s': %s\n", path, sqlite3_errmsg(db));
        sqlite3_close(db);
        return NULL;
    }

    return db;
}
