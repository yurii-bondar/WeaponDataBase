#ifndef TABLES_H
#define TABLES_H

#include "sqlite3.h"

#define MAX_CATEGORY_NAME 48
#define MAX_ITEM_NAME     48

typedef struct {
    int  id;
    char name[MAX_CATEGORY_NAME + 1];
} Category;

/* Loads all categories ordered by ID into a malloc'ed array (caller frees *list).
   Returns number of categories or -1 on error. */
int load_categories(sqlite3 *db, Category **list);

/* Prints all items of a category. Returns number of rows or -1 on error. */
int print_items(sqlite3 *db, const Category *category);

/* Prints basket contents. Returns number of rows or -1 on error. */
int print_basket(sqlite3 *db);

#endif /* TABLES_H */
