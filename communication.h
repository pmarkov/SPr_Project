#define ERR_CODE -1
#define PORT 8080
#define MAX_BUFF_SIZE 1024
#define MAX_WAITING_CONNECTIONS 5
#define MAX_USERNAME_SIZE 50
#define MAX_PASSWORD_SIZE 50


typedef struct Message {
    int response_required;
    char msg_buf[MAX_BUFF_SIZE];
} message;

typedef struct User {
    char username[MAX_USERNAME_SIZE];
    char password[MAX_PASSWORD_SIZE];
    int initial_weight;
    int last_weight;
} s_user;

typedef struct UserNode {
    s_user user;
    struct UserNode *next;
} n_user;