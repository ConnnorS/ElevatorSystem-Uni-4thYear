#include <signal.h>

#define IS_CAR 1
#define IS_CALL 0

typedef struct
{
  int fd;        // file descriptor of the client
  char name[64]; // the car's name when it is initialised
  int type;      // 1 for car, 0 for call pad

  int *queue; // the queue of floors the car is to go to
  int queue_length;
  pthread_cond_t queue_cond; // condition to indicate some value has been updated

  char status[8];
  char current_floor[4];     // the current floor of the car
  char destination_floor[4]; // the destination floor of the car

  char lowest_floor[4];  // the lowest serviceable floor of the car
  char highest_floor[4]; // the highest serviceable floor of the car

  sig_atomic_t connected; // indicate if the client is still connected
} client_t;

/* headers */
int create_server();
void handle_received_car_message(client_t *client, char *message);
void handle_received_status_message(client_t *client, char *message);
void initialise_cond(client_t *client);
void extract_call_floors(char *message, int *source_floor, int *destination_floor);
int find_car_for_floor(int *source_floor, int *destination_floor, client_t **clients, int client_count, char *chosen_car);
void remove_from_queue(client_t *client);
void remove_client(client_t *client, client_t ***clients, int *client_count);