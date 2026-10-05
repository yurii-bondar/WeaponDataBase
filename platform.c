#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"

void clear_screen(void)
{
#ifdef _WIN32
    system("cls");
#else
    fputs("\033[2J\033[H", stdout);
    fflush(stdout);
#endif
}

int read_line(char *buf, size_t size)
{
    size_t len;

    fflush(stdout);
    if (fgets(buf, (int)size, stdin) == NULL)
        return 0;

    len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[--len] = '\0';
    } else {
        /* Line too long: drop the rest so it does not leak into the next read */
        int c;
        while ((c = getchar()) != '\n' && c != EOF)
            ;
    }

    if (len > 0 && buf[len - 1] == '\r')   /* Windows line endings in piped input */
        buf[--len] = '\0';

    return 1;
}

int parse_int(const char *s, int *out)
{
    char *end;
    long  value;

    errno = 0;
    value = strtol(s, &end, 10);
    if (end == s || errno != 0 || value < INT_MIN || value > INT_MAX)
        return 0;

    while (*end == ' ' || *end == '\t')
        end++;
    if (*end != '\0')
        return 0;

    *out = (int)value;
    return 1;
}

int read_number(int *out)
{
    char line[64];

    return read_line(line, sizeof line) && parse_int(line, out);
}
