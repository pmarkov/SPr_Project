#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

#include "communication.h"
#include "fitness_device_constants.h"

#define SA struct sockaddr
#define USERS_FILE "users.dat"
#define WELCOM_TO_FITNESS_MSG "\n\nWelcome to fitness Keep Yourself Positive\n"

typedef struct ConnectionArgs {
    int fd;
} connection_args;

char *fitness_device_types[FITNESS_DEVICES_COUNT];

int initialize_server_socket();
void accept_connections();
void *communicate_with_client(void *args);
void write_to_client(int connection_fd, char *msg_str, int response_required, char *response_buf);

int user_login(int connection_fd, s_user *user);
s_user *find_user(char *username, char *password);

void init_menu(int conn_fd, s_user user);
void append_options(char *options_str, int *exit_code);
int handle_fitness_device(int conn_fd, int option, char *username);
void append_user_training_data(char *username, char *data);

void init_fitness_device_types();
int load_users_from_file();
void add_user_to_list(s_user user);
int load_fitness_devices_from_file();
void add_fitness_device_to_list(s_fitness_device device);
void ask_for_the_current_weight(int conn_fd, char *username);
void update_users_file();
struct tm *getCurrentTime();

void sigintHandler(int signal);

// FOR TEST DATA:
void write_test_users_to_file();
void write_fitness_devices_to_file();

// FOR DEBUG:
void print_user_list();
void print_device_list();


pthread_mutex_t lock;
int server_socket = 0;

n_user *users_list = NULL;
n_fitness_device *fitness_dev_list = NULL;

int main(int argc, char const *argv[]) {
    signal(SIGINT, sigintHandler);
    // write_test_users_to_file();
    // write_fitness_devices_to_file();
    if (!load_users_from_file()) {
        exit(EXIT_FAILURE);
    }
    if (!load_fitness_devices_from_file()) {
        exit(EXIT_FAILURE);
    }
    init_fitness_device_types();

    server_socket = initialize_server_socket();
    if (server_socket == ERR_CODE) {
        exit(EXIT_FAILURE);
    }
    accept_connections();
    return 0;
}

