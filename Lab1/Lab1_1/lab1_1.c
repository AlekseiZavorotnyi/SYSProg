#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <crypt.h>
#include <limits.h>

enum CODES
{
    Syntax_of_operation = INT_MIN,
    Syntax_of_password,
    Open_the_file,
    Wrong_answer,
    Syntax_of_login,
    Exit,
    There_is_no_such_option,
    SUCCESS,
    No_such_login,
    Logout = 2
};

int my_strptime(const char *str, const char *format, struct tm *tm) {
    int scanned = 0;

    if (strcmp(format, "%d.%m.%Y") == 0) {
        scanned = sscanf(str, "%2d.%2d.%4d", &tm->tm_mday, &tm->tm_mon, &tm->tm_year);
        if (scanned == 3) {
            tm->tm_year -= 1900;
            tm->tm_mon -= 1;
            return 1;
        }
    } else if (strcmp(format, "%H:%M:%S") == 0) {
        scanned = sscanf(str, "%2d:%2d:%2d", &tm->tm_hour, &tm->tm_min, &tm->tm_sec);
        if (scanned == 3) {
            return 1;
        }
    }
    return 0;
}

int encrypt_password(const char *password, char **encrypted_password)
{

    char *salt = crypt_gensalt_ra(NULL, 0, NULL, 0);
    if (!salt)
    {
        return -1;
    }

    void *enc_ctx = NULL;
    int enc_cxt_sz = 0;
    char *tmp_encrypted_password = crypt_ra(password, salt, &enc_ctx, &enc_cxt_sz);

    if (tmp_encrypted_password == NULL)
    {
        free(salt);
        return -1;
    }

    *encrypted_password = (char *)calloc((strlen(tmp_encrypted_password) + 1), sizeof(char));

    strcpy(*encrypted_password, tmp_encrypted_password);

    free(enc_ctx);
    free(salt);
    return 0;
}

int compare_passwords(const char *password, const char *hashed_password, int *compare_res)
{
    void *enc_ctx = NULL;
    int enc_cxt_sz = 0;

    char *hashed_entered_password = crypt_ra(password, hashed_password, &enc_ctx, &enc_cxt_sz);
    if (!hashed_entered_password)
    {
        return -1;
    }

    *compare_res = strcmp(hashed_password, hashed_entered_password);

    free(enc_ctx);
    return 0;
}

int check_login(char login[7]) {
    for (int i = 0; i < 7; i++) {
        if (login[i] == '\n' || login[i] == '\0') {
            return 1;
        }
        if (!isalnum(login[i])) {
            return 0;
        }
    }
    return 0;
}

int check_password(char password[7]) {
    int pass;
    sscanf(password, "%d", &pass);
    if (pass > 100000) {
        return 0;
    }
    for (int i = 0; i < 7; i++) {
        if (password[i] == '\n' || password[i] == '\0') {
            return 1;
        }
        if (!isdigit(password[i])) {
            return 0;
        }
    }
    return 0;
}

int Try_again_menu(int Code) {
    printf("\nTry again: 1\nExit: 2\n");
    printf("\nYour choose:");
    int choose;
    scanf("%d", &choose);
    if (choose == 1) {
        if (Code == Syntax_of_operation) {
            return Syntax_of_operation;
        }
        if (Code == Open_the_file) {
            return Exit;
        }
    }
    if (choose == 2) {
        return Exit;
    }
    printf("There is no such option\n");
    return There_is_no_such_option;
}

