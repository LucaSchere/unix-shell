#include <stdio.h>
#include <stdbool.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

/**
 * counts elements in given path
 * returns count of elements
 * returns -1 if path is not readable
 * @param path char*
 * @return int
 */
int count_elements_in(char *path) {
    DIR *dir;
    if ((dir = opendir(path)) == NULL) {
        return -1;
    }
    int i = 0;
    while (readdir(dir)) {
        i++;
    }
    closedir(dir);
    return i;
}

/**
 * read elements names in given path and store them in given array
 * returns count of read elements
 * @param path char*
 * @param elements char**
 * @return int
 */
int read_names(const char *path, char **elements_names) {
    DIR *dir = opendir(path);
    struct dirent *entry;
    int i = 0;
    while ((entry = readdir(dir))) {
        elements_names[i++] = strdup(entry->d_name);
    }
    closedir(dir);
    return i - 1;
}

/**
 * parse path based on (if given) first argument
 * store parsed path in given array
 * @param path char*
 * @param argc int
 * @param argv char**
 * @return void
 */
void parse_dir(char *path, const int argc, char **argv) {
    if (argc > 1) {
        strcpy(path, argv[1]);
    } else {
        strcpy(path, ".");
    }
    return;
}
/**
 * returns true if given path is a symlink
 * @param path_to_file char*
 * @return bool
 */
bool is_symlink(const char *path_to_file) {
    struct stat sb;
    return (lstat(path_to_file, &sb) == 0 && S_ISLNK(sb.st_mode));
}

/**
 * returns true if given path is a executable file
 * @param path_to_file char*
 * @return bool
 */
bool is_executable(const char *path_to_file) {
    struct stat sb;
    return (stat(path_to_file, &sb) == 0 && sb.st_mode & S_IXUSR);
}

/**
 * returns true if given path is a directory
 * @param path_to_file char*
 * @return bool
 */
bool is_dir(const char *path_to_file) {
    struct stat sb;
    return (stat(path_to_file, &sb) == 0 && S_ISDIR(sb.st_mode));
}

/**
 * appends * if given path is a executable file
 * appends / if given path is a directory
 * appends @ if given path is a symlink
 * @param file_name char*
 * @param path_to_file char*
 * @return void
 */
void append_suffix(char *file_name, char *path_to_file) {
    char appended[FILENAME_MAX];
    strcpy(appended, file_name);
    if (is_dir(path_to_file)) {
        strcat(appended, "/");
    } else if (is_symlink(path_to_file)) {
        strcat(appended,"@");
    } else if (is_executable(path_to_file)) {
        strcat(appended,"*");
    }
    strcpy(file_name, appended);
    return;
}

/**
 * sort algorithm for string arrays
 * @param strings char*
 * @param strings_c  int
 * @return void
 */
void sort_alphabetical(char *strings[], int strings_c) {
    char temp[FILENAME_MAX];
    int i,j;
    for(i = 0; i < strings_c; i++){
        for(j = i + 1; j < strings_c; j++) {
            if(strcmp(strings[i],strings[j])>0) {
                strcpy(temp,strings[i]);
                strcpy(strings[i],strings[j]);
                strcpy(strings[j],temp);
            }
        }
    }
}

int main(int argc, char **argv) {
    char working_path[PATH_MAX];
    parse_dir(working_path, argc, argv);

    int elements_count = count_elements_in(working_path);

    if (elements_count == -1) {
        printf("%s\n", working_path);
        return 0;
    }

    char *elements_names[elements_count];
    read_names(working_path, elements_names);

    // no sort required for this task
    // sort_alphabetical(elements_names, elements_count);

    for (int i = 0; i < elements_count; ++i) {
        char path_to_file[PATH_MAX + FILENAME_MAX];

        sprintf(path_to_file, "%s/%s", working_path, elements_names[i]);
        append_suffix(elements_names[i], path_to_file);

        if (elements_names[i][0] != '.') {
            printf("%s\n", elements_names[i]);
        }
    }
    return 0;
}