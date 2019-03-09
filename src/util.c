/*
 * return a pointer to the next input line (without the \n).
 * space must be freed by caller.
 *
 * May be distributed freely but please keep this comment at the top.
 *
 * Larry Ruane, LarryRuane@gmail.com
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *
ckmalloc(int length) {
    void *p;
    if ((p = malloc(length)) == NULL) {
        fprintf(stderr, "no memory\n");
        exit(1);
    }
    return p;
}

void *
ckrealloc(void *p, int length) {
    if ((p = realloc(p, length)) == NULL) {
        fprintf(stderr, "no memory\n");
        exit(1);
    }
    return p;
}

/* return a pointer to the next arbitrary-length line */
char *
fgetl(FILE *fp) {
    char *b = ckmalloc(40);
    int nchars = 0;

    for (;;) {
        if (fgets(b+nchars, 40, fp) == NULL) {
            free(b);
            return NULL;
        }
        nchars = strlen(b);
        if (b[nchars-1] == '\n')
            break;
        b = ckrealloc(b, nchars+40);
    }
    b[--nchars]='\0';           /* remove the newline */
    b = ckrealloc(b, nchars+1); /* trim to min size (+1 for the null) */
    return b;
}
