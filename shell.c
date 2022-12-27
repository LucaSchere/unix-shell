#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>

#define MAX_CHARS_PER_LINE 200
#define MAX_COMMAND_NAME_LENGTH 20
#define MAX_COMMAND_ARGS 20
#define MAX_COMMANDS 5
#define PRODUCT_PLACEHOLDER "\0"
#define MAX_VARIABLES 20
#define MAX_PIPE_VALUE 100
#define MAX_BACKGROUND_TASKS 10

enum IO_TYPE {
    PATH,
    CONSOLE,
    PIPE
};

enum IO_TYPE current_output_type = CONSOLE;
enum IO_TYPE current_input_type = CONSOLE;

bool in_background = false;

char current_output_path[FILENAME_MAX] = "";
char current_input_path[FILENAME_MAX] = "";

char last_pipe_buffer[MAX_PIPE_VALUE] = "";
char pipe_buffer[MAX_PIPE_VALUE] = "";

char *variables[MAX_VARIABLES * 2];
int variables_count = 0;

char *background_tasks[MAX_BACKGROUND_TASKS * 2];
int background_tasks_count = 0;

bool new_path_output = true;

/*  ******************************************************************
 *  utility functions
 *  ******************************************************************
 */

/**
 * splits str into given tokens by delim
 * @param str
 * @param delim
 * @param tokens
 * @return int
 */
int str_split(char *str, char *delim, char **tokens) {
    int i = 0;
    if((tokens[i] = strtok(str, delim)) == NULL) return -1;
    i++;
    while((tokens[i] = strtok(NULL, delim)) != NULL) i++;
    return i;
}

/**
 * fetches input based on the current input type
 * copies the input into string buffer
 * @param string
 */
void input(char *string) {
    switch (current_input_type) {
        case PATH: {
            FILE *file = fopen(current_input_path, "r");
            if (file == NULL) {
                printf("Error opening file %s", current_input_path);
                exit(1);
            }
            fgets(string, MAX_CHARS_PER_LINE, file);
            break;
        }
        case PIPE:
            strcpy(string, last_pipe_buffer);
            break;
        case CONSOLE:
        default:
            scanf("%s", string);
            break;
    }
}

/**
 * outputs given string based on current output type
 * @param message
 */
void output(char *message) {
    switch (current_output_type) {
        case PATH: {
            FILE *file = fopen(current_output_path, new_path_output ? "w" : "a");
            if (file == NULL) {
                printf("Error opening file %s", current_output_path);
                exit(1);
            }
            fprintf(file, "%s", message);
            fclose(file);
            new_path_output = false;
            break;
        }
        case CONSOLE:
            printf("%s", message);
            break;
        case PIPE:
            strcat(pipe_buffer, message);
            break;
    }
}

/**
 * determine if a string is a number
 * @param text
 * @return
 */
bool is_number(char *text)
{
    int i;
    i = strlen(text);
    while(i--)
    {
        if(text[i] > 47 && text[i] < 58)
            continue;
        return false;
    }
    return true;
}

/**
 * determine if given string is a valid expression
 * @param args
 * @param num_args
 * @return
 */
bool expression_is_valid(char *args[], int num_args) {
    for (int i = 1; i < num_args; i = i + 2) {
        if (is_number(args[i]) == false) {
            return false;
        }
    }
    for (int i = 2; i < num_args; i = i + 2) {
        if (strcmp(args[i], "+") != 0 &&
        strcmp(args[i], "-") != 0 &&
        strcmp(args[i], "*") != 0 &&
        strcmp(args[i], "%") != 0 &&
        strcmp(args[i], "/") != 0)
        {
            return false;
        }
    }
    return true;
}

/**
 * evaluate prioritized operations and store the results in products array
 * @param args
 * @param num_args
 * @param products
 */
void eval_prioritized_only(char *args[], int num_args, int products[]){
    const char prioritized[] = {'*','/', '%'};
    int i, a, b;
    bool found;
    for(i = 2; i < num_args; i=i+2){
        found = false;
        a = args[i-1][0] == PRODUCT_PLACEHOLDER[0] ? products[i-1] : atoi(args[i-1]);
        b = args[i+1][0] == PRODUCT_PLACEHOLDER[0] ? products[i+1] : atoi(args[i+1]);
        if(args[i][0] == prioritized[0]){
            products[i+1] = a * b;
            args[i+1] = PRODUCT_PLACEHOLDER;
            found = true;
        }
        else if(args[i][0] == prioritized[1]){
            products[i+1] = a / b;
            args[i+1] = PRODUCT_PLACEHOLDER;
            found = true;
        }
        else if(args[i][0] == prioritized[2]){
            products[i+1] = a % b;
            args[i+1] = PRODUCT_PLACEHOLDER;
            found = true;
        }
        if(found){
            args[i+1] = PRODUCT_PLACEHOLDER;
            args[i-1] = "0";
            args[i] = "+";
        }
    }
}

