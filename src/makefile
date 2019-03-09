# May be distributed freely but please keep this comment at the top.
# Larry Ruane, LarryRuane@gmail.com

all: merf demerf

# "merge file" -- generate merged tmp file from two changed files and
# their common base, prepends 2-character tags to each line
merf: merf.o util.o
	cc -o merf merf.o util.o

merf.o: merf.c
	cc -c merf.c

# discards lines starting with lowercase letter; strips off first two
# of the remaining characters and writes to stdout
demerf: demerf.o util.o
	cc -o demerf demerf.o util.o

demerf.o: demerf.c
	cc -c demerf.c

util.o: util.c util.h
	cc -c util.c

clean:
	rm -f *.o

clobber: clean
	rm -f merf demerf

test: merf demerf runtest

runtest:
	./merf test/remote test/local test/base | cmp -s test/merf.out
	./demerf test/merf.out | cmp -s test/demerf.out