int menu() {
    while(1) {
        printf("Do you have an account?(Y/N/Exit):");
        char is_acc[5];
        int scanned = scanf("%s", is_acc);

        is_acc[strcspn(is_acc, "\n")] = '\0';

        if (!strcmp(is_acc, "Y")) {
            char login[7];
            while(1) {
                printf("Enter your login:");
                scanned = scanf("%s", login);
                while(scanned != 1 || !check_login(login)) {
                    fprintf(stderr, "Error: syntax of login\n");
                    printf("\nTry again: 1\nExit: 2\n\nYour choose:");
                    int choose;
                    scanf("%d", &choose);
                    if (choose == 2) {
                        return Exit;
                    }
                    printf("Enter your login:");
                    scanned = scanf("%s", login);
                }
                login[strcspn(login, "\n")] = '\0';
                FILE* file_logins_and_passwords = fopen("logins_and_passwords.txt", "r");
                if (file_logins_and_passwords == NULL) {
                    fprintf(stderr, "Error: open the file\n");
                    return Open_the_file;
                }
                char tek_login[7], tek_password[256];
                int tek_count_of_operations;
                while(fscanf(file_logins_and_passwords, "%s %s %d", tek_login, tek_password, &tek_count_of_operations) == 3) {
                    if (!strcmp(tek_login, login)) {

                        char password[7];
                        while(1) {
                            printf("Enter password:");
                            scanned = scanf("%s", password);
                            password[strcspn(password, "\n")] = '\0';
                            while(scanned != 1 || !check_password(password)) {
                                fprintf(stderr, "Error: syntax of password\n\n");
                                printf("Try again: 1\nExit: 2\n\nYour choose:");
                                int choose;
                                scanf("%d", &choose);
                                if (choose == 2) {
                                    return Exit;
                                }
                                printf("Enter password:");
                                scanned = scanf("%s", password);
                                password[strcspn(password, "\n")] = '\0';
                            }
                            int compare_res = 0;

                            if (compare_passwords(password, tek_password, &compare_res))
                            {
                                perror("crypt");
                                return Exit;
                            }

                            if (compare_res == 0)
                            {
                                printf("Password is right!\n");
                                break;
                            }

                            printf("Wrong password!\n\n");
                            printf("Try again: 1\nExit: 2\n\nYour choose:");
                            int choose;
                            scanf("%d", &choose);
                            if (choose == 2) {
                                return Exit;
                            }
                        }

                        fclose(file_logins_and_passwords);
                        return tek_count_of_operations;
                    }
                }
                fclose(file_logins_and_passwords);
                fprintf(stderr, "Error: login was not found\n\n");
                printf("Try again: 1\nExit: 2\n\nYour choose:");
                int choose;
                scanf("%d", &choose);
                if (choose == 2) {
                    return Exit;
                }
            }
        }

        if (!strcmp(is_acc, "N")) {
            FILE* file_logins_and_passwords = fopen("logins_and_passwords.txt", "a+");
            if (file_logins_and_passwords == NULL) {
                printf("Error: file logins_and_passwords.txt open\n");
                return Open_the_file;
            }
            char login[7];
            while(1) {
                printf("\nCreate a login:");
                scanned = scanf("%s", login);
                login[strcspn(login, "\n")] = '\0';
                while(scanned > 1 || !check_login(login)) {
                    fprintf(stderr, "Error: syntax of login\n");
                    printf("\nTry again: 1\nExit: 2\n\nYour choose:");
                    int choose;
                    scanf("%d", &choose);
                    if (choose == 2) {
                        fclose(file_logins_and_passwords);
                        return Exit;
                    }
                    printf("\nCreate a login:");
                    scanned = scanf("%s", login);
                    login[strcspn(login, "\n")] = '\0';
                }
                login[strcspn(login, "\n")] = '\0';
                char tek_login[7];
                int fl_login_exist = 0;
                while(fgets(tek_login, sizeof(tek_login), file_logins_and_passwords)) {
                    tek_login[strcspn(login, " ")] = '\0';
                    if (!strcmp(login, tek_login)) {
                        printf("Error: Such a login already exists\n");
                        fl_login_exist = 1;
                        break;
                    }
                }
                if (fl_login_exist == 0) {
                    break;
                }
                printf("\nTry again: 1\nExit: 2\n\nYour choose:");
                int choose;
                scanf("%d", &choose);
                if (choose == 2) {
                    fclose(file_logins_and_passwords);
                    return Exit;
                }
            }

            char password[7];
            printf("\nCreate a password:");
            scanned = scanf("%s", password);
            while (scanned > 1 || !check_password(password)) {
                fprintf(stderr, "Error: syntax of password\n");
                printf("\nTry again: 1\nExit: 2\n\nYour choose:");
                int choose;
                scanf("%d", &choose);
                if (choose == 2) {
                    fclose(file_logins_and_passwords);
                    return Exit;
                }
                printf("\nCreate a password:");
                scanned = scanf("%s", password);
            }
            char *hashed_password;
            encrypt_password(password, &hashed_password);
            fprintf(file_logins_and_passwords, "%s %s -1\n", login, hashed_password);
            free(hashed_password);
            fclose(file_logins_and_passwords);
            return SUCCESS;
        }

        if (!strcmp(is_acc, "Exit")) {
            return Exit;
        }

        fprintf(stderr, "Error: answer must be (Y/N/Exit)\n");
        printf("\nTry again: 1\nExit: 2\n\nYour choose:");
        int choose;
        scanf("%d", &choose);
        if (choose == 2) {
            return Exit;
        }
    }
}

int imposeSanctions(char need_login[7], int count_of_operations) {
    FILE *file = fopen("logins_and_passwords.txt", "r");
    FILE *temp = fopen("temp.txt", "w");
    if (file == NULL || temp == NULL) {
        fprintf(stderr, "Error: open the file\n");
        return Open_the_file;
    }

    char login[7], password[256];
    int number, fl_no_such_login = 0;

    while (fscanf(file, "%s %s %d", login, password, &number) == 3) {
        if (!strcmp(need_login, login)) {
            number = count_of_operations;
            fl_no_such_login = 1;
        }
        fprintf(temp, "%s %s %d\n", login, password, number);
    }
    fclose(file);
    fclose(temp);

    remove("logins_and_passwords.txt");
    rename("temp.txt", "logins_and_passwords.txt");
    if (fl_no_such_login == 0) {
        fprintf(stderr, "Error: No such login\n");
        return No_such_login;
    }
    return SUCCESS;
}

