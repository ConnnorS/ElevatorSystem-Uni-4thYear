#define CAR_NAME_LENGTH 32

/* data structure which keeps track of each client's fd
and the floors they service. This will help when deciding
which car to give floors to */
typedef struct
{
  int fd;
  char name[CAR_NAME_LENGTH];
  char lowest_floor[4];
  char highest_floor[4];
} client_info;

/* structure which the handle_received_call_message() returns
so the calling function can work out which car to call */
typedef struct
{
  int source_floor;
  int destination_floor;
} call_msg_info;

int create_server();
void handle_received_status_message(client_info *client, char *message);
void handle_received_car_message(client_info *client, char *message);
void handle_received_call_message(char *message, call_msg_info *call_msg);
int find_car_for_floor(call_msg_info *call_msg, client_info *clients, int client_count, char car_name[CAR_NAME_LENGTH]);