#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define MAX_LINES 100
#define MAX_CHARS_PER_LINE 100

/**
 * sort algorithm for string arrays
 * asc if order > 0
 * desc if order < 0
 * @param strings char*
 * @param strings_c  int
 * @param order int
 * @return void
 */
void bubble_sort_alphabetical(char *strings[], int strings_c, int order) {
    char temp[MAX_CHARS_PER_LINE];
    int i,j;
    for(i = 0; i < strings_c; i++){
        for(j = i + 1; j < strings_c; j++) {
            if(strcmp(strings[i],strings[j]) * order > 0) {
                strcpy(temp,strings[i]);
                strcpy(strings[i],strings[j]);
                strcpy(strings[j],temp);
            }
        }
    }
}

/**
 * prints array of strings line by line
 * @param strings char*
 * @param strings_c int
 * @return void
 */
void print_strings(char *strings[], int strings_c) {
    for (int i = 0; i < strings_c; i++) {
        printf("%s", strings[i]);
    }
}

int main(int argc, char **argv) {
    char *lines[MAX_LINES];
    int lines_c = 0;

    bool reverse = argc > 1 && strcmp(argv[1], "-r") == 0;

    char line[MAX_CHARS_PER_LINE];
    for (;;) {
        if ( fgets( line, MAX_CHARS_PER_LINE, stdin) == NULL ) break;
        lines[lines_c] = strdup(line);
        lines_c++;
    }

    bubble_sort_alphabetical(lines, lines_c, reverse ? -1 : 1);
    print_strings(lines, lines_c);

    return 0;
}