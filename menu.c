#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crud.h"
#include "menu.h"
#include "platform.h"
#include "tables.h"

#define STATUS_SIZE 256

static void title(void)
{
    printf("                                                         ----------------------------------\n");
    printf("                                                            DataBase  (+) Shot & Hit (+)  \n");
    printf("                                                         ----------------------------------\n");
}

static void show_status(const char *status)
{
    if (status[0] != '\0')
        printf("\n    %s\n", status);
}

static void set_sql_error(sqlite3 *db, char *status)
{
    snprintf(status, STATUS_SIZE, "--> SQL error: %s", sqlite3_errmsg(db));
}

/* Reads a menu choice: first non-space character in lower case,
   '\0' for an empty line, EOF at end of input. */
static int read_choice(void)
{
    char line[32];
    const char *p = line;

    printf("\n    Choice --> ");
    if (!read_line(line, sizeof line))
        return EOF;
    while (*p == ' ' || *p == '\t')
        p++;
    return tolower((unsigned char)*p);
}

/* Asks "(y/N)". Returns 1 only for an answer starting with 'y'. */
static int confirm(void)
{
    char line[16];

    printf(" (y/N) --> ");
    if (!read_line(line, sizeof line))
        return 0;
    return tolower((unsigned char)line[strspn(line, " \t")]) == 'y';
}

/* Reads a line and trims surrounding spaces.
   Returns its length, or -1 at end of input or if it is longer than max_len. */
static int read_text(char *buf, size_t size, size_t max_len)
{
    char line[256];
    char *start = line, *end;
    size_t len;

    if (!read_line(line, sizeof line))
        return -1;

    while (*start == ' ' || *start == '\t')
        start++;
    end = start + strlen(start);
    while (end > start && (end[-1] == ' ' || end[-1] == '\t'))
        *--end = '\0';

    len = (size_t)(end - start);
    if (len > max_len || len >= size)
        return -1;

    memcpy(buf, start, len + 1);
    return (int)len;
}

/* Reads a price. Returns 1 on success, 0 if the line is empty, -1 if invalid. */
static int read_price(int *price)
{
    char line[64];

    if (!read_line(line, sizeof line) || line[strspn(line, " \t")] == '\0')
        return 0;
    if (!parse_int(line, price) || *price < 0)
        return -1;
    return 1;
}

static void new_category(sqlite3 *db, char *status)
{
    char name[MAX_CATEGORY_NAME + 1];
    int len, id;

    printf("\n    <ENTER TO CANCEL>\n");
    printf("\n    New category name (max %d chars) --> ", MAX_CATEGORY_NAME);

    len = read_text(name, sizeof name, MAX_CATEGORY_NAME);
    if (len < 0) {
        snprintf(status, STATUS_SIZE, "--> Name is too long");
        return;
    }
    if (len == 0) {
        status[0] = '\0';
        return;
    }

    id = create_category(db, name);
    if (id >= 0)
        snprintf(status, STATUS_SIZE, "--> Category '%s' created", name);
    else if (sqlite3_extended_errcode(db) == SQLITE_CONSTRAINT_UNIQUE)
        snprintf(status, STATUS_SIZE, "--> Category '%s' already exists", name);
    else
        set_sql_error(db, status);
}

static void remove_category(sqlite3 *db, const Category *categories, int count, char *status)
{
    const Category *category;
    int number, items;

    printf("\n    <ENTER TO CANCEL>\n");
    printf("\n    Number of category to DELETE --> ");
    if (!read_number(&number)) {
        status[0] = '\0';
        return;
    }
    if (number < 1 || number > count) {
        snprintf(status, STATUS_SIZE, "--> No category with ID %d", number);
        return;
    }

    category = &categories[number - 1];
    items = count_items(db, category->id);
    if (items < 0) {
        set_sql_error(db, status);
        return;
    }

    printf("\n    Delete '%s' with %d item(s) (also from the basket)?", category->name, items);
    if (!confirm()) {
        snprintf(status, STATUS_SIZE, "--> Deletion cancelled");
        return;
    }

    if (delete_category(db, category->id) > 0)
        snprintf(status, STATUS_SIZE, "--> Category '%s' deleted", category->name);
    else
        snprintf(status, STATUS_SIZE, "--> Failed to delete category '%s'", category->name);
}

