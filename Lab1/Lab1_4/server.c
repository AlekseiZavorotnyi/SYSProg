#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <pthread.h>
#include <sys/ipc.h>

#define MESSAGE_SIZE 1024

#define LEFT 0
#define RIGHT 1

#define structSiz(strct) (sizeof(strct.messageText) + sizeof(strct.from))

static int msgId;

struct message
{
    long mtype;
    char messageText[MESSAGE_SIZE];
    int from;
};

typedef enum {
    WOLF = 1,
    GOAT = 2,
    CABBAGE = 4
}Items;

enum messageTypes{
    TO_SERVER = 1,
    TO_CLIENT = 2
};

enum returnType{
    EXIT,
    SUCCESS,
    ALREADY_EXISTS,
    MEMORY_ERROR,
    NO_SUCH_ITEM,
    THE_BUFFER_IS_EMPTY,
    THE_BUFFER_IS_FULL,
    GOAT_ATE_CABBAGE,
    WOLF_ATE_GOAT,
    WIN,
    MESSAGE_ERROR
};

typedef struct{
    unsigned char l, r, curPosition;
    Items buffer;
}Game;

typedef int (*callback)(Game*, struct message*, unsigned char);

int take(Game* game, struct message* msg, unsigned char arg) {
    (void)msg;
    if(game->buffer != 0) return THE_BUFFER_IS_FULL;
    if(game->curPosition == LEFT) {
        if((game->l & arg) == 0) return NO_SUCH_ITEM;
        game->buffer = arg;
        game->l = game->l ^ arg;
    } else {
        if((game->r & arg) == 0) return NO_SUCH_ITEM;
        game->buffer = arg;
        game->r = game->r ^ arg;
    }
    return SUCCESS;
}

int put(Game* game, struct message* msg, unsigned char arg){
    (void)msg;(void)arg;
    if(!(game->buffer | 0)){
        return THE_BUFFER_IS_EMPTY;
    }
    if(game->curPosition == LEFT){
        game->l |= game->buffer;
        game->buffer &= 0;
        if(game->l == (WOLF | CABBAGE | GOAT)){
            game->l = 0;
            game->r = WOLF | CABBAGE | GOAT;
            game->curPosition = RIGHT;
            game->buffer = 0;
            return WIN;
        }
    } else{
        game->r |= game->buffer;
        game->buffer &= 0;
    }
    return SUCCESS;
}

int move(Game* game, struct message* msg, unsigned char arg){
    (void)arg;(void)msg;
    if(game->curPosition == LEFT){
        switch (game->l)
        {
            case CABBAGE | GOAT:
                game->l = 0;
                game->r = WOLF | CABBAGE | GOAT;
                game->curPosition = RIGHT;
                game->buffer = 0;
                return GOAT_ATE_CABBAGE;
                break;
            case WOLF | GOAT:
                game->l = 0;
                game->r = WOLF | CABBAGE | GOAT;
                game->curPosition = RIGHT;
                game->buffer = 0;
                return WOLF_ATE_GOAT;
                break;
            default:
                break;
        }
        game->curPosition = RIGHT;
    }else{
        switch (game->r)
        {
            case CABBAGE | GOAT:
                game->l = 0;
                game->r = WOLF | CABBAGE | GOAT;
                game->curPosition = RIGHT;
                game->buffer = 0;
                return GOAT_ATE_CABBAGE;
                break;
            case WOLF | GOAT:
                game->l = 0;
                game->r = WOLF | CABBAGE | GOAT;
                game->curPosition = RIGHT;
                game->buffer = 0;
                return WOLF_ATE_GOAT;
                break;
            default:
                break;
        }
        game->curPosition = LEFT;
    }
    return SUCCESS;
}

int move_new(Game* game, struct message* msg, unsigned char arg){
    (void)arg;(void)msg;
    if(game->curPosition == LEFT){
        switch (game->l)
        {
            case CABBAGE | GOAT: return GOAT_ATE_CABBAGE;
            case WOLF | GOAT: return WOLF_ATE_GOAT;
            case WOLF | GOAT | CABBAGE: return WOLF_ATE_GOAT;
            default:
                break;
        }
        game->curPosition = RIGHT;
    }else{
        switch (game->r)
        {
            case CABBAGE | GOAT: return GOAT_ATE_CABBAGE;
            case WOLF | GOAT: return WOLF_ATE_GOAT;
            case WOLF | GOAT | CABBAGE: return WOLF_ATE_GOAT;
            default:
                break;
        }
        game->curPosition = LEFT;
    }
    return SUCCESS;
}

