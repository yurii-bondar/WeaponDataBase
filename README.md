# WeaponDataBase

Console weapon shop written in **C** with the **SQLite3** library.
Course project of Vinnytsia IT-Academy (CRUD).

Runs on **macOS, Linux and Windows**. SQLite is bundled (`sqlite3.c` / `sqlite3.h`),
so no external libraries are needed — only a C compiler.

## Features

- Browse weapon categories and their items with prices
- Create new categories
- Add new items to a category, edit name and price of existing ones
- Basket: add items, remove items, clear the basket, see the total amount

## Build

### macOS / Linux

Requires `cc` (clang or gcc) and `make`. On macOS: `xcode-select --install`.

```sh
make            # builds ./weapondb
make run        # builds and starts it with Weapon.db
make clean      # removes build files
```

### Windows

**MinGW-w64 / MSYS2:**

```sh
make CC=gcc     # builds weapondb.exe
```

**CMake** (Visual Studio, MinGW, or any other platform):

```sh
cmake -S . -B build
cmake --build build --config Release
```

The binary appears in `build/` (or `build/Release/` for Visual Studio).

**Qt Creator:** open `PROJECT.pro` and press *Build*.

## Run

```sh
./weapondb                  # uses Weapon.db in the current directory
./weapondb path/to/my.db    # uses another database file
```

On Windows: `weapondb.exe` or `weapondb.exe path\to\my.db`.

The database file must already exist. When the program is started from another
directory (e.g. from `build/`), pass the path to `Weapon.db` explicitly.
An empty file also works — the program will create the tables in it.

## Usage

Every command is typed and confirmed with **Enter**. Empty input cancels the current action.

**Main menu**

| Input   | Action                                 |
|---------|----------------------------------------|
| `1`…`N` | Open a category                        |
| `B`     | Open the basket                        |
| `N`     | Create a new category                  |
| `0`     | Exit                                   |

**Category screen**

| Input | Action                                                  |
|-------|---------------------------------------------------------|
| `1`   | Add an item to the basket (by item ID)                  |
| `2`   | Create a new item (name + price)                        |
| `3`   | Edit an item (Enter keeps the current name / price)     |
| `0`   | Back                                                    |

**Basket**

| Input | Action                                          |
|-------|-------------------------------------------------|
| `1`   | Remove one copy of an item (by item ID)         |
| `2`   | Clear the basket                                |
| `0`   | Back                                            |

## Database

```
Categories (ID, Name)                      -- Name is unique, case-insensitive
Items      (ID, CategoryID -> Categories, Name, Price >= 0)
Basket     (ID, ItemID -> Items)           -- one row per item in the basket
```

The basket stores links to items, so editing an item's price changes the basket total too.

The schema version is kept in `PRAGMA user_version`. A database in the old format
(one table per category: `Throwing_weapon`, `Sights`, …) is converted automatically,
in a single transaction, the first time the program opens it. Item IDs and the basket
contents are preserved.

## Project structure

| File                      | Purpose                                                    |
|---------------------------|------------------------------------------------------------|
| `main.c`                  | Entry point: opens the database, runs the menu, closes it  |
| `db.c`, `db.h`            | Opening the database, schema creation and migration        |
| `menu.c`, `menu.h`        | Console UI: screens and user input handling                |
| `tables.c`, `tables.h`    | Reading and printing categories, items and the basket      |
| `crud.c`, `crud.h`        | Create / update operations for categories, items, basket   |
| `platform.c`, `platform.h`| Cross-platform screen clearing and line input              |
| `sqlite3.c`, `sqlite3.h`  | Bundled SQLite amalgamation                                |
| `Weapon.db`               | Sample database                                            |
| `Makefile`, `CMakeLists.txt`, `PROJECT.pro` | Build files (make / CMake / Qt Creator)  |
