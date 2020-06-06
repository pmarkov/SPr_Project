#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

#include "communication.h"
#include "client.h"


int main() {
    int socket_fd = initialize_client_socket();

    if (socket_fd == ERR_CODE) {
        exit(EXIT_FAILURE);
    }

    communicate_with_server(socket_fd);

    close(socket_fd);
}

int initialize_client_socket() {
    int socket_fd;
    struct sockaddr_in server_addr;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        printf("Error initializng client socket: %s\n", strerror(errno));
        return ERR_CODE;
    } else {
        printf("Socket successfully created!\n");
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    if (connect(socket_fd, (SA*)&server_addr, sizeof(server_addr)) != 0) {
        printf("Failed connecting to server: %s\n", strerror(errno));
        close(socket_fd);
        return ERR_CODE;
    } else {
        printf("Connected to the server!\n");
    }
    return socket_fd;
}

void communicate_with_server(int socket_fd) {
    message received_msg = {0};
    message response_msg = {0};
    size_t bytes_read, bytes_sent;

    while (1) {
        bzero(&received_msg, sizeof(received_msg));
        bzero(&response_msg, sizeof(response_msg));
        bytes_read = read(socket_fd, &received_msg, sizeof(received_msg));

        if (bytes_read <= 0) {
            printf("Could not read information from server: %s\n", strerror(errno));
            break;
        }

        printf("%s", received_msg.msg_buf);
        if (!received_msg.response_required) {
            printf("Session ended!\n");
            break;
        }

        scanf("%s", response_msg.msg_buf);
        response_msg.response_required = 1;
        bytes_sent = write(socket_fd, &response_msg, sizeof(response_msg)); 
        if (bytes_sent <= 0) {
            printf("[Could not write to server: %s\n", strerror(errno));
            break;
        }
    }
}