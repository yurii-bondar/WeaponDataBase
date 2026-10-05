#include <stdio.h>

#include "crud.h"

/* Finishes a write statement. Returns number of changed rows or -1. */
static int finish(sqlite3 *db, sqlite3_stmt *stmt)
{
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE ? sqlite3_changes(db) : -1;
}

int create_category(sqlite3 *db, const char *name)
{
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, "INSERT INTO Categories (Name) VALUES (?)",
                           -1, &stmt, NULL) != SQLITE_OK)
        return -1;

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    if (finish(db, stmt) < 0)
        return -1;
    return (int)sqlite3_last_insert_rowid(db);
}

int create_item(sqlite3 *db, int category_id, const char *name, int price)
{
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db,
            "INSERT INTO Items (CategoryID, Name, Price) VALUES (?, ?, ?)",
            -1, &stmt, NULL) != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, category_id);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, price);
    if (finish(db, stmt) < 0)
        return -1;
    return (int)sqlite3_last_insert_rowid(db);
}

int get_item(sqlite3 *db, int category_id, int id,
             char *name, size_t name_size, int *price)
{
    sqlite3_stmt *stmt;
    int rc;

    if (sqlite3_prepare_v2(db,
            "SELECT Name, Price FROM Items WHERE ID = ? AND CategoryID = ?",
            -1, &stmt, NULL) != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_int(stmt, 2, category_id);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const unsigned char *text = sqlite3_column_text(stmt, 0);
        snprintf(name, name_size, "%s", text ? (const char *)text : "");
        *price = sqlite3_column_int(stmt, 1);
    }
    sqlite3_finalize(stmt);

    if (rc == SQLITE_ROW)
        return 1;
    return rc == SQLITE_DONE ? 0 : -1;
}

int update_item(sqlite3 *db, int id, const char *name, int price)
{
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, "UPDATE Items SET Name = ?, Price = ? WHERE ID = ?",
                           -1, &stmt, NULL) != SQLITE_OK)
        return -1;

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, price);
    sqlite3_bind_int(stmt, 3, id);
    return finish(db, stmt);
}

/* Runs a statement with parameters ?1 = a and (if present) ?2 = b.
   Returns number of changed rows or -1. */
static int run_with_ids(sqlite3 *db, const char *sql, int a, int b)
{
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, a);
    if (sqlite3_bind_parameter_count(stmt) >= 2)
        sqlite3_bind_int(stmt, 2, b);
    return finish(db, stmt);
}

/* Commits if result >= 0, otherwise rolls back. Returns result, or -1 if commit failed. */
static int end_transaction(sqlite3 *db, int result)
{
    if (result >= 0 && sqlite3_exec(db, "COMMIT", NULL, NULL, NULL) == SQLITE_OK)
        return result;

    sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
    return -1;
}

int delete_item(sqlite3 *db, int category_id, int id)
{
    int result;

    if (sqlite3_exec(db, "BEGIN", NULL, NULL, NULL) != SQLITE_OK)
        return -1;

    /* Basket rows reference the item, so they must go first */
    result = run_with_ids(db,
        "DELETE FROM Basket WHERE ItemID IN "
        "(SELECT ID FROM Items WHERE ID = ?1 AND CategoryID = ?2)",
        id, category_id);
    if (result >= 0)
        result = run_with_ids(db,
            "DELETE FROM Items WHERE ID = ?1 AND CategoryID = ?2",
            id, category_id);

    return end_transaction(db, result);
}

int delete_category(sqlite3 *db, int category_id)
{
    int result;

    if (sqlite3_exec(db, "BEGIN", NULL, NULL, NULL) != SQLITE_OK)
        return -1;

    result = run_with_ids(db,
        "DELETE FROM Basket WHERE ItemID IN "
        "(SELECT ID FROM Items WHERE CategoryID = ?1)",
        category_id, 0);
    if (result >= 0)
        result = run_with_ids(db, "DELETE FROM Items WHERE CategoryID = ?1", category_id, 0);
    if (result >= 0)
        result = run_with_ids(db, "DELETE FROM Categories WHERE ID = ?1", category_id, 0);

    return end_transaction(db, result);
}

int count_items(sqlite3 *db, int category_id)
{
    sqlite3_stmt *stmt;
    int count = -1;

    if (sqlite3_prepare_v2(db, "SELECT count(*) FROM Items WHERE CategoryID = ?",
                           -1, &stmt, NULL) != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, category_id);
    if (sqlite3_step(stmt) == SQLITE_ROW)
        count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    return count;
}

int add_to_basket(sqlite3 *db, int category_id, int id)
{
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db,
            "INSERT INTO Basket (ItemID) "
            "SELECT ID FROM Items WHERE ID = ? AND CategoryID = ?",
            -1, &stmt, NULL) != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_int(stmt, 2, category_id);
    return finish(db, stmt);
}

int remove_from_basket(sqlite3 *db, int id)
{
    sqlite3_stmt *stmt;

    /* Basket may hold several copies of the same item: delete only the latest one */
    if (sqlite3_prepare_v2(db,
            "DELETE FROM Basket WHERE ID = "
            "(SELECT ID FROM Basket WHERE ItemID = ? ORDER BY ID DESC LIMIT 1)",
            -1, &stmt, NULL) != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, id);
    return finish(db, stmt);
}

int clear_basket(sqlite3 *db)
{
    if (sqlite3_exec(db, "DELETE FROM Basket", NULL, NULL, NULL) != SQLITE_OK)
        return -1;
    return sqlite3_changes(db);
}

int basket_total(sqlite3 *db, long long *total)
{
    sqlite3_stmt *stmt;
    int rc;

    if (sqlite3_prepare_v2(db,
            "SELECT COALESCE(SUM(i.Price), 0) FROM Basket b "
            "JOIN Items i ON i.ID = b.ItemID",
            -1, &stmt, NULL) != SQLITE_OK)
        return -1;

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
        *total = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);

    return rc == SQLITE_ROW ? 0 : -1;
}
