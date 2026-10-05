#ifndef CRUD_H
#define CRUD_H

#include <stddef.h>

#include "sqlite3.h"

/* All functions return -1 on SQL error; details are in sqlite3_errmsg(db). */

/* Creates a category. Returns its ID. */
int create_category(sqlite3 *db, const char *name);

/* Creates an item in a category. Returns its ID. */
int create_item(sqlite3 *db, int category_id, const char *name, int price);

/* Looks up item `id` inside a category. Returns 1 if found, 0 if not. */
int get_item(sqlite3 *db, int category_id, int id,
             char *name, size_t name_size, int *price);

/* Changes name and price of an item. Returns 1 if updated, 0 if not found. */
int update_item(sqlite3 *db, int id, const char *name, int price);

/* Puts item `id` of a category into the basket. Returns 1 if added, 0 if not found. */
int add_to_basket(sqlite3 *db, int category_id, int id);

/* Removes one copy of item `id` from the basket. Returns 1 if removed, 0 if not found. */
int remove_from_basket(sqlite3 *db, int id);

/* Removes everything from the basket. Returns number of removed items. */
int clear_basket(sqlite3 *db);

/* Stores the sum of basket prices in `total`. Returns 0 on success. */
int basket_total(sqlite3 *db, long long *total);

#endif /* CRUD_H */
