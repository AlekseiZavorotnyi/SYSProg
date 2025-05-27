#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <pthread.h>

#define MESSAGE_LEN PATH_MAX
#define STSIZE(st) (sizeof(st.messageText) + sizeof(st.from))
#define TO_SERVER 1
#define BLUE     "\x1b[34m"
#define BRIGHT_BLUE     "\x1b[94m"
#define END_COLOR "\x1b[0m"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

typedef enum{
    EMPTY,
    WRITING
}State;

static State state = EMPTY;
static int msgId;

struct message{
    long mtype;
    char messageText[MESSAGE_LEN];
    long from;
};

struct threadInfo{
    char pathToRead[PATH_MAX];
    pthread_t thread;
    int msgTo;
};


typedef enum{
    SUCCESS,
    MEMORY_ERROR,
    FILE_ERROR,
    DIRECTORY_ERROR,
    NULL_ERROR,
    MESSAGE_ERROR
}returnType;


void error_handler(returnType type){
    switch (type)
    {
    case MEMORY_ERROR:
        printf("Memory error occured!\n");
        break;
    case FILE_ERROR:
        printf("File error occured!\n");
        break;
    case DIRECTORY_ERROR:
        printf("Directory error!\n");
        break;
    case NULL_ERROR:
        printf("Directory error!\n");
        break;
    case MESSAGE_ERROR:
        printf("Message error!\n");
        break;
    default:
        break;
    }
}


void* threadQueue(void* arg){
    if(arg == NULL){
        return NULL;
    }
    struct threadInfo* ti = (struct threadInfo*)arg;
    while(1){
        pthread_mutex_lock(&mutex);
        while(state != EMPTY){
            pthread_cond_wait(&cond, &mutex);
        }
        pthread_mutex_unlock(&mutex);

        if(state == EMPTY){
            state = WRITING;
            DIR* dir = opendir(ti->pathToRead);
            returnType ret = SUCCESS;
            (void)ret;
            if(!dir){
                ret = DIRECTORY_ERROR;
                return /*&ret*/ NULL;
            }

            struct message upd;
            upd.mtype = ti->msgTo;
            upd.from = TO_SERVER;
            strcpy(upd.messageText, "\tFor directory: \0");
            strcat(upd.messageText, ti->pathToRead);
            strcat(upd.messageText, ":");
            if(msgsnd(msgId, &upd, STSIZE(upd), 0)){
                perror("msgsnd");
                state = EMPTY;
                pthread_cond_broadcast(&cond);
                closedir(dir);
                ret = MESSAGE_ERROR;
                return /*&ret*/ NULL;
            }
            struct dirent* dEnt;
            while ((dEnt = readdir(dir)) != NULL)
            {
                struct stat st;
                char fullName[PATH_MAX];
                strcpy(fullName, "\0");
                strcat(fullName, ti->pathToRead);
                strcat(fullName, dEnt->d_name);
                if(dEnt->d_name[0] != '.' && stat(fullName, &st) != -1){
                    if(S_ISDIR(st.st_mode)){
                        strcpy(upd.messageText, "\0");
                        strcat(upd.messageText, "\t\t"BLUE);
                        strcat(upd.messageText, dEnt->d_name);
                        strcat(upd.messageText, END_COLOR);
                        strcat(upd.messageText, "\0");
                        if(msgsnd(msgId, &upd, STSIZE(upd), 0) == -1){
                            perror("msgsnd");
                            state = EMPTY;
                            pthread_cond_broadcast(&cond);
                            closedir(dir);
                            ret = MESSAGE_ERROR;
                            return /*&ret*/ NULL;
                        }
                    }
                    else if(S_ISREG(st.st_mode)){
                        strcpy(upd.messageText, "\t\t\0");
                        strcat(upd.messageText, dEnt->d_name);
                        strcat(upd.messageText, "\0");
                        if(msgsnd(msgId, &upd, STSIZE(upd), 0) == -1){
                            perror("msgsnd");
                            state = EMPTY;
                            pthread_cond_broadcast(&cond);
                            closedir(dir);
                            ret = MESSAGE_ERROR;
                            return /*&ret*/ NULL;
                        }
                    }
                    else{
                        strcpy(upd.messageText, "\0");
                        strcat(upd.messageText, "\t\t"BRIGHT_BLUE);
                        strcat(upd.messageText, dEnt->d_name);
                        strcat(upd.messageText, END_COLOR);
                        strcat(upd.messageText, "\0");
                        if(msgsnd(msgId, &upd, STSIZE(upd), 0) == -1){
                            perror("msgsnd");
                            state = EMPTY;
                            pthread_cond_broadcast(&cond);
                            closedir(dir);
                            ret = MESSAGE_ERROR;
                            return /*&ret*/ NULL;
                        }
                    }
                }
            }
            closedir(dir);
            state = EMPTY;
            pthread_cond_broadcast(&cond);
            ret = SUCCESS;
            free(ti);
            return /*&ret*/ NULL;
        }
    }
}

