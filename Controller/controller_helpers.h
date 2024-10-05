#define IS_CAR 1
#define IS_CALL 0

typedef struct
{
  int fd;        // file descriptor of the client
  char name[64]; // the car's name when it is initialised
  int type;      // 1 for car, 0 for call pad

  pthread_mutex_t mutex; // mutex for this client specifically
  pthread_cond_t cond;   // condition to indicate some value has been updated

  char status[8];
  char current_floor[4];     // the current floor of the car
  char destination_floor[4]; // the destination floor of the car

  char lowest_floor[4];  // the lowest serviceable floor of the car
  char highest_floor[4]; // the highest serviceable floor of the car
} client_t;

/* this will support an array of the connected clients
purely for the call_pad_manager thread to loop through */
typedef struct
{
  int fd;
  char name[64];
  char lowest_floor[4];  // the lowest serviceable floor of the car
  char highest_floor[4]; // the highest serviceable floor of the car
} client_floors;

/* headers */
int create_server();
void handle_received_car_message(client_t *client, char *message);
void handle_received_status_message(client_t *client, char *message);
void initialise_mutex_cond(client_t *client);
void add_to_client_floors_list(client_t *client, client_floors *client_floors_list, int *length);