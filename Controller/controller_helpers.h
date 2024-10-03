#define CAR_NAME_LENGTH 32
#define IS_CAR 1
#define IS_CALL_PAD 0

#define CAR_UP 1
#define CAR_NEUTRAL 0
#define CAR_DOWN -1

/* data structure which keeps track of each client's fd
and the floors they service. This will help when deciding
which car to give floors to */
typedef struct
{
  int fd;         // the file descriptor of the client for sending messages
  int type;       // whether it's a car or callpad
  char status[8]; // C string indicating the elevator's status
  char name[CAR_NAME_LENGTH];
  char lowest_floor[4];
  char highest_floor[4];
  int *queue;                // the array of ints for the queue
  int queue_length;          // the length of the queue
  pthread_cond_t queue_cond; // condition variable for watching the queue change
  int direction;             // -1 for down, 1 for up, 0 for empty queue
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
void handle_received_car_message(client_info *client, char *message, char *name);
void parse_received_call_message(char *message, call_msg_info *call_msg);
int find_car_for_floor(call_msg_info *call_msg, client_info *clients, int client_count, char car_name[CAR_NAME_LENGTH], int *call_direction);
void add_to_car_queue(client_info *client, call_msg_info *call_msg);
void remove_floor(client_info *client);