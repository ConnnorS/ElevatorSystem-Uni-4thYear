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
#include "../type_conversions.h"
#include "../common_comms.h"

#define UP 1
#define DOWN -1

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

int floors_are_in_range(int sourceFloor, int destinationFloor, int lowestFloor, int highestFloor)
{
  return (sourceFloor >= lowestFloor && sourceFloor <= highestFloor) &&
         (destinationFloor >= lowestFloor && destinationFloor <= highestFloor);
}

void add_floor_to_queue(int **queue, int *queue_length, int *floor, int *call_direction)
{
  /* check if the floor is already in the queue */
  for (int index = 0; index < *queue_length; index++)
  {
    if ((*queue)[index] == *floor)
      return;
  }
  
  *queue_length += 1;
  *queue = realloc(*queue, sizeof(int) * (*queue_length));
  (*queue)[*queue_length - 1] = *floor;
}

/* finds the fd of the car which can service the floors then adds them to the queue */
int find_car_for_floor(int *source_floor, int *destination_floor, client_t **clients, int *client_count, char *chosen_car)
{
  int found = 0;
  client_t **options = malloc(0);
  int options_count = 0;

  /* find the clients which can service this request */
  client_t *current;
  for (int index = 0; index < *client_count; index++)
  {
    current = (client_t *)clients[index];
    int current_lowest_floor_int = floor_char_to_int(current->lowest_floor);
    int current_highest_floor_int = floor_char_to_int(current->highest_floor);
    int can_service = floors_are_in_range(*source_floor, *destination_floor, current_lowest_floor_int, current_highest_floor_int);
    if (current->type == IS_CAR && can_service)
    {
      found = 1;
      /* add the client to our list of options */
      options_count += 1;
      options = realloc(options, sizeof(client_t *) * options_count);
      options[options_count - 1] = current;
    }
  }

  if (found)
  {
    /* find the car with the smallest queue to minimise disruptions */
    current = options[0];
    for (int index = 1; index < options_count; index++)
    {
      if (options[index]->queue_length < current->queue_length)
      {
        current = options[index];
      }
    }

    strcpy(chosen_car, current->name);

    /* determine the direction of the floors */
    int direction = *source_floor < *destination_floor ? UP : DOWN;

    /* add the floors to their queue */
    add_floor_to_queue(&current->queue, &current->queue_length, source_floor, &direction);
    add_floor_to_queue(&current->queue, &current->queue_length, destination_floor, &direction);

    // /* FOR TESTING - REMOVE LATER */
    printf("Car %s\'s queue of length %d: ", current->name, current->queue_length);
    for (int index = 0; index < current->queue_length; index++)
    {
      printf("%d,", current->queue[index]);
    }
    printf("\n");

    /* signal the watching queue thread to wake up */
    pthread_cond_signal(&current->queue_cond);
  }

  free(options);
  return found;
}

/* gets the desired floors of the call message and then searches for a client to service those floors */
void handle_received_call_message(client_t *client, char *message, client_t **clients, int *client_count)
{
  client->type = IS_CALL;
  /* extract data */
  int source_floor = 0, destination_floor = 0;
  extract_call_floors(message, &source_floor, &destination_floor);
  printf("Received call message for %d-%d\n", source_floor, destination_floor);

  /* find car to service */
  char chosen_car[64];
  if (find_car_for_floor(&source_floor, &destination_floor, clients, client_count, chosen_car))
  {
    printf("%s can service this request\n", chosen_car);
    char response[68];
    snprintf(response, sizeof(response), "Car %s", chosen_car);
    send_message(client->fd, response);
  }
  else
  {
    send_message(client->fd, "UNAVAILABLE");
  }
}

/* helper to initialise the condition variable for each client */
void initialise_cond(client_t *client)
{
  pthread_condattr_t cond_attr;
  pthread_condattr_init(&cond_attr);
  pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
  pthread_cond_init(&client->queue_cond, NULL);
}

/* remove the most recent floor from the queue */
void remove_from_queue(client_t *client)
{
  /* shift all the values down to overwrite the first value */
  for (int index = 0; index < client->queue_length; index++)
  {
    client->queue[index] = client->queue[index + 1];
  }

  /* reduce the queue length and realloc memory */
  client->queue_length--;
  client->queue = realloc(client->queue, sizeof(int) * client->queue_length);
}

void remove_client(client_t *client, client_t ***clients, int *client_count)
{
  if (*client_count > 1)
  {
    int index;

    for (index = 0; index < *client_count; index++)
    {
      if ((*clients)[index] == client)
      {
        break;
      }
    }

    /* if the disconnecting client is not at the end */
    if (index < *client_count)
    {
      /* shift all the elements in the clients array to the right */
      for (; index < *client_count - 1; index++)
      {
        clients[index] = clients[index + 1];
      }
    }
    *clients = realloc(*clients, sizeof(client_t *) * (*client_count - 1));
  }

  /* realloc memory */
  *client_count = *client_count - 1;

  /* finally, free the client object and associated pointers */
  free(client->queue);
  pthread_cond_destroy(&client->queue_cond);
  free(client);
}