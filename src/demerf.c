/* cc demerf.c util.c
 *
 * cc demerf.c util.c
 *
 * May be distributed freely but please keep this comment at the top.
 *
 * Larry Ruane, LarryRuane@gmail.com
 */
#include <stdio.h>
#include <stdlib.h>

#include "util.h"

int
main(int ac, char **av)
{
    FILE *fp = stdin;
    char *text;

    if (ac < 1 || ac > 2 || (ac == 2 && av[1][0] == '-')) {
        fprintf(stderr, "usage: demerf [file]\n");
        return 1;
    }
    if (ac == 2) {
        if ((fp = fopen(av[1], "r")) == NULL) {
            fprintf(stderr, "cannot open %s\n", av[1]);
            return 1;
        }
    }
    /* input lines should start with a letter or a space,
     * followed by a space; include anything starting with
     * an uppercase letter (or a space)
     */
    for (; (text = fgetl(fp)); free(text)) {
        if (text[0] == '\0' || text[1] != ' ') {
            /* include normal lines, e.g. code that has been
             * added (don't require two leading special chars)
             */
            puts(text);
            continue;
        }
        if (text[0] >= 'a' && text[0] <= 'z') {
            /* don't include lowercase in the result */
            continue;
        }
        if ((text[0] >= 'A' && text[0] <= 'Z') || text[0] == ' ') {
            /* include uppercase in the result */
            puts(&text[2]);
            continue;
        }
        /* include anything we're not sure about */
        puts(text);
    }
    fclose(fp);
    return 0;
}
