/*
 * mkdirs - Create recursive directory structures
 * Usage: mkdirs <path>
 *        mkdirs <path>/[dir1,dir2,dir3]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* Create directory recursively */
int mkdir_recursive(const char *path) {
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;
    int ret;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    
    /* Remove trailing slash if present */
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }

    /* Create directories recursively */
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            ret = mkdir(tmp, 0755);
            if (ret != 0 && errno != EEXIST) {
                return -1;
            }
            *p = '/';
        }
    }
    
    /* Create the final directory */
    ret = mkdir(tmp, 0755);
    if (ret != 0 && errno != EEXIST) {
        return -1;
    }
    
    return 0;
}

/* Parse and expand brace expansion like path/[a,b,c] */
int expand_and_create(const char *cwd, const char *path_arg) {
    char *bracket_open = strchr(path_arg, '[');
    char *bracket_close = strchr(path_arg, ']');
    
    /* No brace expansion found */
    if (!bracket_open || !bracket_close || bracket_close < bracket_open) {
        char full_path[PATH_MAX];
        const char *clean_path = path_arg;
        
        /* Remove leading slash if present */
        if (clean_path[0] == '/') {
            clean_path++;
        }
        
        snprintf(full_path, sizeof(full_path), "%s/%s", cwd, clean_path);
        
        if (mkdir_recursive(full_path) != 0) {
            fprintf(stderr, "Error: Failed to create %s: %s\n", 
                    full_path, strerror(errno));
            return 1;
        }
        
        printf("%s has been created\n", full_path);
        return 0;
    }
    
    /* Extract base path (before '[') */
    size_t base_len = bracket_open - path_arg;
    char base_path[PATH_MAX];
    strncpy(base_path, path_arg, base_len);
    base_path[base_len] = '\0';
    
    /* Remove leading slash from base path */
    const char *clean_base = base_path;
    if (clean_base[0] == '/') {
        clean_base++;
    }
    
    /* Extract suffix (after ']') */
    const char *suffix = bracket_close + 1;
    
    /* Parse items inside brackets */
    char items[PATH_MAX];
    size_t items_len = bracket_close - bracket_open - 1;
    strncpy(items, bracket_open + 1, items_len);
    items[items_len] = '\0';
    
    /* Split by comma and create directories */
    char *item = strtok(items, ",");
    int error_occurred = 0;
    
    while (item != NULL) {
        /* Trim leading/trailing spaces */
        while (*item == ' ') item++;
        char *end = item + strlen(item) - 1;
        while (end > item && *end == ' ') {
            *end = '\0';
            end--;
        }
        
        /* Build full path */
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s%s%s", 
                 cwd, clean_base, item, suffix);
        
        /* Create directory */
        if (mkdir_recursive(full_path) != 0) {
            fprintf(stderr, "Error: Failed to create %s: %s\n", 
                    full_path, strerror(errno));
            error_occurred = 1;
        } else {
            printf("%s has been created\n", full_path);
        }
        
        item = strtok(NULL, ",");
    }
    
    return error_occurred ? 1 : 0;
}

int main(int argc, char *argv[]) {
    char cwd[PATH_MAX];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <path>\n", argv[0]);
        fprintf(stderr, "Example: %s /src/main/java\n", argv[0]);
        fprintf(stderr, "Example: %s src/main/java\n", argv[0]);
        fprintf(stderr, "Example: %s src/main/[java,res,imgs]\n", argv[0]);
        return 1;
    }

    /* Get current working directory */
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");
        return 1;
    }

    /* Expand and create directories */
    return expand_and_create(cwd, argv[1]);
}