/**
 * evaluates the expression in the args array and returns the result
 * replaces product placeholders with the actual product
 * @param args
 * @param num_args
 * @param products
 * @return
 */
int eval_left_to_right(char *args[], int num_args, int products[]) {
    int result = atoi(args[1]);
    int right_side;
    for (int i = 2; i < num_args; i=i+2) {
        right_side = args[i+1][0] == PRODUCT_PLACEHOLDER[0] ? products[i+1] : atoi(args[i+1]);
        if (*args[i] == '+') {
            result += right_side;
        }
        else if (*args[i] == '-') {
            result -= right_side;
        }
    }
    return result;
}

/* ******************************************************************
 *  background task functions
 *  ******************************************************************
 */

/**
 * returns true if the given command is a background task
 * @param args
 * @param num_args
 * @return bool
 */
bool is_in_background(char *args[], int num_args) {
    for (int i = 0; i < num_args; i++) {
        if(strcmp(args[i], "&") == 0) {
            for (int j = i; j < num_args - 1; ++j) {
                strcpy(args[j], args[j+1]);
            }
            args[num_args - 1] = NULL;
            return true;
        }
    }
    return false;
}

/**
 * run new shell instance in background
 * if pid is 0, then it is the child process else it is the parent process
 * pid of child process is stored in the global background_tasks array
 * @param args
 * @param num_args
 */
void run_in_background(char *args[], int num_args) {
    pid_t pid = fork();
    if (pid == 0) {
        char file[MAX_CHARS_PER_LINE];
        char *_args[MAX_COMMAND_ARGS];
        strcpy(file, "./shell");
        strcpy(_args[0], "./shell");
        for (int i = 0; i < num_args; i++) {
            strcat(file, " ");
            strcat(file, args[i]);
            strcpy(_args[i+1], args[i]);
        }
        execvp(file, _args);
    } else {
        char pid_str[10];
        background_tasks[background_tasks_count] = malloc(sizeof(args[0]));
        strcpy(background_tasks[background_tasks_count++], args[0]);

        sprintf(pid_str, "%d", pid);

        background_tasks[background_tasks_count] = malloc(sizeof(pid_str));
        strcpy(background_tasks[background_tasks_count++], pid_str);
    }
}

/* ******************************************************************
 *  redirection functions
 *  ******************************************************************
 */

/**
 * parses path out of current by delim
 * returns -1 if path is not in current
 * @param delim
 * @param current
 * @param path
 * @return int
 */
int parse_io_path(char delim, char *current, char *path) {
    if (strlen(current) == 1) return -1;
    char *token = strtok(current, &delim);
    strcpy(path, token);
    return 0;
}

/**
 * sets global io types based on given tokens
 * @param args
 * @param num_args
 */
int set_redirection_io_type(char *args[], int num_args) {
    for (int i = 0; i < num_args; i++) {
        char path[FILENAME_MAX];
        if (args[i][0] == '>') {
            current_output_type = PATH;
            if (parse_io_path('>', args[i], path) == -1) {
                strcpy(current_output_path, args[i+1]);
                num_args--;
                args[i+1][0] = '\0';
            } else {
                strcpy(current_output_path, path);
            }
        } else if (args[i][0] == '<') {
            current_input_type = PATH;
            if (parse_io_path('<', args[i], path) == -1) {
                strcpy(current_input_path, args[i+1]);
                num_args--;
                args[i+1][0] = '\0';
            } else {
                strcpy(current_input_path, path);
            }
        } else continue;

        args[i][0] = '\0';
        num_args--;
    }
    return num_args;
}

/*  ******************************************************************
 *  variable functions
 *  ******************************************************************
 */

/**
 * persists variable in the global variables array
 * @param string
 */
void set_variable(char *string) {
    char temp[strlen(string)];
    strcpy(temp, string);

    char *var_name, *var_value;
    var_name = strtok(temp, "=");
    var_value = strtok(NULL, "=");

    if ( var_value == NULL || var_name == NULL) return;

    variables[variables_count] = malloc(sizeof(var_name));
    strcpy(variables[variables_count++], var_name);

    variables[variables_count] = malloc(sizeof(var_value));
    strcpy(variables[variables_count++], var_value);

    return;
}

/**
 * get variable index in global array
 * @param string
 * @return
 */
int get_variable(char *string) {
    for (int i = 0; i < variables_count; i = i + 2) {
        if (strcmp(string, variables[i]) == 0) {
            return i+1;
        }
    }
    return 0;
}

