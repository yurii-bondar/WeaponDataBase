#ifndef DB_H
#define DB_H

#include "sqlite3.h"

/* Opens an existing database file and brings its schema up to date
   (the old one-table-per-category layout is migrated automatically).
   Returns NULL on error; the reason is printed to stderr. Close with sqlite3_close(). */
sqlite3 *db_open(const char *path);

#endif /* DB_H */
