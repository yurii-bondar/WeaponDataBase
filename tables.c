#include <stdio.h>
#include <stdlib.h>

#include "tables.h"

#define LINE "--------------------------------------------------------------\n"

int load_categories(sqlite3 *db, Category **list)
{
    sqlite3_stmt *stmt;
    Category *items = NULL;
    int count = 0, capacity = 0, rc;

    *list = NULL;
    if (sqlite3_prepare_v2(db, "SELECT ID, Name FROM Categories ORDER BY ID",
                           -1, &stmt, NULL) != SQLITE_OK)
        return -1;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned char *name = sqlite3_column_text(stmt, 1);

        if (count == capacity) {
            Category *grown;
            capacity = capacity ? capacity * 2 : 16;
            grown = realloc(items, capacity * sizeof *items);
            if (grown == NULL) {
                rc = SQLITE_NOMEM;
                break;
            }
            items = grown;
        }

        items[count].id = sqlite3_column_int(stmt, 0);
        snprintf(items[count].name, sizeof items[count].name, "%s",
                 name ? (const char *)name : "");
        count++;
    }
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        free(items);
        return -1;
    }

    *list = items;
    return count;
}

/* Prints rows of (ID, Name, Price). Returns number of rows or -1 on error. */
static int print_rows(sqlite3 *db, sqlite3_stmt *stmt)
{
    int rc, rows = 0;

    printf("%-6s%-48s%s\n", "<ID>", "<NAME>", "<PRICE>");
    printf(LINE);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned char *name = sqlite3_column_text(stmt, 1);
        printf("%-6d%-48s$ %d\n",
               sqlite3_column_int(stmt, 0),
               name ? (const char *)name : "",
               sqlite3_column_int(stmt, 2));
        rows++;
    }

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    if (rows == 0)
        printf("      (empty)\n");
    return rows;
}

int print_items(sqlite3 *db, const Category *category)
{
    sqlite3_stmt *stmt;
    int rows;

    printf("\n                  < %s >\n\n", category->name);

    if (sqlite3_prepare_v2(db,
            "SELECT ID, Name, Price FROM Items WHERE CategoryID = ? ORDER BY ID",
            -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_int(stmt, 1, category->id);
    rows = print_rows(db, stmt);
    sqlite3_finalize(stmt);
    return rows;
}

int print_basket(sqlite3 *db)
{
    sqlite3_stmt *stmt;
    int rows;

    printf("\n                  < BASKET >\n\n");

    if (sqlite3_prepare_v2(db,
            "SELECT i.ID, i.Name, i.Price FROM Basket b "
            "JOIN Items i ON i.ID = b.ItemID ORDER BY b.ID",
            -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    rows = print_rows(db, stmt);
    sqlite3_finalize(stmt);
    return rows;
}
