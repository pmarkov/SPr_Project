#define ERR_CODE -1
#define PORT 8080
#define MAX_BUFF_SIZE 1024
#define MAX_WAITING_CONNECTIONS 5
#define MAX_USERNAME_SIZE 50
#define MAX_PASSWORD_SIZE 50

#define SA struct sockaddr

typedef struct Message {
    int response_required;
    char msg_buf[MAX_BUFF_SIZE];
} message;
