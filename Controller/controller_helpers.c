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

int floors_are_in_range(int sourceFloor, int destinationFloor, int lowestFloor, int highestFloor)
{
  return (sourceFloor >= lowestFloor && sourceFloor <= highestFloor) &&
         (destinationFloor >= lowestFloor && destinationFloor <= highestFloor);
}

void append_floors(client_t *client, int *source_floor, int *destination_floor, int *direction)
{
  char source[5];
  char destination[5];

  /* determine the direction and add the floors */
  if (*direction == UP)
  {
    snprintf(source, sizeof(source), "U%d", *source_floor);
    snprintf(destination, sizeof(destination), "U%d", *destination_floor);
  }
  else
  {
    snprintf(source, sizeof(source), "D%d", *source_floor);
    snprintf(destination, sizeof(destination), "D%d", *destination_floor);
  }

  /* change the - to a B if negative floor */
  if (*source_floor < 0)
  {
    source[1] = 'B';
  }
  if (*destination_floor < 0)
  {
    destination[1] = 'B';
  }

  /* append the floors to the end of the queue */
  char *src_ptr = malloc(strlen(source) + 1);
  char *dst_ptr = malloc(strlen(destination) + 1);
  strcpy(src_ptr, source);
  strcpy(dst_ptr, destination);

  client->queue_length += 2;
  client->queue = realloc(client->queue, sizeof(char *) * client->queue_length);
  client->queue[client->queue_length - 2] = src_ptr;
  client->queue[client->queue_length - 1] = dst_ptr;
}

void add_floor_at_index(client_t *client, char *floor, int *index, int *direction)
{
  printf("Inserting %s at index %d\n", floor, *index);
  /* allocate memory for the new floor */
  char new_floor[5];
  if (*direction == UP)
  {
    snprintf(new_floor, sizeof(new_floor), "U%s", floor);
  }
  else
  {
    snprintf(new_floor, sizeof(new_floor), "D%s", floor);
  }

  char *new_floor_ptr = malloc(strlen(new_floor) + 1);
  strcpy(new_floor_ptr, new_floor);

  /* allocate more memory for the queue*/
  client->queue_length += 1;
  client->queue = realloc(client->queue, sizeof(char *) * client->queue_length);

  /* shift all the values to the right at index */
  for (int end = client->queue_length - 1; end > *index; end--)
  {
    client->queue[end] = client->queue[end - 1];
  }
  client->queue[*index] = new_floor_ptr;
}

void insert_floor_in_block(client_t *client, int *index, int *floor_int, char *floor, int *direction)
{
  int block_index = *index;
  for (; block_index < client->queue_length; block_index++)
  {
    char current_floor[4];
    strcpy(current_floor, client->queue[block_index] + 1); // strip off the 'U' or 'D'
    int current_floor_int = floor_char_to_int(current_floor);

    if (current_floor_int == *floor_int)
    {
      return; // duplicate, don't add
    }

    if (
        (*direction == UP && *floor_int < current_floor_int) ||
        (*direction == DOWN && *floor_int > current_floor_int))
    {
      break;
    }
  }

  add_floor_at_index(client, floor, &block_index, direction);
  pthread_cond_signal(&client->queue_cond);
}

void add_floors_to_queue(client_t *client, char *source, int *source_int, char *destination, int *destination_int)
{
  /* work out the direction of the request */
  int direction = *source_int < *destination_int ? UP : DOWN;

  /* if the queue is empty, just append the floors */
  if (client->queue_length == 0)
  {
    printf("Queue is empty - appending floors\n");
    append_floors(client, source_int, destination_int, &direction);
    return;
  }

  /* if not, find where the floor's up or down block will start */
  int index = 0;
  int found_block = 0;
  for (; index < client->queue_length; index++)
  {
    /* if going up - stop at the up block or if going down - stop at the down block */
    if ((client->queue[index][0] == 'U' && direction == UP) ||
        (client->queue[index][0] == 'D' && direction == DOWN))
    {
      printf("Found block at index %d\n", index);
      found_block = 1;
      break;
    }
  }

  /* if we didn't find a block - append to start a new block */
  if (!found_block)
  {
    printf("No block found - appending floors\n");
    append_floors(client, source_int, destination_int, &direction);
    return;
  }

  /* additionally, if any of the floors fall outside the elevator's current floor - append to the end */
  int car_current_floor = floor_char_to_int(client->current_floor);
  if (
      (client->direction == UP && direction == UP && *source_int < car_current_floor) ||
      (client->direction == DOWN && direction == DOWN && *source_int > car_current_floor))

  {
    printf("Found block but floors fall out of car's current range - appending\n");
    append_floors(client, source_int, destination_int, &direction);
    return;
  }

  /* otherwise, insert the floor in the block */
  insert_floor_in_block(client, &index, source_int, source, &direction);
  insert_floor_in_block(client, &index, destination_int, destination, &direction);
}

/* finds the fd of the car which can service the floors then adds them to the queue */
int find_car_for_floor(char *source, char *destination, client_t **clients, int *client_count, char *chosen_car)
{
  int found = 0;
  client_t **options = malloc(0);
  int options_count = 0;

  int source_floor_int = floor_char_to_int(source);
  int destination_floor_int = floor_char_to_int(destination);

  /* find the clients which can service this request */
  client_t *current;
  for (int index = 0; index < *client_count; index++)
  {
    current = (client_t *)clients[index];
    int current_lowest_floor_int = floor_char_to_int(current->lowest_floor);
    int current_highest_floor_int = floor_char_to_int(current->highest_floor);
    int can_service = floors_are_in_range(source_floor_int, destination_floor_int, current_lowest_floor_int, current_highest_floor_int);
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

    /* add the floors to their queue */
    add_floors_to_queue(current, source, &source_floor_int, destination, &destination_floor_int);

    // /* FOR TESTING - REMOVE LATER */
    printf("Car %s\'s queue of length %d: ", current->name, current->queue_length);
    for (int index = 0; index < current->queue_length; index++)
    {
      printf("%s,", current->queue[index]);
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
  char source[4], destination[4];
  sscanf(message, "%*s %3s %3s", source, destination);
  printf("Received call message for %s-%s\n", source, destination);

  /* find car to service */
  char chosen_car[64];
  if (find_car_for_floor(source, destination, clients, client_count, chosen_car))
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

/* remove the most recent floor from the queue */
void remove_from_queue(client_t *client)
{
  /* free the char */
  free(client->queue[0]);

  /* shift all the pointers down to overwrite the first value */
  for (int index = 0; index < client->queue_length - 1; index++)
  {
    client->queue[index] = client->queue[index + 1];
  }

  /* reduce the length of the queue */
  client->queue_length -= 1;
  client->queue = realloc(client->queue, sizeof(char *) * client->queue_length);
}

/* remove the specified client from the queue */
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