/**
 * determine if a string contains a variable declaration
 * @param args
 * @param num_args
 * @return
 */
bool is_variable_declaration(char *args, int num_args) {
    char temp[strlen(args)];
    strcpy(temp, args);

    char *var_name, *var_value;
    if (num_args != 1) return false;

    var_name = strtok(temp, "=");
    if ( var_name == NULL ) return false;

    var_value = strtok(NULL, "=");
    if ( var_value == NULL ) return false;
    return true;
}

/**
 * replace all variables in the string with their (if exists) env values.
 * @param string
 * @return
 */
void replace_variables(char *string, char *replaced){
    int start = string[0] == '$' ? 0 : 1;
    int dollar_chunks_count = 0;
    char *dollar_chunks[MAX_VARIABLES];

    if((dollar_chunks[dollar_chunks_count] = strtok(string,"$")) != NULL){
        dollar_chunks_count++;
        while((dollar_chunks[dollar_chunks_count] = strtok(NULL, "$")) != NULL) dollar_chunks_count++;
    }

    for(int i = 0; i < dollar_chunks_count; i++){
        if(i >= start){
            char *var_name = strtok(dollar_chunks[i], " }\n");
            if (var_name[0] == '{') var_name++;

            int var_index = get_variable(var_name);

            if(var_index != 0){
                strcat(replaced, variables[var_index]);
            }
            else{
                strcat(replaced, "$");
                strcat(replaced, var_name);
            }

            char *rest;
            while ((rest = strtok(NULL, " }\n")) != NULL) {
                strcat(replaced, rest);
            }

        } else {
            strcat(replaced, dollar_chunks[i]);
        }
    }
}

/*  ******************************************************************
 *  command functions
 *  ******************************************************************
 */

/**
 * prints every nth argument in the given array (n > 0)
 * doesnt consider quotes
 * @param args
 * @param num_args
 */
void echo(char *args[], int num_args) {
    int i;
    char out[300];
    for (i = 1; i < num_args; i++) {
        sprintf(out, "%s%s", args[i], i == 1 ? "" : " ");
        output(out);
    }
    if (current_output_type == CONSOLE) output("\n");
}

/**
 * changes the directory to the given argument with chdir from unistd.h
 * if no arguments are given, it changes to the home directory
 * @param args
 * @param num_args
 */
void cd(char *args[], int num_args) {
    if (num_args == 1) {
        chdir(getenv("HOME"));
    } else {
        chdir(args[1]);
    }
}

/**
 * prints the current working directory returned by getcwd from unistd.h
 */
void pwd() {
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    output(cwd);
    output("\n");
}

/**
 * evaluates and prints a given expression
 * arithmetic operators only
 * @param args
 * @param num_args
 */
void arithmetic_expr(char *args[], int num_args) {
    if (expression_is_valid(args, num_args) == false) {
        output("Invalid expression\n");
        return;
    }
    int products[num_args];
    char out[300];
    eval_prioritized_only(args, num_args, products);
    sprintf(out,"%d\n", eval_left_to_right(args, num_args, products));
    output(out);
}

/**
 * replaces substrings of args[1] with args[2]
 * base string is given from input()
 * @param args
 */
void tr(char *args[]) {
    char *from = args[1];
    char *to = args[2];
    char string[100];

    input(string);

    char *result = malloc(MAX_PIPE_VALUE);

    for (int i = 0; i < strlen(string); i++) {
        if (string[i] == from[0]) {
            if (strlen(from) == 1) {
                strncat(result, to, strlen(to));
            }
            for (int j = 1; j < strlen(from); j++) {
                if (strlen(from) > 1 && string[i+j] != from[j]) break;
                strncat(result, to, strlen(to));
            }
        } else {
            strncat(result, &string[i], 1);
        }
    }
    output(result);
    output("\n");
    free(result);
}

/**
 * outputs contents of a file
 * @param args
 * @param num_args
 */
void cat(char *args[], int num_args) {
    char out[FILENAME_MAX + 300];
    char path[FILENAME_MAX];

    if (num_args == 1) {
        input(path);
    } else {
        strcpy(path, args[1]);
    }

    /*
     * the following code should be in a separate function
     * if more commands are added that require file reading
     */
    if (args[1][0] == '-') {
        input(out);
        output(out);
        output("\n");
        return;
    }

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        sprintf(out, "cat: %s: No such file or directory\n", path);
        output(out);
    }
    char c;
    while ((c = fgetc(file)) != EOF) {
        sprintf(out, "%c", c);
        output(out);
    }
    fclose(file);
    output("\n");
}

/**
 * remove file or directory
 * @param args
 * @param num_args
 */
