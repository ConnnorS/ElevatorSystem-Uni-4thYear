typedef struct
{
  int fd; // file descriptor of the client
  char name[64]; // the car's name when it is initialised

  pthread_mutex_t mutex; // mutex for this client specifically
  pthread_cond_t cond;   // condition to indicate some value has been updated

  char current_floor[4];     // the current floor of the car
  char destination_floor[4]; // the destination floor of the car

  char lowest_floor[4];  // the lowest serviceable floor of the car
  char highest_floor[4]; // the highest serviceable floor of the car
} client_t;

int create_server();
void handle_received_car_message(client_t *client, char *message);