#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <linux/limits.h>

enum errors {
    SUCCESS,
    ERROR_OPEN_DIR,
    ERROR_STAT
};

int list_files(const char *dirpath) {
    DIR *dir = opendir(dirpath);
    if (dir == NULL) {
        printf("ERROR: open directory");
        return ERROR_OPEN_DIR;
    }
    struct dirent* entry;
    struct stat file_stat;
    while((entry = readdir(dir)) != NULL) {
        char filepath[PATH_MAX];
        if (strcmp(dirpath, entry->d_name) != 0) {
            snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, entry->d_name);
        }
        else {
            snprintf(filepath, sizeof(filepath), "%s", dirpath);
        }


        if (stat(filepath, &file_stat) == -1) {
            printf("ERROR: function stat");
            continue;
        }

        printf("%s", entry->d_name);

        if (S_ISDIR(file_stat.st_mode)) {
            printf(" (directory) ");
        } else if (S_ISREG(file_stat.st_mode)) {
            printf(" (regular file) ");
        } else if (S_ISLNK(file_stat.st_mode)) {
            printf(" (symbolic link) ");
        } else {
            printf(" (other) ");
        }

        printf(" [inode: %lu] [size: %ld bytes]", (unsigned long)file_stat.st_ino, (long)file_stat.st_size);

        char timebuf[80];
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&file_stat.st_mtime));
        printf(" [modified: %s]", timebuf);

        struct passwd *pw = getpwuid(file_stat.st_uid);
        struct group *gr = getgrgid(file_stat.st_gid);
        printf(" [owner: %s]", pw ? pw->pw_name : "unknown");
        printf(" [group: %s]\n", gr ? gr->gr_name : "unknown");

    }
    closedir(dir);
    return SUCCESS;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Syntax must be: directory1 directory2 ...\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; i++) {
        printf("Files in directory: %s\n", argv[i]);
        list_files(argv[i]);
        printf("\n");
    }

    return SUCCESS;
}