void rm(char *args[], int num_args) {
    char out[FILENAME_MAX + 300];
    char path[FILENAME_MAX];

    if (num_args == 1) {
        sprintf(out, "rm: missing operand\n");
        output(out);
        return;
    } else {
        strcpy(path, args[1]);
    }

    if (remove(path) != 0) {
        sprintf(out, "rm: cannot remove '%s': No such file or directory\n", path);
        output(out);
    }
}

/**
 * wrapper function for unistd sleep
 * @param args
 * @param num_args
 */
void _sleep(char *args[], int num_args) {
    if (num_args == 1) {
        output("sleep: missing operand\n");
        return;
    }
    int seconds = atoi(args[1]);
    sleep(seconds);
}

/**
 * search pids by name in global array
 * kill process by pid
 * @param args
 * @param num_args
 */
void pkill(char *args[], int num_args) {
    if (num_args < 2) {
        output("pkill: missing operand\n");
        return;
    }
    char *name = args[1];
    for(int i = 0; i < background_tasks_count; i = i + 2) {
        if (strcmp(name, background_tasks[i]) == 0) {
            int pid = atoi(background_tasks[i + 1]);
            kill(pid, SIGKILL);
        }
    }
}

/*  ******************************************************************
 *  shell loop and dispatching
 *  ******************************************************************
 */

/**
 * dispatches the given command to the appropriate function
 * returns -1 if exit is called; returns 1 if the command is not found; returns 0 otherwise
 * @param args
 * @param num_args
 * @return int
 */
int dispatch_command(char *args[], int num_args) {
    if (in_background) {
        run_in_background(args, num_args);
    } else if (strcmp(args[0], "echo") == 0) {
        echo(args, num_args);
    } else if (strcmp(args[0], "cd") == 0) {
        cd(args, num_args);
    } else if (strcmp(args[0], "pwd") == 0) {
        pwd();
    } else if (strcmp(args[0], "exit") == 0) {
        return -1;
    } else if (strcmp(args[0], "expr") == 0) {
        arithmetic_expr(args, num_args);
    } else if (strcmp(args[0], "tr") == 0) {
        tr(args);
    } else if (strcmp(args[0], "cat") == 0) {
        cat(args, num_args);
    } else if (strcmp(args[0], "rm") == 0) {
        rm(args, num_args);
    } else if (strcmp(args[0], "sleep") == 0) {
        _sleep(args, num_args);
    } else if (strcmp(args[0], "true") == 0) {
        output("");
    } else if (strcmp(args[0], "pkill") == 0) {
        pkill(args, num_args);
    } else if (is_variable_declaration(args[0], num_args)) {
        set_variable(args[0]);
    } else {
        printf("Unknown command: %s\n", args[0]);
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    char line[MAX_CHARS_PER_LINE];
    char replaced[MAX_CHARS_PER_LINE];
    char *tokens[MAX_COMMAND_ARGS];
    char *commands[MAX_COMMANDS];

    int tokens_count;
    int commands_count;

    /* ******************************************************************
     *  a command can be executed over args of main program
     *  ******************************************************************
     */
    if (argc > 1) {
        for (int i = 0; i < argc - 1; i++) {
            strcpy(argv[i], argv[i+1]);
        }
        argv[argc - 1] = NULL;

        return dispatch_command(argv, argc - 1);
    }

    for (;;) {
        printf("%c ", '$');

        memset(line, '\0', MAX_CHARS_PER_LINE );
        memset(replaced, '\0', MAX_CHARS_PER_LINE );

        if (fgets(line, MAX_CHARS_PER_LINE, stdin) == NULL) break;
        if (line[0] == '\n') continue;

        replace_variables(line, replaced);

        commands_count = str_split(replaced, "|", commands);
        if (commands_count == -1) continue;

        for (int i = 0; i < commands_count; i++) {
            tokens_count = str_split(commands[i], " \n\t", tokens);

            if (last_pipe_buffer[0] != '\0') current_input_type = PIPE;
            if (commands_count > 1 && i < commands_count -1) current_output_type = PIPE;

            tokens_count = set_redirection_io_type(tokens, tokens_count);


            in_background = is_in_background(tokens, tokens_count);
            if (in_background) tokens_count--;

            if (dispatch_command(tokens, tokens_count) == -1) break;

            current_output_type = CONSOLE;
            current_input_type = CONSOLE;
            new_path_output = true;

            /* end pipe buffer with zero byte */
            strcat(pipe_buffer, "\0");

            /* copy pipe buffer to last pipe buffer and reset */
            strcpy(last_pipe_buffer, pipe_buffer);
            strcpy(pipe_buffer, "");

            /* reset background bool */
            in_background = false;
        }
    }
    return 0;
}