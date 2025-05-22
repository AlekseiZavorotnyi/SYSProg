#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

enum ret_type_t {
    SUCCESS,
    ERROR
};

typedef void(*callback)(FILE*, FILE*, int);

char* f10to16(int x){
    char buf[4], *pb = buf + 3;
    *pb-- = 0;
    while (x) {
        int r = x % 16;
        *pb-- = (r > 9) ? r - 10 + 'A' : r + '0';
        x /= 16;
    }
    pb++;
    return pb;
}

void funcForDA(FILE* fi, FILE* fiout, const int ind) {
    while(!feof(fi)) {
        char ch = fgetc(fi);
        if (ch == EOF) {
            break;
        }
        if (ind == 0) {
            if (!isdigit(ch)) {
                fputc(ch, fiout);
            }
        }
        else {
            if (ch == '\n') {
                fputc('\n', fiout);
                continue;
            }
            if(!isdigit(ch)) {
                fputs(f10to16(ch), fiout);
            }
            else {
                fputc(ch, fiout);
            }
        }
    }
}

void funcForIS(FILE* fi, FILE* fiout, const int ind) {
    int cnt = 0;
    while(!feof(fi)) {
        char ch = fgetc(fi);
        if (ch == '\n' || ch == EOF) {
            fputc(cnt + '0', fiout);
            fputc('\n', fiout);
            cnt = 0;
            continue;
        }
        if (ind == 1) {
            if (isalpha(ch)) {
                cnt++;
            }
        }
        else {
            if (!isalnum(ch) && ch != ' ') {
                cnt++;
            }
        }
    }
}

int findFlag(const char* qarg, const char** flags, int size) {
    for (int i = 0; i < size; ++i)
    {
        if (!(strcmp(qarg, flags[i])))
        {
            return i;
        }
    }
    return -1;
}

int validpath(char* path) {
    int l = strlen(path);
    char* ptr = path + l - 1;
    while (*ptr != '.' and l > 0) {
        --ptr;
        l--;
    }
    ++ptr;
    if(strcmp(ptr, "txt") != 0) {
        return -1;
    }
    while (isalpha(*ptr) || isdigit(*ptr) || *ptr == '-' || *ptr == '_' || *ptr == '.' || *ptr == '\\') {
        --ptr;
    }
    if (*ptr != ':' || !isupper(*(ptr - 1)) || *(ptr + 1) != '\\') {
        return -1;
    }
    return 1;
}

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 4 || (argv[1][1] != 'n' && argc != 3)) {
        printf("Wrong number of args");
        return -1;
    }
    const char* flags[] = { "-nd", "/nd", "-d", "/d", "-ni", "/ni", "-i", "/i", "-ns", "/ns", "-s", "/s", "-na", "/na", "-a", "/a" };
    callback cbs[] = { &funcForDA, &funcForIS, &funcForIS, &funcForDA };
    int ret = findFlag(argv[1], flags, sizeof(flags) / sizeof(char*));
    if (ret == -1)
    {
        printf("THIS FLAG DOES NOT EXIST %s", argv[1]);
        return -1;
    }
    FILE* fi = NULL, *fiout = NULL;
    fi = fopen(argv[2], "r");

    if (argv[1][1] == 'n' && argc == 4) {
        char* path = argv[3];
        if (validpath(path) == -1) {
            printf("Can not be a path");
            return -1;
        }
        fiout = fopen(argv[3], "w");
    }
    else {
        char pref[5] = "_out", *gde = argv[2];
        int gdeind = 0, cnt = 0;
        while(*gde) {
            if (*gde++ == '/') {
                gdeind = cnt;
            }
            cnt++;
        }
        char *endind = &argv[2][gdeind + 1], endad[strlen(argv[2]) - gdeind];
        int i = 0;
        while(*endind) {
            endad[i++] = *endind++;
        }
        argv[2][gdeind + 1] = '\0';
        fiout = fopen(strcat(strcat(argv[2], pref), endad), "w");
    }
    if (fi == NULL || fiout == NULL){
        printf("file was not found");
        fclose(fi);
        fclose(fiout);
        return -1;
    }
    int findCbsInt = ret / 4;
    callback find = cbs[findCbsInt];
    find(fi, fiout, findCbsInt);
    printf("the program has been completed successfully");
    fclose(fi);
    fclose(fiout);
    return 0;
}