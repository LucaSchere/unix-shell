# custom sort command implementation

## features

This program takes several lines of input from standard input and prints them out in sorted order to standard
output.

By default, lines are sorted in ascending order. If the program is invoked with
an optional parameter `-r`, the lines will be sorted in descending order.

Lines can be entered interactively in the terminal and
`<Control-D>` can be used to properly terminate the input.



### sorting in *ascending* order:

```
$ ./sort
bonjour
hallo
gruesseuch
<Control-D>
bonjour
gruesseuch
hallo
```

### output with parameter `-r`

```
$ ./sort -r
bonjour
hallo
gruesseuch
<Control-D>
hallo
gruesseuch
bonjour
```

### sorting lines from a file

```
$ cat test.txt
zzzz
gggg
abcd
def
$ ./sort < test.txt
abcd
def
gggg
zzzz
$ ./sort -r < test.txt
zzzz
gggg
def
abcd
```

## how to use

1. Clone https://github.com/LucaSchere/unix-shell.git
2. Go to sort directory
3. Install GCC
4. Use `make` to compile the project:
```bash
    make
    ./sort
```

Have fun!