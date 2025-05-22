#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdint.h>


#define BLOCK_SIZE 1024

enum CODES {
    SUCCESS = 0,
    ERROR_OPEN_FILE,
    ERROR_MALLOC,
    ERROR_FORK,
    ERROR_EMPTY_SEARCH_STRING,
    ERROR_SYNTAX,
    ERROR_LENGTH_OF_MASK,
    ERROR_INVALID_N
};


int xorN(const char *filename, int n) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        return ERROR_OPEN_FILE;
    }

    size_t bits_requested = 1 << n;
    size_t total_bytes = bits_requested / 8 == 0 ? 1 : bits_requested / 8;

    uint8_t* cur_bytes = calloc(total_bytes, sizeof(uint8_t));
    if (cur_bytes == NULL) {
        fclose(file);
        return ERROR_MALLOC;
    }
    uint8_t* result = calloc(total_bytes, sizeof(uint8_t));
    if (result == NULL) {
        fclose(file);
        free(cur_bytes);
        return ERROR_MALLOC;
    }

    size_t bytes_read;
    while ((bytes_read = fread(cur_bytes, sizeof(uint8_t), total_bytes, file)) > 0) {
        if (bytes_read < total_bytes) {
            memset(cur_bytes + bytes_read, 0, total_bytes - bytes_read);
        }
        if (n > 2) {
            for (size_t i = 0; i < total_bytes; i++) {
                result[i] ^= cur_bytes[i];
                //printf("cur_bytes[i] = %d, result[%llu] = %d\n", cur_bytes[i], i, result[i]);
            }
        }
        if (n == 2) {
            uint8_t mask = (1 << 4) - 1;
            //printf("cur_bytes[0] = %d\ncur_bytes[0]>>4 = %d\n", cur_bytes[0], cur_bytes[0]>>4);
            uint8_t left = cur_bytes[0] >> 4, right = cur_bytes[0] & mask;
            result[0] ^= left;
            result[0] ^= right;
            //printf("left = %d, right = %d, result[0] = %d\n", left, right, result[0]);
        }
    }

    printf("XOR result for file '%s' with N=%d (%zu bits):\n", filename, n, bits_requested);
    if (n > 2) {
        for (size_t i = 0; i < total_bytes; i++) {
            printf("%x ", result[i]);
        }
    }

    if (n > 2) {
        printf("%x (only 4 bits)", result[total_bytes]);
    }
    printf("\n");

    free(cur_bytes);
    free(result);
    fclose(file);
    return SUCCESS;
}

int mask(const char *filename, const uint32_t mask_value) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        return ERROR_OPEN_FILE;
    }

    uint32_t value;
    int count = 0;

    while (fread(&value, sizeof(uint32_t), 1, file)) {
        if ((value & mask_value) == mask_value) {
            count++;
        }
    }

    fclose(file);

    printf("Mask count for %s: %d\n", filename, count);
    return SUCCESS;
}

int copyN(const char *filename, int n) {
    for (int i = 1; i <= n; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            char new_filename[256];
            snprintf(new_filename, sizeof(new_filename), "%s_%d", filename, i);

            FILE *src_file = fopen(filename, "rb");
            if (!src_file) {
                perror("Failed to open source file");
                exit(EXIT_FAILURE);
            }

            FILE *dst_file = fopen(new_filename, "wb");
            if (!dst_file) {
                perror("Failed to open destination file");
                fclose(src_file);
                exit(EXIT_FAILURE);
            }

            uint8_t buffer[BLOCK_SIZE];
            size_t bytes_read;

            while ((bytes_read = fread(buffer, 1, BLOCK_SIZE, src_file))) {
                fwrite(buffer, 1, bytes_read, dst_file);
            }

            fclose(src_file);
            fclose(dst_file);

            exit(EXIT_SUCCESS);
        }
        if (pid < 0) {
            perror("Failed to fork");
            return ERROR_FORK;
        }
    }

    for (int i = 0; i < n; i++) {
        wait(NULL);
    }
    return SUCCESS;
}

int find(const char *filename, const char search_string[]) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        return ERROR_OPEN_FILE;
    }

    char line[BLOCK_SIZE];
    int found = 0;

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, search_string)) {
            found = 1;
            break;
        }
    }

    fclose(file);

    if (found) {
        printf("String found in %s\n", filename);
    } else {
        printf("String not found in %s\n", filename);
    }
    return SUCCESS;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Syntax must be: file1 file2 ... flag [args]\n");
        return ERROR_SYNTAX;
    }

    char *flag = argv[argc - 1];
    int file_count = argc - 2;

    if (strncmp(flag, "xor", 3) == 0) {
        int n = atoi(flag + 3);
        if (n < 2 || n > 6) {
            fprintf(stderr, "Invalid N value for xorN\n");
            return ERROR_INVALID_N;
        }

        for (int i = 1; i <= file_count; i++) {
            xorN(argv[i], n);
        }
    }
    else if (strncmp(flag, "mask<", 5) == 0) {
        if (argc < 3) {
            fprintf(stderr, "Syntax must be: file1 file2 ... mask'<hex>'\n");
            return ERROR_SYNTAX;
        }

        char *p = argv[argc - 1] + 5, mask_input[8];
        int j = 0;
        while(*p != '>') {
            if (j >= 8) {
                printf("Length of mask must be less then 9");
                return ERROR_LENGTH_OF_MASK;
            }
            if (*p == '\0' || *p == '\n') {
                fprintf(stderr, "Syntax must be: file1 file2 ... mask'<hex>'\n");
                return ERROR_SYNTAX;
            }
            mask_input[j++] = *p++;
        }
        mask_input[j] = '\0';

        uint32_t mask_value = strtoul(mask_input, NULL, 16);

        for (int i = 1; i <= file_count; i++) {
            mask(argv[i], mask_value);
        }
    }
    else if (strncmp(flag, "copy", 4) == 0) {
        int n = atoi(flag + 4);
        if (n <= 0) {
            fprintf(stderr, "Invalid N value for copyN\n");
            return ERROR_SYNTAX;
        }

        for (int i = 1; i <= file_count; i++) {
            copyN(argv[i], n);
        }
        printf("Copied successfully\n");
    }
    else if (strncmp(flag, "find<", 5) == 0) {
        if (argc < 3) {
            fprintf(stderr, "Syntax must be: file1 file2 ... find'<string>'\n");
            return ERROR_SYNTAX;
        }

        char *p = argv[argc - 1] + 5, search_string[BUFSIZ];
        int j = 0;
        while(*p != '>') {
            if (*p == '\0' || *p == '\n') {
                fprintf(stderr, "Syntax must be: file1 file2 ... find'<string>'\n");
                return ERROR_SYNTAX;
            }
            search_string[j++] = *p++;
        }
        search_string[j] = '\0';

        for (int i = 1; i <= file_count; i++) {
            pid_t pid = fork();
            if (pid == 0) {
                find(argv[i], search_string);
                exit(EXIT_SUCCESS);
            }
            if (pid < 0) {
                perror("Failed to fork\n");
                return ERROR_FORK;
            }
        }

        for (int i = 0; i < file_count; i++) {
            wait(NULL);
        }
    }
    else {
        fprintf(stderr, "Unknown flag: %s\n", flag);
        return ERROR_SYNTAX;
    }

    return SUCCESS;
}