#include <stddef.h>

#include "db.h"
#include "menu.h"

int main(int argc, char *argv[])
{
    const char *path = argc > 1 ? argv[1] : "Weapon.db";
    sqlite3 *db = db_open(path);

    if (db == NULL)
        return 1;

    run_menu(db);

    sqlite3_close(db);
    return 0;
}
