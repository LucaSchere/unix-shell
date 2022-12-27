# custom unix ls implementation

## features

The program has the same behavior as the GNU/Linux `ls` command with the options `-1` and  `-F`.

Directories are listed with `/` at the end, executable files with
`*` and symlinks with `@`.

Example output of GNU/Linux `ls`:

```
$ ls -1 -F
ls*
ls.c
Makefile
README.md
symlink@
```

## how to use

1. Clone https://github.com/LucaSchere/unix-shell.git
2. Go to ls directory
3. Install GCC
4. Use `make` to compile the project:
```bash
    make
    ./ls
```

Have fun!