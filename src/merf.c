/*
 * cc merf.c util.c
 *
 * May be distributed freely but please keep this comment at the top.
 *
 * Larry Ruane, LarryRuane@gmail.com
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

/* line > 0 means insert line-1 ; line < 0 means delete -line-1 */
struct edit {
    int line;
    struct edit *link;
};

struct file {
    int len;
    int alloc;
    char *flags;
    char **line;
} R, L, RL;

#define HSTRING 1024
struct string {
    char *p;
    struct string *next;
} *hstring[HSTRING];

/* keep track of last few initial characters, to catch possible conflicts */
char lastc[6];

/* space efficiency; malloc'ing many small structures is wasteful */
struct edit *
ealloc(void)
{
    static struct edit *fp;
    struct edit *newedit, *t, *r;

    if (!fp) {
        newedit = (struct edit *)ckmalloc(1024*sizeof(struct edit));
        for (t = newedit; t<&newedit[1024]; ++t) {
            t->link = fp;
            fp = t;
        }
    }
    fp = (r = fp)->link;
    return r;
}

/* append a line and flags to a file, growing it if necessary */
void
appendf(struct file *f, int flags, char *line) {
    if (f->len == f->alloc) {
        f->alloc += 1024;
        f->line = (char **)ckrealloc(f->line, sizeof(char *)*f->alloc);
        f->flags = (char *)ckrealloc(f->flags, sizeof(char)*f->alloc);
    }
    f->line[f->len] = line;
    f->flags[f->len++] = flags;
}

/* initialize a file structure */
void
initf(struct file *f) {
    if (f->len) {
        f->len = 0;
        free(f->line);
        free(f->flags);
    }
    f->alloc = 1024;
    f->line = (char **)ckmalloc(sizeof(char *)*f->alloc);
    f->flags = (char *)ckmalloc(sizeof(char)*f->alloc);
}

void
rfile(char *filename, struct file *f) {
    FILE *fp;
    char *text, *p;
    struct string **sp;
    unsigned cs;

    initf(f);
    if (!(fp = fopen(filename, "r")))
        return;
    while ((text = fgetl(fp))) {
        for (cs = 0, p = &text[strlen(text)-1]; p >= text; --p)
            cs += *p;
        for (sp = &hstring[cs & (HSTRING-1)];; sp= &(*sp)->next) {
            if (!(*sp)) {
                *sp = (struct string *)ckmalloc(sizeof **sp);
                (*sp)->p = text;
                (*sp)->next = NULL;
                break;
            }
            if (!strcmp((*sp)->p, text)) {
                free(text);
                break;
            }
        }
        appendf(f, 0, (*sp)->p);
    }
    fclose(fp);
}

struct edit *
diff(struct file *r, struct file *l) {
    int *last_d;
    struct edit **script;
    int origin;
    int d;
    int lower, upper;
    int row;

    origin = r->len;
    for (row = 0;
        row < r->len && row < l->len && r->line[row] == l->line[row];
        row++);
    lower = origin + ((row == r->len) ? 1 : -1);
    upper = origin + ((row == l->len) ? -1 : 1);
    if (lower>upper)
        return NULL;
    last_d = (int *)ckmalloc(sizeof(int)*(r->len+l->len+1));
    last_d[origin] = row;
    script = (struct edit **)ckmalloc(sizeof(struct edit *)*(r->len+l->len+1));
    script[origin] = NULL;
    for (d = 1;; ++d) {
        for (int k = lower; k <= upper; k += 2) {
            struct edit *newedit = ealloc();
            int col;
            if (k == origin-d ||
                (k != origin+d && last_d[k+1] >= last_d[k-1]))
            {
                /* delete */
                row = last_d[k+1]+1;
                col = row+k-origin;
                newedit->link = script[k+1];
                newedit->line = -col-1;
            } else {
                /* insert */
                row = last_d[k-1];
                col = row+k-origin;
                newedit->link = script[k-1];
                newedit->line = row+1;
            }
            while (row<r->len && col<l->len && r->line[row] == l->line[col]) {
                ++row;
                ++col;
            }
            if (row == r->len && col == l->len) {
                free(last_d);
                free(script);
                return newedit;
            }
            script[k] = newedit;
            last_d[k] = row;
            if (row == r->len)
                lower = k+2;
            if (col == l->len)
                upper = k-2;
        }
        --lower;
        ++upper;
    }
}

/* apply start, the r-to-l edit script, to r, result in rl */
void
aedit(
    struct file *r,
    struct file *l,
    struct file *rl,
    char bit,
    struct edit *start
) {
    struct edit *t, *ep;
    int i, j;

    initf(rl);
    ep = NULL;
    while ((t = start)) {
        start = t->link;
        t->link = ep;
        ep = t;
    }
    for (i = 0, j = 0;;) {
        if (ep && ep->line>0 && ep->line-1 == i) {
            /* insert */
            appendf(rl, bit, l->line[j++]);
            ep = ep->link;
        } else {
            if (i >= r->len)
                break;
            if (ep && ep->line<0 && -ep->line-1 == j) {
                /* delete */
                appendf(rl, r->flags[i], r->line[i]);
                ep = ep->link;
            } else {
                /* neither inserted nor deleted */
                appendf(rl, r->flags[i]|bit, r->line[i]);
                ++j;
            }
            ++i;
        }
    }
}

int
main(int ac, char **av) {
    struct file *r= &R, *l = &L, *rl = &RL, *t;
    char c;

    if (ac<3 || ac>4 || av[1][0] == '-') {
        fprintf(stderr, "usage: %s R [L] base\n", av[0]);
        return 1;
    }
    int bit = 1<<(ac-2); /* if 3 files, bit = 4 */
    rfile(*++av, r);
    for (int i = 0; i<r->len; ++i)
        r->flags[i] = bit;
    for (bit >>= 1; bit; bit >>= 1) {
        rfile(*++av, l);
        aedit(r, l, rl, bit, diff(r, l));
        t = r;
        r = rl;
        rl = t;
    }
    for (int i = 0; i<r->len; ++i) {
        if (r->flags[i] == (1<<(ac-1))-1)
            /* all bits on, common line to all files */
            c = ' ';
        else
            c = r->flags[i]+'0';
        if (ac == 3) {
            switch(c) {
            case '2': c = 'R'; break;
            case '1': c = 'r'; break;
            }
        } else {
            switch(c) {
            case '4': c = 'R'; break;
            case '3': c = 'r'; break;
            case '2': c = 'L'; break;
            case '5': c = 'l'; break;
            case '6': c = 'C'; break;
            case '1': c = 'c'; break;
            }
        }
        /* adding a line with just 'x ' helps highlight possible
         * conflicts (these lines are removed by demerf
         */
        for (int j = 0; j<sizeof(lastc); j++) {
            if (strchr("Ccx", lastc[j]))
                break;
            if ((strchr("Rr", lastc[j]) && strchr("Ll", c)) ||
                (strchr("Ll", lastc[j]) && strchr("Rr", c)))
            {
                puts("x ");
                for (j = sizeof(lastc)-1; j > 0; j--)
                    lastc[j] = lastc[j-1];
                lastc[0] = 'x';
                break;
            }
        }
        for (int j = sizeof(lastc)-1; j>0; j--)
            lastc[j] = lastc[j-1];
        printf("%c %s\n", lastc[0] = c, r->line[i]);
    }
    return 0;
}
