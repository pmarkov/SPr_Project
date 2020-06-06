#define USERS_FILE "users.dat"
#define WELCOM_TO_FITNESS_MSG "\n\nWelcome to fitness Keep Yourself Positive\n"

typedef struct ConnectionArgs {
    int fd;
} connection_args;

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