static void add_item_to_basket(sqlite3 *db, const Category *category, char *status)
{
    int id;

    printf("\n    <ENTER TO CANCEL>\n");
    printf("\n    ID weapon for ADD --> ");
    if (!read_number(&id)) {
        status[0] = '\0';
        return;
    }

    switch (add_to_basket(db, category->id, id)) {
    case 1:  snprintf(status, STATUS_SIZE, "--> ID %d added to basket", id); break;
    case 0:  snprintf(status, STATUS_SIZE, "--> ID %d not found in this category", id); break;
    default: set_sql_error(db, status); break;
    }
}

static void new_item(sqlite3 *db, const Category *category, char *status)
{
    char name[MAX_ITEM_NAME + 1];
    int len, price, id;

    printf("\n    <ENTER TO CANCEL>\n");
    printf("\n    Name (max %d chars) --> ", MAX_ITEM_NAME);
    len = read_text(name, sizeof name, MAX_ITEM_NAME);
    if (len < 0) {
        snprintf(status, STATUS_SIZE, "--> Name is too long");
        return;
    }
    if (len == 0) {
        status[0] = '\0';
        return;
    }

    printf("    Price $ --> ");
    if (read_price(&price) != 1) {
        snprintf(status, STATUS_SIZE, "--> Price must be a number >= 0");
        return;
    }

    id = create_item(db, category->id, name, price);
    if (id >= 0)
        snprintf(status, STATUS_SIZE, "--> '%s' added with ID %d", name, id);
    else
        set_sql_error(db, status);
}

static void edit_item(sqlite3 *db, const Category *category, char *status)
{
    char name[MAX_ITEM_NAME + 1];
    char new_name[MAX_ITEM_NAME + 1];
    int id, price, new_price, len, found;

    printf("\n    <ENTER TO CANCEL>\n");
    printf("\n    ID weapon for EDIT --> ");
    if (!read_number(&id)) {
        status[0] = '\0';
        return;
    }

    found = get_item(db, category->id, id, name, sizeof name, &price);
    if (found <= 0) {
        if (found == 0)
            snprintf(status, STATUS_SIZE, "--> ID %d not found in this category", id);
        else
            set_sql_error(db, status);
        return;
    }

    printf("\n    Current: %s  $ %d\n", name, price);
    printf("    <ENTER TO KEEP CURRENT VALUE>\n\n");

    printf("    New name (max %d chars) --> ", MAX_ITEM_NAME);
    len = read_text(new_name, sizeof new_name, MAX_ITEM_NAME);
    if (len < 0) {
        snprintf(status, STATUS_SIZE, "--> Name is too long");
        return;
    }
    if (len == 0)
        strcpy(new_name, name);

    printf("    New price $ --> ");
    switch (read_price(&new_price)) {
    case 0:
        new_price = price;
        break;
    case -1:
        snprintf(status, STATUS_SIZE, "--> Price must be a number >= 0");
        return;
    }

    if (update_item(db, id, new_name, new_price) > 0)
        snprintf(status, STATUS_SIZE, "--> ID %d updated: %s  $ %d", id, new_name, new_price);
    else
        set_sql_error(db, status);
}

static void remove_item(sqlite3 *db, const Category *category, char *status)
{
    char name[MAX_ITEM_NAME + 1];
    int id, price, found;

    printf("\n    <ENTER TO CANCEL>\n");
    printf("\n    ID weapon for DELETE --> ");
    if (!read_number(&id)) {
        status[0] = '\0';
        return;
    }

    found = get_item(db, category->id, id, name, sizeof name, &price);
    if (found <= 0) {
        if (found == 0)
            snprintf(status, STATUS_SIZE, "--> ID %d not found in this category", id);
        else
            set_sql_error(db, status);
        return;
    }

    printf("\n    Delete '%s' (also from the basket)?", name);
    if (!confirm()) {
        snprintf(status, STATUS_SIZE, "--> Deletion cancelled");
        return;
    }

    if (delete_item(db, category->id, id) > 0)
        snprintf(status, STATUS_SIZE, "--> '%s' deleted", name);
    else
        snprintf(status, STATUS_SIZE, "--> Failed to delete ID %d", id);
}

