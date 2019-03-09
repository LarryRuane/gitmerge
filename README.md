# Git merge tool #

This tool is the easiest way I have found to resolve git merge conflicts.
It uses a tool I wrote called `merf`, which is written in generic C
and included in this repository. It works well with any text editor --
no split-screens or anything fancy is needed, and no GUIs.

## Installation ##
Clone this repository, then:
```
cd gitmerge/src
make test
cp merf demerf ~/bin
cd ..
cp bin/gitmerf ~/bin
cat >> ~/.gitconfig <<'EOF'
[mergetool "m"]
    cmd = gitmerf $REMOTE $LOCAL $BASE $MERGED
    trustExitCode = true
EOF
```
Edit `~/bin/gitmerf` and replace `gvim -f` with your favorite text editor.
It should be invoked so that it doesn't return until the editor exits.
(Since `gvim`, by default, opens a new window and runs in the background,
and you get your shell prompt immediately, the `-f` flag is
needed to cause it to run in the foreground.)

## Use ##
When your clone gets into merge-conflict mode (after doing a `git merge`,
`git rebase` or similar), then run:
```
git mergetool -t m
```
This will bring up your editor on a temporary file that is a 3-way merge
of the base file (the common ancestor), the local file (with your changes),
and the remote file (with the changes that others have made).
You edit this file to resolve conflicts, write it, and quit the editor.

It may be easiest to explain with an example. The repo includes three
files that are used when running the tiny test (`make test`). File
`src/test/base` represents the base file:
```
common line 1
common line 2
(...)
common line 20
```
Pretend you've made changes so that the file looks like this (note that
you've deleted line 17):
```
common line 1
common line 2
common line 3
new line added between 3 and 4 by both
common line 4
common line 5
common line 6
common line 7 -- changed by local
common line 8
common line 9
common line 10
common line 11
common line 12
common line 13 -- changed by local (and remote)!
common line 14
common line 15
common line 16
common line 18
common line 19
common line 20
new line 21 by local
```
The remote side changes the file to the following (also deleted line 17):
```
common line 1
common line 2 -- changed by remote
common line 3
new line added between 3 and 4 by both
common line 4
common line 5
common line 6
common line 7
common line 8
common line 9
common line 10
common line 11
common line 12
common line 13 -- changed by remote (and local)!
common line 14
common line 15
common line 16
common line 18
common line 19
common line 20
new line 21 by remote
```
When you run the mergetool, you see the following in the editor:
```
  common line 1
R common line 2 -- changed by remote
r common line 2
  common line 3
C new line added between 3 and 4 by both
  common line 4
  common line 5
  common line 6
l common line 7
L common line 7 -- changed by local
  common line 8
  common line 9
  common line 10
  common line 11
  common line 12
R common line 13 -- changed by remote (and local)!
L common line 13 -- changed by local (and remote)!
c common line 13
  common line 14
  common line 15
  common line 16
c common line 17
  common line 18
  common line 19
  common line 20
R new line 21 by remote
L new line 21 by local
```
This file is in what I call _merf_ format.
Two additional tag characters are prepended to each line, the second
always a space (just to improve readability). All changes are shown as lines
being added or removed (there is no "line was change" indication).
Uppercase tags mean the line is being added; lowercase tags mean the
line is being removed. The `R` and `r` tags indicate lines that were
added or removed by the remote side (other people). The `L` and `l` tags
show lines that you added or removed ("local"). The `C` and `c` tags
show "common" changes -- the line were added or removed by both; examples
are the new line between lines 3 and 4 (added) and line 17 (removed).

To summarize:
- `L` -- added by local (you)
- `l` -- removed by local (you)
- `R` -- added by remote
- `r` -- removed by remote
- `C` -- added by both (or conflict)
- `c` -- removed by both (or conflict)

Within the editor, simply edit this file as you wish, keeping two
things in mind that the following will happen when you exit the editor:
1. Lines that begin with uppercase letters will be preserved
1. Lines that begin with lowercase letters will be discarded
1. Lines that begin with a space are preserved
1. The first two characters are stripped from each line
This result is then written back to the source file.

If there are no conflicts, you don't need to take any action in the
editor (just write-quit), but it may be useful to look through all
of the changes.
Note that lines beginning with `C` or `c` are often conflicts; you can even
think of these as standing for "conflict". For example, common line
13 in this example was changed by both; it's likely that
the result should have only one version of this line. In this case, you
must manually merge these lines together. Just do that right in the
editor.

It's sometimes useful to remove lines that start with lowercase letters
in the editor, just to clean up what you're seeing, even though those
lines will be removed anyway.

Notice the last two lines were added by both, so there are no `C` or `c`
tags. This is often a conflict; for example, suppose in C you're
defining symbols to consecutive integers (there are better ways to
do this in C):
```
(...)
#define ABCx      34
#define ABCy      35
#define NEWLOCAL  36
```
Meanwhile, someone else adds to this list:
```
(...)
#define ABCx      34
#define ABCy      35
#define NEWREMOTE 36
```
The merf file will be:
```
  (...)
  #define ABCx      34
  #define ABCy      35
R #define NEWREMOTE 36
L #define NEWLOCAL  36
```
This is a conflict that must be resolved (probably by changing your
line's value to 37).

To make these types of conflicts stand out, the merf file format
is actually slightly different from what was shown earlier:
```
  common line 1
R common line 2 -- changed by remote
r common line 2
  common line 3
C new line added between 3 and 4 by both
  common line 4
  common line 5
  common line 6
l common line 7
L common line 7 -- changed by local
  common line 8
  common line 9
  common line 10
  common line 11
  common line 12
x 
R common line 13 -- changed by remote (and local)!
x 
L common line 13 -- changed by local (and remote)!
c common line 13
  common line 14
  common line 15
  common line 16
c common line 17
  common line 18
  common line 19
  common line 20
R new line 21 by remote
x 
L new line 21 by local
```
An `x` line (always lowercase, nothing else on the rest of the line)
is _added_ wherever local and remote changes are close (within 6 lines)
of each other. This is a likely conflict. You don't have to remove these
lines, since they start with a lowercase letter (of course it
doesn't hurt to remove them to clean up your view).

A common search pattern for me is `^[Ccx]` which matches any line
that begins with any of those three letters. These are often conflicts
or close to conflicts.

When you exit the editor, git will bring up the next file with conflicts.
If you want to abondon the merge process, delete all the lines in
the editor, write and quit, and you will be prompted for whether you'd
like to continue or stop. (If you continue, the same merf file will
again appear.)

I hope this tool is useful! Don't hesitate to open PRs if you run into
any problems or have suggestions.
