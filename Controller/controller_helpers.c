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
// my functions
#include "controller_helpers.h"
#include "../common_helpers.h"

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
the car's data and saves it in the car's client_t data structure */
void handle_received_car_message(client_t *client, char *message)
{
  char name[32];
  char lowest_floor[4];
  char highest_floor[4];
  sscanf(message, "%*s %31s %3s %3s", name, lowest_floor, highest_floor);
  strcpy(client->highest_floor, highest_floor);
  strcpy(client->lowest_floor, lowest_floor);
}

/* this does nothing yet except for extact data */
void handle_received_status_message(client_t *client, char *message)
{
  char status[8];
  char current_floor[4];
  char destination_floor[4];
  sscanf(message, "%*s %7s %3s %3s", status, current_floor, destination_floor);
}

/* this will extract the data and check if any car can service
the request */
void handle_received_call_message(char *message, call_msg_info *call_msg)
{
  char source_floor[4];
  char destination_floor[4];
  sscanf(message, "%*s %3s %3s", source_floor, destination_floor);

  call_msg->source_floor = floor_char_to_int(source_floor);
  call_msg->destination_floor = floor_char_to_int(destination_floor);
}

void find_car_for_floor(call_msg_info *call_msg, client_t *clients, int client_count)
{
  for (int client_index = 0; client_index < client_count; client_index++)
  {
    if (strcmp(clients[client_index].highest_floor, "") == 0 || strcmp(clients[client_index].lowest_floor, "") == 0)
    {
      printf("We have not a car\n");
    }
    else
    {
      printf("We have a car\n");
    }
  }
}