static void category_screen(sqlite3 *db, const Category *category)
{
    char status[STATUS_SIZE] = "";

    for (;;) {
        clear_screen();
        title();
        print_items(db, category);
        show_status(status);

        printf("\n -------------------------------------\n");
        printf("   1 --> ADD to basket\n");
        printf("   2 --> NEW item\n");
        printf("   3 --> EDIT item\n");
        printf("   4 --> DELETE item\n");
        printf("   0 --> BACK\n");
        printf(" -------------------------------------\n");

        switch (read_choice()) {
        case '1': add_item_to_basket(db, category, status); break;
        case '2': new_item(db, category, status);           break;
        case '3': edit_item(db, category, status);          break;
        case '4': remove_item(db, category, status);        break;
        case '0':
        case EOF: return;
        default:  snprintf(status, STATUS_SIZE, "--> Unknown command"); break;
        }
    }
}

static void delete_from_basket(sqlite3 *db, char *status)
{
    int id;

    printf("\n    <ENTER TO CANCEL>\n");
    printf("\n    ID weapon for DELETE --> ");
    if (!read_number(&id)) {
        status[0] = '\0';
        return;
    }

    switch (remove_from_basket(db, id)) {
    case 1:  snprintf(status, STATUS_SIZE, "--> ID %d removed from basket", id); break;
    case 0:  snprintf(status, STATUS_SIZE, "--> ID %d is not in the basket", id); break;
    default: set_sql_error(db, status); break;
    }
}

static void basket_screen(sqlite3 *db)
{
    char status[STATUS_SIZE] = "";
    long long total;

    for (;;) {
        clear_screen();
        title();
        print_basket(db);

        printf("--------------------------------------------------------------\n");
        if (basket_total(db, &total) == 0)
            printf("      T O T A L   A M O U N T   > > > > > > > > > >   $ %lld\n", total);
        show_status(status);

        printf("\n -------------------------------------\n");
        printf("   1 --> DELETE item\n");
        printf("   2 --> CLEAR BASKET\n");
        printf("   0 --> BACK\n");
        printf(" -------------------------------------\n");

        switch (read_choice()) {
        case '1':
            delete_from_basket(db, status);
            break;
        case '2':
            if (clear_basket(db) >= 0)
                snprintf(status, STATUS_SIZE, "--> Basket is cleared");
            else
                set_sql_error(db, status);
            break;
        case '0':
        case EOF:
            return;
        default:
            snprintf(status, STATUS_SIZE, "--> Unknown command");
            break;
        }
    }
}

void run_menu(sqlite3 *db)
{
    char status[STATUS_SIZE] = "";
    char line[32];
    Category *categories;
    int count, i, number, done = 0;

    while (!done) {
        count = load_categories(db, &categories);
        if (count < 0) {
            fprintf(stderr, "Can't load categories: %s\n", sqlite3_errmsg(db));
            return;
        }

        clear_screen();
        title();
        printf(" --------------------------------------\n");
        printf("  <ID>        < MAIN MENU >\n");
        printf(" --------------------------------------\n\n");
        for (i = 0; i < count; i++)
            printf("  %2d --> %s\n", i + 1, categories[i].name);
        if (count == 0)
            printf("        (no categories yet)\n");
        printf("\n --------------------------------------\n");
        printf("   B --> GO  to BASKET\n");
        printf("   N --> NEW category\n");
        printf("   D --> DELETE category\n");
        printf("   0 --> EXIT\n");
        printf(" --------------------------------------\n");
        show_status(status);

        printf("\n    Choice --> ");
        if (!read_line(line, sizeof line)) {
            done = 1;
        } else if (parse_int(line, &number)) {
            status[0] = '\0';
            if (number == 0)
                done = 1;
            else if (number >= 1 && number <= count)
                category_screen(db, &categories[number - 1]);
            else
                snprintf(status, STATUS_SIZE, "--> No category with ID %d", number);
        } else {
            switch (tolower((unsigned char)line[strspn(line, " \t")])) {
            case 'b': status[0] = '\0'; basket_screen(db);       break;
            case 'n': new_category(db, status);                  break;
            case 'd': remove_category(db, categories, count, status); break;
            case '\0': status[0] = '\0';                         break;
            default:  snprintf(status, STATUS_SIZE, "--> Unknown command"); break;
            }
        }

        free(categories);
    }
}
