#ifndef MENU_H
#define MENU_H

#include "sqlite3.h"

/* Main loop of the console UI. Returns when the user chooses Exit. */
void run_menu(sqlite3 *db);

#endif /* MENU_H */
