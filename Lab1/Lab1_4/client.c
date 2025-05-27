#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <pthread.h>
#include <ctype.h>

#define MESSAGE_SIZE 1024
#define STRINGIZE(x) #x
#define MS_STR(x) STRINGIZE(x)
#define structSiz(strct) (sizeof(strct.messageText) + sizeof(strct.from))


struct message
{
    long mtype;
    char messageText[MESSAGE_SIZE];
    int from;
};

enum messageTypes{
    TO_SERVER = 1,
    TO_CLIENT = 2
};


static int msgId;
static pid_t pid;

void* messageRcv(void* arg){
    (void)arg;
    while (1)
    {
        struct message reply;
        if (msgrcv(msgId, &reply, structSiz(reply), pid, 0) == -1) {
            perror("msgrcv");
            exit(1);
        }
        printf("Server answer: %s", reply.messageText);
    }
    return NULL;
}

int main(int argc, char const *argv[])
{
    (void)argc;
    (void)argv;
    key_t key = ftok("/tmp/river_crossing.key", 'A');

    pid = getpid();

    if(key == -1){
        printf("Ftok error!\n");
        return -1;
    }
    msgId = msgget(key, 0700);
    if(msgId == -1){
        printf("Message error!\n");
        return -1;
    }

    pthread_t rcvThread;
    if(pthread_create(&rcvThread, NULL, messageRcv, NULL) != 0){
        printf("Thread error\n");
        return -1;
    }

    if(argc > 1){
        FILE* f = fopen(argv[1], "r");
        if(!f){
            printf("File error!\n");
            //goto nofile;
            return -1;
        }
        char c = (char)fgetc(f);
        size_t len = 0;
        char buf[MESSAGE_SIZE];
        do
        {
            if(iscntrl(c)){
                buf[len] = '\0';
                len = 0;
                struct message msg;
                msg.from = (int)pid;
                msg.mtype = TO_SERVER;
                printf("%s\n", msg.messageText);
                sleep(1);
                strcpy(msg.messageText, buf);
                if (msgsnd(msgId, &msg, structSiz(msg), 0) == -1) {
                    perror("msgsnd");
                    pthread_cancel(rcvThread);
                    return -1;
                }
            }
            else{
                buf[len++] = c;
            }
        } while ((c = (char)fgetc(f)) != EOF);
        if(iscntrl(c)){
            buf[len] = '\0';
            len = 0;
            struct message msg;
            msg.from = (int)pid;
            msg.mtype = TO_SERVER;
            strcpy(msg.messageText, buf);
            if (msgsnd(msgId, &msg, structSiz(msg), 0) == -1) {
                perror("msgsnd");
                pthread_cancel(rcvThread);
                return -1;
            }
        }
    }

//IF NO FILE:
    while(1){
        char buf[MESSAGE_SIZE];
        struct message msg;
        msg.mtype = TO_SERVER;
        msg.from = (int)pid;
        char c;
        int ii = 0;

        fflush(NULL);
        while((c = (char)getchar()) != '\n'){
            buf[ii++] = c;
            buf[ii] = '\0';
        }

        if(!strcmp(buf, "exit")){
            pthread_cancel(rcvThread);
            return 0;
        }
        strcpy(msg.messageText, buf);

        if (msgsnd(msgId, &msg, structSiz(msg), 0) == -1) {
            perror("msgsnd");
            pthread_cancel(rcvThread);
            return -1;
        }



    }
    pthread_cancel(rcvThread);
    return 0;
}