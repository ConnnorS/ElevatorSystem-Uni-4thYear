// basic C stuff
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// threads
#include <pthread.h>
// shared memory
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
// networks
#include <arpa/inet.h>
// headers
#include "controller_helpers.h"

int floor_char_to_int(char *floor)
{
  if (floor[0] == 'B')
  {
    floor[0] = '-';
  }
  return atoi(floor);
}

void floor_int_to_char(int floor, char *floorChar)
{
  if (floor < 0)
  {
    sprintf(floorChar, "B%d", abs(floor));
  }
  else
  {
    sprintf(floorChar, "%d", floor);
  }
}

int create_server()
{
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(3000);

  /* create the file descriptor for the socket */
  int serverFd = socket(AF_INET, SOCK_STREAM, 0);
  if (serverFd == -1)
  {
    perror("socket()");
    return -1;
  }

  /* bind the socket to an actual IP and port */
  int opt_enable = 1;
  setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt_enable, sizeof(opt_enable));
  if (bind(serverFd, (const struct sockaddr *)&addr, sizeof(addr)) == -1)
  {
    perror("bind()");
    return -1;
  }

  /* now get the socket to start listening */
  if (listen(serverFd, 10) == -1)
  {
    perror("listen()");
    return -1;
  }

  return serverFd;
}

/* handles a received message that starts with "CAR", extract's
the car's data and saves it in the car's client_info data structure */
void handle_received_car_message(client_t *client, char *message)
{
  /* extract the data */
  char name[64];
  char lowest_floor[4];
  char highest_floor[4];
  sscanf(message, "%*s %31s %3s %3s", name, lowest_floor, highest_floor);

  client->type = IS_CAR;
  strcpy(client->highest_floor, highest_floor);
  strcpy(client->lowest_floor, lowest_floor);
  strcpy(client->name, name);
}

/* extract all the data from the status message, update the client object
and then signal to the queue manager that something has changed for that
car's floor */
void handle_received_status_message(client_t *client, char *message)
{
  /* extract the data */
  char status[8];
  char current_floor[4];
  char destination_floor[4];
  sscanf(message, "%*s %7s %3s %3s", status, current_floor, destination_floor);

  strcpy(client->status, status);
  strcpy(client->current_floor, current_floor);
  strcpy(client->destination_floor, destination_floor);
  pthread_cond_signal(&client->queue_cond); // signal that the floors have changed and the queue thread might need to act
}

/* extracts the call pad's floors and converts them to an integer for easier comparison */
void extract_call_floors(char *message, int *source_floor, int *destination_floor)
{
  char source[4];
  char destination[4];
  sscanf(message, "%*s %3s %3s", source, destination);

  *source_floor = floor_char_to_int(source);
  *destination_floor = floor_char_to_int(destination);
}

/* helper to initialise the condition variable for each client */
void initialise_cond(client_t *client)
{
  pthread_condattr_t cond_attr;
  pthread_condattr_init(&cond_attr);
  pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
  pthread_cond_init(&client->queue_cond, &cond_attr);
}

int floors_are_in_range(int sourceFloor, int destinationFloor, int lowestFloor, int highestFloor)
{
  return (sourceFloor >= lowestFloor && sourceFloor <= highestFloor) &&
         (destinationFloor >= lowestFloor && destinationFloor <= highestFloor);
}
/* finds the fd of the car which can service the floors then adds them to the queue */
int find_car_for_floor(int *source_floor, int *destination_floor, client_t *clients, int client_count, char *chosen_car)
{
  int found = 0;

  client_t *current;
  /* find a client which can service the request */
  for (int index = 0; index < client_count; index++)
  {
    current = &clients[index];
    int current_lowest_floor_int = floor_char_to_int(current->lowest_floor);
    int current_highest_floor_int = floor_char_to_int(current->highest_floor);
    int can_service = floors_are_in_range(*source_floor, *destination_floor, current_lowest_floor_int, current_highest_floor_int);
    /* if the client is a car and can service the range of floors */
    if (current->type == IS_CAR && can_service)
    {
      strcpy(chosen_car, current->name);
      found = 1;
      break;
    }
  }

  /* if found, add the floors to their queue */
  if (found)
  {
    current->queue_length += 2;
    current->queue = realloc(current->queue, sizeof(int) * (current->queue_length));
    current->queue[current->queue_length - 2] = *source_floor;
    current->queue[current->queue_length - 1] = *destination_floor;

    /* FOR TESTING - REMOVE LATER */
    printf("Car %s\'s queue of length %d: ", current->name, current->queue_length);
    for (int index = 0; index < current->queue_length; index++)
    {
      printf("%d,", current->queue[index]);
    }
    printf("\n");

    // signal the watching queue thread to wake up
    pthread_cond_signal(&current->queue_cond);
  }

  return found;
}