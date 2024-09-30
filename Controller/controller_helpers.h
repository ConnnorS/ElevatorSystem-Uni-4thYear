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

/* data structure which keeps track of each client's fd
and the floors they service. This will help when deciding
which car to give floors to */
typedef struct
{
  int fd;
  char lowest_floor[4];
  char highest_floor[4];
} client_t;

/* structure which the handle_received_call_message() returns
so the calling function can work out which car to call */
typedef struct
{
  int source_floor;
  int destination_floor;
} call_msg_info;

int create_server();
void handle_received_status_message(client_t *client, char *message);
void handle_received_car_message(client_t *client, char *message);
void handle_received_call_message(char *message, call_msg_info *call_msg);
void find_car_for_floor(call_msg_info *call_msg, client_t *clients, int client_count);