returnType work(struct message* msg){
    DIR* dir = opendir(msg->messageText);
    if(!dir){
        return DIRECTORY_ERROR;
    }
    state = WRITING;
    struct message upd;
    upd.mtype = msg->from;
    upd.from = TO_SERVER;
    strcpy(upd.messageText, "For directory: \0");
    strcat(upd.messageText, msg->messageText);
    if(msg->messageText[strlen(msg->messageText) - 1] != '/'){
        strcat(upd.messageText, "/\0");
    }
    if(msgsnd(msgId, &upd, STSIZE(upd), 0) == -1){
        perror("msgsnd");
        closedir(dir);
        return MESSAGE_ERROR;
    }

    size_t threadsNumber = 0;
    size_t threadInfoCap = 16;


    struct threadInfo* inf = (struct threadInfo*)malloc(sizeof(struct threadInfo) * threadInfoCap);
    if(!inf){
        closedir(dir);
        return MEMORY_ERROR;
    }

    struct dirent* dEnt;
    while ((dEnt = readdir(dir)) != NULL)
    {
        struct stat st;
        char fullName[PATH_MAX];
        strcpy(fullName, "\0");
        strcat(fullName, msg->messageText);
        strcat(fullName, dEnt->d_name);
        strcat(fullName, "/\0");
        printf("%s\n", fullName);
        if(dEnt->d_name[0] != '.'){
            if(stat(fullName, &st) != -1){
                if(S_ISDIR(st.st_mode)){
                    struct threadInfo *ti = (struct threadInfo*)malloc(sizeof(struct threadInfo));
                    if(!ti){
                        perror("Memory");
                        continue;
                    }
                    ti->msgTo = msg->from;
                    //strcat(fullName, "/\0");
                    strcpy(ti->pathToRead, fullName);

                    if(pthread_create(&ti->thread, NULL, threadQueue, (void*)ti) == -1){
                        perror("thread");
                        continue;
                    }

                    if(threadInfoCap == threadsNumber){
                        threadInfoCap *= 2;
                        struct threadInfo* buf = (struct threadInfo*)realloc((void*)inf, threadInfoCap * sizeof(struct threadInfo));
                        if(!buf){
                            for (size_t i = 0; i < threadsNumber; i++)
                            {
                                pthread_cancel(inf[i].thread);
                            }
                            free(inf);
                        }
                        inf = buf;
                    }
                    inf[threadsNumber++] = *ti;
                }
                else{
                    strcpy(upd.messageText, "\0");
                    strcat(upd.messageText, dEnt->d_name);
                    strcat(upd.messageText, "\0");
                    if(msgsnd(msgId, &upd, STSIZE(upd), 0) == -1){
                        perror("msgsnd");
                        state = EMPTY;
                        pthread_cond_broadcast(&cond);
                        closedir(dir);
                        free(inf);
                        return MESSAGE_ERROR;
                    }
                }
            }
        }



    }
    closedir(dir);

    state = EMPTY;
    pthread_cond_broadcast(&cond);

    for (size_t i = 0; i < threadsNumber; i++)
    {
        pthread_join(inf[i].thread, NULL);
    }
    free(inf);
    return SUCCESS;
}

void* input(void* arg){
    (void)arg;
    struct message msg;
    while (1)
    {
        if(msgrcv(msgId, &msg, STSIZE(msg), TO_SERVER, 0) == -1){
            perror("msgrcv");
            return NULL;
        }

        returnType ret = work(&msg);
        error_handler(ret);

    }
}

int main(int argc, char const *argv[])
{

    //переделать под отдельный поток
    (void)argv;(void)argc;
    key_t key = ftok("/home/gaalex/Programs/SysProg/SysProg/Labs/Lab1/Task6/Server/output/main", 'A');
    if(key == -1){
        perror("ftok");
        return -1;
    }

    msgId = msgget(key, IPC_CREAT | 0700);
    if(msgId == -1){
        perror("msgget");
        return -1;
    }

    pthread_t iput;
    if( pthread_create(&iput, NULL, input, NULL) == -1){
        printf("Thread error!\n");
        return -1;
    }

    /* struct message msg;
    while (1)
    {
        if(msgrcv(msgId, &msg, STSIZE(msg), TO_SERVER, 0) == -1){
            perror("msgrcv");
            return -1;
        }

        returnType ret = work(&msg);
        error_handler(ret);

    } */
    //pthread_join(iput, NULL);
    char c = getchar();
    pthread_cancel(iput);
    (void)c;
    return 0;
}