void sigintHandler(int sig) {
    printf("\nServer stopped by Ctrl-C\n");
    printf("Closing server socket %d...\n", server_socket);
    if (shutdown(server_socket, 2) != 0) {
        printf("Could not close server socket: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    exit(0);
}

int initialize_server_socket() {
    int socket_fd, connection_fd, client_add_len;
    struct sockaddr_in server_addr;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        printf("Error initializng server socket: %s\n", strerror(errno));
        return ERR_CODE;
    } else {
        printf("Socket successfully created!\n");
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(socket_fd, (SA*)&server_addr, sizeof(server_addr)) != 0) {
        printf("Error connecting to socket: %s\n", strerror(errno));
        return ERR_CODE;
    } else {
        printf("Socket successfully binded!\n");
    }

    if (listen(socket_fd, MAX_WAITING_CONNECTIONS) != 0) {
        printf("Listen failed: %s\n", strerror(errno));
        return ERR_CODE;
    } else {
        printf("Server listening!\n");
    }
    return socket_fd;
}

void accept_connections() {
    int connection_fd, client_add_len;
    struct sockaddr_in client_addr;

    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("mutex init failed\n");
        return;
    }

    while (1) {
        printf("server socket=%d\n", server_socket);
        connection_fd = accept(server_socket, (SA*)&client_addr, &client_add_len);
        if (connection_fd < 0) {
            printf("Server accept failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        pthread_t worker;
        connection_args args = {0};
        args.fd = connection_fd;
        pthread_create(&worker, NULL, &communicate_with_client, (void*)&args);
    }
}

void *communicate_with_client(void *args) {
    connection_args *curr_args = args;
    int conn_fd = curr_args->fd;

    s_user user = {0};
    if (!user_login(conn_fd, &user)) {
        write_to_client(conn_fd, "Login was not successful!\n", 0, NULL);
        printf("Login unsuccsessful for connection: %d!\n", conn_fd);
        printf("[DEBUG] closing connection_fd: %d\n", conn_fd);
        close(conn_fd);
        return NULL;
    }
    printf("[DEBUG] successful login for user:%s\n", user.username);
    init_menu(conn_fd, user);

    printf("[DEBUG] closing connection_fd: %d\n", conn_fd);
    close(conn_fd);
}

int user_login(int connection_fd, s_user *user) {
    printf("[DEBUG] in user_login connection_fd = %d\n", connection_fd);
    int rc = 0;
    char username[MAX_USERNAME_SIZE] = {0};
    char password[MAX_PASSWORD_SIZE] = {0};
    write_to_client(connection_fd, "Enter your username: ", 1, username);
    if (username == NULL || strlen(username) == 0) {
        return 0;
    }
    write_to_client(connection_fd, "Enter your password: ", 1, password);
    if (password == NULL || strlen(password) == 0) {
        return 0;
    }
    printf("[DEBUG] got username=%s", username);
    printf(" and password=%s\n", password);

    pthread_mutex_lock(&lock);
    s_user *result = find_user(username, password);
    if (result != NULL) {
        *user = *result;
        rc = 1;
    }
    pthread_mutex_unlock(&lock);

    return rc;
}

s_user *find_user(char *username, char *password) {
    s_user *result = NULL;

    if (username == NULL || password == NULL) {
        printf("[DEBUG] username or pass is null\n");
        return result;
    }
    n_user *curr_user = users_list;
    // print_user_list();
    while (curr_user != NULL) {
        if (strcmp(curr_user->user.username, username) == 0 &&
            strcmp(curr_user->user.password, password) == 0) {
                printf("[DEBUG] found the user\n");
                result = &curr_user->user;
                break;
            }
        curr_user = curr_user->next;
    }
    return result;
}

void init_menu(int conn_fd, s_user user) {
    char options_str[MAX_BUFF_SIZE] = {0};
    char response[MAX_BUFF_SIZE] = {0};
    int exit_code, send_goodbye_msg = 0;
    strcat(options_str, WELCOM_TO_FITNESS_MSG);
    while(1) {
        append_options(options_str, &exit_code);
        write_to_client(conn_fd, options_str, 1, response);
        long option = strtol(response, NULL, 10);
        if (option < 0 || option > exit_code) {
            write_to_client(conn_fd, "Invalid option!\n", 0, NULL);
            return;
        }
        if (option == exit_code) {
            send_goodbye_msg = 1;
            break;
        }
        int rc = handle_fitness_device(conn_fd, option, user.username);
        if (rc == -1) {
            printf("Error occured with user: %s\n", user.username);
            break;
        } else if (rc == 0) {
            send_goodbye_msg = 1;
            break;
        }
        bzero(response, sizeof(response));
        bzero(options_str, sizeof(options_str));
    }
    bzero(response, sizeof(response));
    bzero(options_str, sizeof(options_str));
    if (send_goodbye_msg) {
        ask_for_the_current_weight(conn_fd, user.username);
    }
}

void append_options(char *options_str, int *exit_code) {
    strcat(options_str, "\nChoose your torture device:\n");
    char curr_opt[MAX_BUFF_SIZE] = {0};
    int i;
    for (i = 0; i < FITNESS_DEVICES_COUNT; i++) {
        sprintf(curr_opt, "   %d. %s\n", i, fitness_device_types[i]);
        strcat(options_str, curr_opt);
    }
    *exit_code = i;
    sprintf(curr_opt, "   %d. Leave and get some rest!\nEnter your choice: ", i);
    strcat(options_str, curr_opt);
    bzero(curr_opt, sizeof(curr_opt));
}

int handle_fitness_device(int conn_fd, int option, char *username) {
    printf("[DEBUG] in handle_fitness_device for %s\n", username);
    n_fitness_device *curr_node = fitness_dev_list;
    int found = 0;
    pthread_mutex_lock(&lock);
    while (curr_node != NULL) {
        if (curr_node->device.type == option &&
            !curr_node->device.is_currently_used) {
                curr_node->device.is_currently_used = 1;
                found = 1;
                break;
            }
        curr_node = curr_node->next;
    }
    pthread_mutex_unlock(&lock);
    char response[MAX_BUFF_SIZE] = {0};
    int rc = 0;
    if (found) {
        write_to_client(conn_fd, "How much time would you like to train: ", 1, response);
        long time = strtol(response, NULL, 10);
        bzero(response, sizeof(response));
        if (time < 0 || time > MAX_TIME_FOR_REP) {
            write_to_client(conn_fd, "Invalid time!", 0, NULL);
            curr_node->device.is_currently_used = 0;
            return -1;
        }
        sleep(time);
        char training_data_str[MAX_BUFF_SIZE];
        pthread_mutex_lock(&lock);
        // struct tm *timeinfo = getCurrentTime();
        sprintf(training_data_str, "[INFO] Did 1 rep on the %s with id - %d, for %d seconds.\n",
                                fitness_device_types[option], curr_node->device.id, time);
        append_user_training_data(username, training_data_str);
        pthread_mutex_unlock(&lock);
        int reps_count = 1;
        while (reps_count <= MAX_REPS_FOR_DEVICE) {
            write_to_client(conn_fd, "Do you want to go for another rep? ", 1, response);
            if (strcmp(response, "yes") == 0 || strcmp(response, "1") == 0) {
                sleep(time);
                pthread_mutex_lock(&lock);
                append_user_training_data(username, training_data_str);
                pthread_mutex_unlock(&lock);
                reps_count++;
            } else {
                break;
            }
        }
        bzero(training_data_str, sizeof(training_data_str));
        bzero(response, sizeof(response));
        curr_node->device.is_currently_used = 0;
        rc = 1;
    } else {
        write_to_client(conn_fd, "There are no available devices of this type.\nDo you want to continue? ", 1, response);
        if (strcmp(response, "yes") == 0 || strcmp(response, "1") == 0) {
            rc = 1;
        } else {
            rc = 0;
        }
    }
    return rc;
}

void append_user_training_data(char *username, char *data) {
    char *filename;
    sprintf(filename, "%s.dat", username);
    int fd = open(filename, O_CREAT | O_WRONLY | O_APPEND, 0600);
    if (fd <= 0) {
        printf("Could not open '%s': %s\n", filename, strerror(errno));
        return;
    }
    int written_bytes = write(fd, data, strlen(data));
    if (written_bytes <= 0) {
        printf("Could not write data to '%s': %s\n", filename, strerror(errno));
    }
    close(fd);
}

void ask_for_the_current_weight(int conn_fd, char *username) {
    n_user *curr_node = users_list;
    while(curr_node != NULL) {
        if (strcmp(curr_node->user.username, username) == 0) {
            break;
        }
        curr_node = curr_node->next;
    }
    char response[MAX_BUFF_SIZE] = {0};
    char info_for_the_weight[MAX_BUFF_SIZE] = {0};
    write_to_client(conn_fd, "Don't forget to check your weight: ", 1, response);
    long curr_weight = strtol(response, NULL, 10);
    if (curr_weight < 0) {
        strcat(info_for_the_weight, "\nCome on, you couldn't be training so hard...\n");
    } else {
        if (curr_weight > curr_node->user.last_weight) {
            sprintf(info_for_the_weight, "\nYou've gained some weight since the last time!\nLast weight was: %d\nInitial weight: %d\n",
                                                                curr_node->user.last_weight, curr_node->user.initial_weight);
        } else {
            sprintf(info_for_the_weight, "\nYour current state is better or same!\nLast weight was: %d\nInitial weight: %d\n",
                                                                curr_node->user.last_weight, curr_node->user.initial_weight);
        }
        curr_node->user.last_weight = curr_weight;
        // print_user_list();
        pthread_mutex_lock(&lock);
        update_users_file();
        pthread_mutex_unlock(&lock);
    }
    strcat(info_for_the_weight, "\nSee you later, alligator!\n");
    write_to_client(conn_fd, info_for_the_weight, 0, NULL);
}

void write_to_client(int connection_fd, char *msg_str, int response_required, char *response_buf) {
    printf("[DEBUG] in write_to_client for %d\n", connection_fd);

    message msg = {0};
    strncpy(msg.msg_buf, msg_str, MAX_BUFF_SIZE);
    msg.response_required = response_required;

    pthread_mutex_lock(&lock);
    size_t sent_bytes = write(connection_fd, &msg, sizeof(msg));
    pthread_mutex_unlock(&lock);
    if (sent_bytes < 0) {
        printf("Could not write to client...\n");
        return;
    }

    if (response_required) {
        bzero(&msg, sizeof(msg));
        size_t bytes_read = read(connection_fd, &msg, sizeof(msg));
        if (bytes_read > 0) {
            strcpy(response_buf, msg.msg_buf);
        } else {
            printf("Could not get response from client...\n");
        }
    }
}

int load_users_from_file() {
    int fd = open(USERS_FILE, O_RDONLY);
    if (fd <= 0) {
        printf("Could not open '%s': %s\n", USERS_FILE, strerror(errno));
        return 0;
    }
    s_user curr_user = {0};
    while ((read(fd, &curr_user, sizeof(s_user))) > 0) {
        // printf("read user from file - %s\n", curr_user.username);
        if (users_list == NULL) {
            users_list = (n_user*) malloc(sizeof(n_user));
            users_list->user = curr_user;
            users_list->next = NULL;
        } else {
            add_user_to_list(curr_user);
        }
        bzero(&curr_user, sizeof(curr_user));
    }
    close(fd);
    print_user_list();
    return 1;
}

void update_users_file() {
    int fd = open(USERS_FILE, O_CREAT | O_TRUNC | O_WRONLY, 0600);
    if (fd <= 0) {
        printf("Could not open '%s': %s\n", USERS_FILE, strerror(errno));
        return;
    }
    n_user *curr_node = users_list;
    while (curr_node != NULL) {
        s_user curr_user = curr_node->user;
        printf("[DEBUG] writing curr_node for %s with last weight: %d\n", curr_user.username, curr_user.last_weight);
        if (write(fd, &curr_user, sizeof(curr_user)) <= 0) {
            printf("Could not write user to '%s': %s\n", USERS_FILE, strerror(errno));
        }
        curr_node = curr_node->next;
    }
    close(fd);
}

void print_user_list() {
    printf("[DEBUG] in print_user_list\n");
    n_user *curr_node = users_list;
    while (curr_node != NULL) {
        printf("User - %s ", curr_node->user.username);
        printf("Password - %s\n", curr_node->user.password);
        printf("Initial weight - %d\n", curr_node->user.initial_weight);
        printf("Last weight - %d\n", curr_node->user.last_weight);
        curr_node = curr_node->next;
    }
}

void print_device_list() {
    printf("[DEBUG] in print_device_list\n");
    n_fitness_device *curr_node = fitness_dev_list;
    while (curr_node != NULL) {
        printf("ID: %d; TYPE: %d; IS_IN_USE:%d\n", curr_node->device.id,
                                                    curr_node->device.type,
                                                    curr_node->device.is_currently_used);
        curr_node = curr_node->next;
    }
}

void add_user_to_list(s_user user) {
    n_user *curr_node = users_list;
    while (curr_node->next != NULL) {
        curr_node = curr_node->next;
    }
    curr_node->next = (n_user*) malloc(sizeof(n_user));
    curr_node->next->user = user;
    curr_node->next->next = NULL;
}

int load_fitness_devices_from_file() {
    int fd = open(FITNESS_DEVICES_FILE, O_RDONLY);
    if (fd <= 0) {
        printf("Could not open '%s': %s\n", FITNESS_DEVICES_FILE, strerror(errno));
        return 0;
    }
    s_fitness_device curr_device = {0};
    while ((read(fd, &curr_device, sizeof(s_fitness_device))) > 0) {
        if (fitness_dev_list == NULL) {
            fitness_dev_list = (n_fitness_device*) malloc(sizeof(n_fitness_device));
            fitness_dev_list->device = curr_device;
            fitness_dev_list->next = NULL;
        } else {
            add_fitness_device_to_list(curr_device);
        }
        bzero(&curr_device, sizeof(curr_device));
    }
    close(fd);
    print_device_list();
    return 1;
}

void add_fitness_device_to_list(s_fitness_device device) {
    n_fitness_device *curr_node = fitness_dev_list;
    while (curr_node->next != NULL) {
        curr_node = curr_node->next;
    }
    curr_node->next = (n_fitness_device*) malloc(sizeof(n_fitness_device));
    curr_node->next->device = device;
    curr_node->next->next = NULL;
}

struct tm *getCurrentTime() {
    time_t rawtime;
    time (&rawtime);
    return localtime (&rawtime);
}

void write_test_users_to_file() {
    int fd = open(USERS_FILE, O_CREAT | O_WRONLY, 0600);
    if (fd <= 0) {
        printf("Could not open '%s': %s\n", USERS_FILE, strerror(errno));
        return;
    }

    s_user test_info[4];
    strcpy(test_info[0].username, "Pakozavur");
    strcpy(test_info[0].password, "1234");
    test_info[0].initial_weight = 90;
    test_info[0].last_weight = 89;

    strcpy(test_info[1].username, "AzSymBot");
    strcpy(test_info[1].password, "omg");
    test_info[1].initial_weight = 106;
    test_info[1].last_weight = 110;

    strcpy(test_info[2].username, "Bat");
    strcpy(test_info[2].password, "Milko");
    test_info[2].initial_weight = 54;
    test_info[2].last_weight = 53;

    strcpy(test_info[3].username, "TestUser1");
    strcpy(test_info[3].password, "pass1");
    test_info[3].initial_weight = 80;
    test_info[3].last_weight = 82;

    for (int i = 0; i < 4; i++) {
        write(fd, &test_info[i], sizeof(test_info[i]));
    }
    close(fd);
}

void write_fitness_devices_to_file() {
    int fd = open(FITNESS_DEVICES_FILE, O_CREAT | O_WRONLY, 0600);
    if (fd <= 0) {
        printf("Could not open '%s': %s\n", FITNESS_DEVICES_FILE, strerror(errno));
        return;
    }

    s_fitness_device devices[6];
    devices[0].id = 0;
    devices[0].type = TREADMILL;
    devices[0].is_currently_used = 0;

    devices[1].id = 1;
    devices[1].type = TREADMILL;
    devices[1].is_currently_used = 0;

    devices[2].id = 2;
    devices[2].type = GLADIATOR;
    devices[2].is_currently_used = 0;

    devices[3].id = 3;
    devices[3].type = HIP_PRESS;
    devices[3].is_currently_used = 0;

    devices[4].id = 4;
    devices[4].type = BENCH_PRESS;
    devices[4].is_currently_used = 0;

    devices[5].id = 5;
    devices[5].type = BICYCLE;
    devices[5].is_currently_used = 0;

    for (int i = 0; i < 6; i++) {
        write(fd, &devices[i], sizeof(devices[i]));
    }
    close(fd);
}

void init_fitness_device_types() {
    fitness_device_types[TREADMILL] = "TREADMILL";
    fitness_device_types[GLADIATOR] = "GLADIATOR";
    fitness_device_types[BENCH_PRESS] = "BENCH_PRESS";
    fitness_device_types[HIP_PRESS] = "HIP_PRESS";
    fitness_device_types[BICYCLE] = "BICYCLE";
}