int help(Game* game, struct message* msg, unsigned char arg){
    (void)arg;(void)game;(void)msg;
    struct message hlp;
    hlp.mtype = msg->from;
    strcpy(hlp.messageText, "\n\nAll commands:\n"
    "1.take <item> - to take one of items (cabbage, wolf or goat)\n"
    "2.put; - to put an item to the nearest coast\n"
    "3.move; - to move to another coast\nhelp; - to see all commands\n"
    "4.exit\n\n");
    if(msgsnd(msgId, &hlp, structSiz(hlp), 0) == -1){
        perror("msgsnd");
        return MESSAGE_ERROR;
    }
    return SUCCESS;
}

int curpos(Game* game, struct message* msg, unsigned char arg){
    (void)game;(void)msg;(void)arg;
    struct message hlp;
    hlp.mtype = msg->from;
    hlp.from = TO_SERVER;

    strcat(hlp.messageText, "LEFT SIDE: ");
    if((game->l & WOLF) == WOLF){
        strcat(hlp.messageText, "wolf ");
    }
    if((game->l & GOAT) == GOAT){
        strcat(hlp.messageText, "goat ");
    }
    if((game->l & CABBAGE) == CABBAGE){
        strcat(hlp.messageText, "cabbage ");
    }
    strcat(hlp.messageText, "\nRIGHT SIDE: ");
    if((game->r & WOLF) == WOLF){
        strcat(hlp.messageText, "wolf ");
    }
    if((game->r & GOAT) == GOAT){
        strcat(hlp.messageText, "goat ");
    }
    if((game->r & CABBAGE) == CABBAGE){
        strcat(hlp.messageText, "cabbage ");
    }
    strcat(hlp.messageText, "\nThe boat is on the ");
    game->curPosition == RIGHT? strcat(hlp.messageText, "right side."): strcat(hlp.messageText, "left side.");
    switch (game->buffer)
    {
    case WOLF:
        strcat(hlp.messageText, "\nWolf is on the boat.\n");
        break;
    case CABBAGE:
        strcat(hlp.messageText, "\nCabbage is on the boat.\n");
        break;
    case GOAT:
        strcat(hlp.messageText, "\nGoat is on the boat.\n");
        break;
    default:
        strcat(hlp.messageText, "\nNobody is on the boat.\n");
        break;
    }
    if(msgsnd(msgId, &hlp, structSiz(hlp), 0) == -1){
        perror("msgsnd");
        return MESSAGE_ERROR;
    }
    return SUCCESS;
}

const char *commands[] = {"take", "put;", "move;", "help;", "curpos;"};
const char *takeArgs[] = {"wolf;", "goat;", "cabbage;"};
const callback cbs[] = {take, put, move_new, help, curpos};

