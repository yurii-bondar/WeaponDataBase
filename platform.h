#ifndef PLATFORM_H
#define PLATFORM_H

#include <stddef.h>

/* Clears the console window. */
void clear_screen(void);

/* Reads one line without the trailing newline; the rest of a too-long line is dropped.
   Returns 1 on success, 0 at end of input. */
int  read_line(char *buf, size_t size);

/* Parses the whole string (surrounding spaces allowed) as an integer.
   Returns 1 on success, 0 if it is empty or not a number. */
int  parse_int(const char *s, int *out);

/* read_line + parse_int. Returns 0 on empty/non-numeric input or end of input. */
int  read_number(int *out);

#endif /* PLATFORM_H */
