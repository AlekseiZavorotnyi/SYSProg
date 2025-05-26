#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>

typedef enum {
    SIMPLE_MODE,
    DETAILED_MODE,
    ALL_MODE,
    SUCCESS = 0,
    ERROR_READ_PATH,
    ERROR_OPEN_FILE,
    ERROR_FILE_STAT,
    ERROR_OPEN_DIR,
    ERROR_UNKNOWN_FLAG,
} StatusCode;

typedef enum {
    COLOR_RESET = 0,
    COLOR_GREEN = 32,
    COLOR_BLUE_ON_GREEN = 42
} TextColor;

void set_file_permissions(const mode_t mode, char* permissions) {
    switch (mode & S_IFMT) {
        case S_IFREG: permissions[0] = '-'; break;
        case S_IFDIR: permissions[0] = 'd'; break;
        case S_IFBLK: permissions[0] = 'b'; break;
        case S_IFCHR: permissions[0] = 'c'; break;
        case S_IFIFO: permissions[0] = 'p'; break;
        case S_IFSOCK: permissions[0] = 's'; break;
        case S_IFLNK: permissions[0] = 'l'; break;
        default: perror("set_file_permissions"); break;
    }

    permissions[1] = (mode & S_IRUSR) ? 'r' : '-';
    permissions[2] = (mode & S_IWUSR) ? 'w' : '-';
    permissions[3] = (mode & S_IXUSR) ? 'x' : '-';
    permissions[4] = (mode & S_IRGRP) ? 'r' : '-';
    permissions[5] = (mode & S_IWGRP) ? 'w' : '-';
    permissions[6] = (mode & S_IXGRP) ? 'x' : '-';
    permissions[7] = (mode & S_IROTH) ? 'r' : '-';
    permissions[8] = (mode & S_IWOTH) ? 'w' : '-';
    permissions[9] = (mode & S_IXOTH) ? 'x' : '-';
    permissions[10] = '\0';
}

void print_colored(const char* text, TextColor color) {
    if (color == COLOR_BLUE_ON_GREEN) {
        printf("\033[42;34m%s\033[0m", text);
    } else {
        printf("\033[%dm%s\033[0m", color, text);
    }
}

StatusCode list_directory(DIR* dir, const char* dir_path, const int display_mode) {
    struct dirent *entry;
    struct stat file_info;

    printf("Contents of %s:\n", dir_path);

    if (chdir(dir_path)) {
        return ERROR_OPEN_DIR;
    }

    while ((entry = readdir(dir)) != NULL) {
        // Skip hidden files in detailed mode without -a
        if (display_mode == DETAILED_MODE && entry->d_name[0] == '.') {
            continue;
        }

        const int fd = open(entry->d_name, O_RDONLY);
        if (fd < 0) {
            return ERROR_OPEN_FILE;
        }

        if (fstat(fd, &file_info) < 0) {
            close(fd);
            return ERROR_FILE_STAT;
        }

        if (display_mode == SIMPLE_MODE) {
            TextColor color = S_ISDIR(file_info.st_mode) ? COLOR_BLUE_ON_GREEN : COLOR_GREEN;
            print_colored(entry->d_name, color);
            printf(" ");
        }
        else if (display_mode == DETAILED_MODE || display_mode == ALL_MODE) {
            const struct passwd *owner = getpwuid(file_info.st_uid);
            const struct group *group = getgrgid(file_info.st_gid);
            if (!owner || !group) {
                close(fd);
                return ERROR_FILE_STAT;
            }

            char permissions[11];
            set_file_permissions(file_info.st_mode, permissions);

            char time_buf[20];
            strftime(time_buf, sizeof(time_buf), "%b %d %H:%M", localtime(&file_info.st_mtime));

            printf("%10s %2ld %8s %8s %7ld %s ",
                   permissions, file_info.st_nlink, owner->pw_name,
                   group->gr_name, file_info.st_size, time_buf);

            TextColor color = S_ISDIR(file_info.st_mode) ? COLOR_BLUE_ON_GREEN : COLOR_GREEN;
            print_colored(entry->d_name, color);
            printf("\n");
        }
        close(fd);
    }
    printf("\n");
    return SUCCESS;
}

StatusCode process_arguments(int argc, const char **argv) {
    int display_mode = SIMPLE_MODE;
    int flag_index = argc - 1;

    // Determine display mode from last argument
    if (argc > 1) {
        if (strcmp(argv[flag_index], "-l") == 0) {
            display_mode = DETAILED_MODE;
        } else if (strcmp(argv[flag_index], "-la") == 0 || strcmp(argv[flag_index], "-al") == 0) {
            display_mode = ALL_MODE;
        }
    }

    // If no directories specified or only flag provided
    if (argc == 1 || (argc == 2 && display_mode != SIMPLE_MODE)) {
        char current_path[1024];
        if (getcwd(current_path, sizeof(current_path))) {
            DIR *dir = opendir(current_path);
            if (dir) {
                StatusCode status = list_directory(dir, current_path, display_mode);
                closedir(dir);
                return status;
            }
        }
        printf("Could not open current directory\n");
        return ERROR_OPEN_DIR;
    }

    // Process each directory argument
    for (int i = 1; (display_mode == SIMPLE_MODE) ? (i < argc) : (i < argc - 1); i++) {
        DIR *dir = opendir(argv[i]);
        if (dir) {
            StatusCode status = list_directory(dir, argv[i], display_mode);
            closedir(dir);
            if (status != SUCCESS) {
                return status;
            }
        } else {
            printf("Could not open directory '%s'\n", argv[i]);
        }
    }
    return SUCCESS;
}

int main(int argc, const char *argv[]) {
    return process_arguments(argc, argv);
}