int answerHandler(int answer, struct message* msg){
    (void)answer;(void)msg;
    struct message ans;
    ans.mtype = msg->from;
    ans.from = TO_SERVER;
    switch (answer)
    {
    case ALREADY_EXISTS:
        {
            strcpy(ans.messageText, "Such name already exists!\n");
            if(msgsnd(msgId, &ans, structSiz(ans), 0) == -1){
                perror("msgsnd");
                return MESSAGE_ERROR;
            }
            break;
        }
    case MEMORY_ERROR:
        {
            strcpy(ans.messageText, "Memory error!\n");
            if(msgsnd(msgId, &ans, structSiz(ans), 0) == -1){
                perror("msgsnd");
                return MESSAGE_ERROR;
            }
            break;
        }
    case NO_SUCH_ITEM:
        {
            strcpy(ans.messageText, "There is no such item!\n");
            if(msgsnd(msgId, &ans, structSiz(ans), 0) == -1){
                perror("msgsnd");
                return MESSAGE_ERROR;
            }
            break;
        }
    case THE_BUFFER_IS_EMPTY:
        {
            strcpy(ans.messageText, "The boat is empty!\n");
            if(msgsnd(msgId, &ans, structSiz(ans), 0) == -1){
                perror("msgsnd");
                return MESSAGE_ERROR;
            }
            break;
        }
    case THE_BUFFER_IS_FULL:
        {
            strcpy(ans.messageText, "The boat is full!\n");
            if(msgsnd(msgId, &ans, structSiz(ans), 0) == -1){
                perror("msgsnd");
                return MESSAGE_ERROR;
            }
            break;
        }
    case GOAT_ATE_CABBAGE:
        {
            strcpy(ans.messageText, "You should not do it, because if you do it the goat will eat the cabbage! Try again...\n");
            if(msgsnd(msgId, &ans, structSiz(ans), 0) == -1){
                perror("msgsnd");
                return MESSAGE_ERROR;
            }
            break;
        }
    case WOLF_ATE_GOAT:
        {
            strcpy(ans.messageText, "You should not do it, because if you do it the wolf will eat the goat! Try again...\n");
            if(msgsnd(msgId, &ans, structSiz(ans), 0) == -1){
                perror("msgsnd");
                return MESSAGE_ERROR;
            }
            break;
        }
    case WIN:
        {
            strcpy(ans.messageText, "Victory! The game is restarted!\n");
            if(msgsnd(msgId, &ans, structSiz(ans), 0) == -1){
                perror("msgsnd");
                return MESSAGE_ERROR;
            }
            break;
        }
    default:
        break;
    }
    return SUCCESS;
}

void* input(void* argg)
{
    (void)argg;
    Game game;
    game.l = (unsigned char)0;
    game.r = (unsigned char)WOLF | (unsigned char)GOAT | (unsigned char)CABBAGE;
    game.curPosition = RIGHT;
    game.buffer = 0;

    key_t key = ftok("/tmp/river_crossing.key", 'A');
    if(key == -1){
        perror("ftok");
        printf("Ftok error!\n");
        return NULL;
    }

    msgId = msgget(key, IPC_CREAT | 0700);
    if(msgId == -1){
        perror("msgget");
        printf("Message error!\n");
        return NULL;
    }

    while (1)
    {
        struct message msg;
        if(msgrcv(msgId, &msg, structSiz(msg), TO_SERVER, 0) == -1){
            perror("msgrcv");
            printf("msgrcv error!\n");
            msgctl(msgId, IPC_RMID, NULL);
            return NULL;
        }
        printf("%s\n", msg.messageText);

        char* ptr = strtok(msg.messageText, " \n\t");
        int cnt = 0;
        callback cb;
        unsigned char arg = 0;
        do
        {
            if(cnt == 0){

                for (size_t i = 0; i < 5; i++)
                {
                    if(!strcmp(ptr, commands[i])){
                        cb = cbs[i];
                        cnt++;
                        break;
                    }
                }
                if(!cnt){
                    struct message ans;
                    ans.mtype = msg.from;
                    strcpy(ans.messageText, "No such command! Input 'help;' to find out all commands!\n");
                    if(msgsnd(msgId, &ans, structSiz(ans), 0) == -1){
                        perror("msgsnd");
                        return NULL;
                    }
                }
            }
            else{
                for (size_t i = 0; i < 3; i++)
                {
                    if(!strcmp(ptr, takeArgs[i])){
                        arg = 1 << i;
                    }
                }
            }

        } while ((ptr = strtok(NULL, " \n\t")) != NULL);
        int ret = SUCCESS;
        if(cnt != 0){
            ret = cb(&game, &msg, arg);
        }

        ret = answerHandler(ret, &msg);

        continue;
    }


    return NULL;
}

int main(int argc, char* argv[]){
    (void)argc;(void)argv;

    pthread_t mth;

    printf("Server has been started!!!\n\n");

    if(pthread_create(&mth, NULL, input, NULL) != 0){
        printf("Thread error!\n");
        return -1;
    }

    char buf[128];
    scanf("%s", buf);
    pthread_cancel(mth);
    return 0;
}