int perform_an_operation() {
    printf("\nChoose one operation:\nTime\nDate\nHowmuch <time> flag\nLogout\nSanctions username <number>\n");
    char operation[28];
    printf("\nYour choose:");
    int scanned = scanf("%s", operation);
    if (scanned > 28) {
        fprintf(stderr, "Error: Syntax of operation\n");
        return Syntax_of_operation;
    }
    time_t mytime = time(NULL);
    struct tm *now = localtime(&mytime);
    if (!strncmp(operation, "Time", 4)) {
        printf("Time: %d:%d:%d\n", now->tm_hour, now->tm_min, now->tm_sec);
        return SUCCESS;
    }
    if (!strncmp(operation, "Date", 4)) {
        printf("Date: %d.%d.%d\n", now->tm_mday, now->tm_mon + 1, now->tm_year + 1900);
        return SUCCESS;
    }
    if (!strncmp(operation, "Howmuch", 7)) {
        char flag[3], input_date[11];
        scanned = scanf("%10s %2s", input_date, flag);
        printf("%s %s\n", input_date, flag);
        if (scanned != 2) {
            fprintf(stderr, "Error: Syntax of operation\n");
            return Syntax_of_operation;
        }
        struct tm input_date_tm = {0};
        if (my_strptime(input_date, "%d.%m.%Y", &input_date_tm) == 0) {
            fprintf(stderr, "Error: Syntax of operation\n");
            return Syntax_of_operation;
        }
        time_t input_date_time_t = mktime(&input_date_tm);
        double diff = difftime(mytime, input_date_time_t);
        if (!strcmp(flag, "-s")) {
            printf("%.0f seconds\n", diff);
            return SUCCESS;
        }
        if (!strcmp(flag, "-m")) {
            printf("%.0f minute\n", diff / 60);
            return SUCCESS;
        }
        if (!strcmp(flag, "-h")) {
            printf("%.0f hours\n", diff / 3600);
            return SUCCESS;
        }
        if (!strcmp(flag, "-y")) {
            printf("%.0f years\n", diff / 31536000);
            return SUCCESS;
        }
    }
    if (!strncmp(operation, "Logout", 6)) {
        return Logout;
    }
    if (!strncmp(operation, "Sanctions", 9)) {
        char username[7];
        int count_of_operations;
        scanned = scanf("%s %d", username, &count_of_operations);
        if (scanned != 2) {
            fprintf(stderr, "Error: Syntax of operation\n");
            return Syntax_of_operation;
        }
        while(1) {
            char entered_password[6];
            printf("Enter password for check:");
            if (scanf("%s", entered_password) != 1) {
                fprintf(stderr, "Error: syntax of password\n");
                printf("\nTry again: 1\nLogout: 2\nExit: 3\n");
                printf("\nYour choose:");
                int choose;
                scanf("%d", &choose);
                if(choose == 1) {
                    continue;
                }
                if (choose == 2) {
                    return Logout;
                }
                if (choose == 3) {
                    return Syntax_of_password;
                }
            }
            entered_password[strcspn(entered_password, "\n")] = '\0';
            if (!strcmp(entered_password, "12345")) {
                printf("Password is valid\n");
                break;
            }
            printf("Password is not valid\n");
            printf("\nTry again: 1\nLogout: 2\nExit: 3\n");
            printf("\nYour choose:");
            int choose;
            scanf("%d", &choose);
            if(choose == 1) {
                continue;
            }
            if (choose == 2) {
                return Logout;
            }
            if (choose == 3) {
                return Exit;
            }
        }
        int result = imposeSanctions(username, count_of_operations);
        if (result == SUCCESS) {
            return SUCCESS;
        }
        if (result == Open_the_file) {
            return Open_the_file;
        }
        return Syntax_of_operation;
    }
    fprintf(stderr, "Error: There is no operation %s\n", operation);
    return Syntax_of_operation;
}

int Try_again(int Code) {
    printf("\nTry again: 1\nLogout: 2\nExit: 3\n");
    printf("\nYour choose:");
    int choose;
    scanf("%d", &choose);
    if (choose == 1) {
        if (Code == Syntax_of_operation) {
            return Syntax_of_operation;
        }
        if (Code == Open_the_file) {
            return Exit;
        }
    }
    if (choose == 2) {
        return Logout;
    }
    if (choose == 3) {
        return Exit;
    }
    printf("There is no such option\n");
    return There_is_no_such_option;
}

int main()
{
    while(1) {
        int count_of_operations = 0, max_count_of_operations = menu();
        if (max_count_of_operations == Open_the_file){
            return 0;
        }
        if (max_count_of_operations == Exit) {
            return 0;
        }
        while(1) {
            int result = perform_an_operation();
            count_of_operations += 1;
            if (result == Logout) {
                break;
            }
            if (result == Exit) {
                return 0;
            }
            if (result != SUCCESS) {
                int result_try_again;
                while(1) {
                    result_try_again = Try_again(result);
                    if (result_try_again != There_is_no_such_option) {
                        break;
                    }
                }
                if (result_try_again == Syntax_of_operation) {
                    continue;
                }
                if (result_try_again == Logout) {
                    break;
                }
                if (result_try_again == Exit) {
                    return 0;
                }
            }
            if (count_of_operations >= max_count_of_operations && max_count_of_operations != -1) {
                printf("Count of possible operations has been achieved\n");
                return 1;
            }
        }
    }
}