#define CAR_NAME_LENGTH 32

typedef struct car_status
{
  char name[CAR_NAME_LENGTH];
  char status[8];
  char current_floor[4];
  char destination_floor[4];
} car_status_info;

typedef struct
{
  char name[CAR_NAME_LENGTH];
  char lowest_floor[4];
  char highest_floor[4];
} controller_car_info;

typedef struct
{
  int fd;
  char lowest_floor[4];
  char highest_floor[4];
} client_t;

int create_server();
char *receive_message(int fd);
void handle_received_status_message(client_t *client, char *message);
void handle_received_car_message(client_t *client, char *message);
int send_floor_request(int clientFd, const char *message);