# basic unix shell

Written by Luca Scherer in the first year of his studies.
(Repository structure was chosen for teaching purposes)

## features

### print a shell prompt (`$`) when ready

```
...
$
```

### read multiple commands from standard input and execute them

```
$ echo Hello
Hello
$ expr 1 + 3
4
```

### support shell variables and expansion

```
$ A=5
$ echo $A
5
$ echo ${A}BC
5BC
$ A=$A$A
echo $A
55
```

### support running programs in the background with `&`

```
$ cat &
$ pkill cat
```

### support redirection of stdin and stdout with `>` and `<`

```
$ echo Hello > test.txt
$ cat - < test.txt
Hello
```

### support the pipeline operator `|`

```
$ echo Hello | tr l L
HeLLo
```

## how to use

1. Clone https://github.com/LucaSchere/unix-shell.git
2. Install GCC
3. Use `make` to compile the project:
```bash
    make
    ./shell
```

Have fun!
