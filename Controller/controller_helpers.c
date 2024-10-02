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
the car's data and saves it in the car's client_info data structure */
void handle_received_car_message(client_info *client, char *message)
{
  char name[CAR_NAME_LENGTH];
  char lowest_floor[4];
  char highest_floor[4];
  sscanf(message, "%*s %31s %3s %3s", name, lowest_floor, highest_floor);
  strcpy(client->highest_floor, highest_floor);
  strcpy(client->lowest_floor, lowest_floor);
  strcpy(client->name, name);
}

/* this does nothing yet except for extact data */
void handle_received_status_message(client_info *client, char *message)
{
  char status[8];
  char current_floor[4];
  char destination_floor[4];
  sscanf(message, "%*s %7s %3s %3s", status, current_floor, destination_floor);
}

/* this will extract the call message and assign it to the call_msg */
void handle_received_call_message(char *message, call_msg_info *call_msg)
{
  char source_floor[4];
  char destination_floor[4];
  sscanf(message, "%*s %3s %3s", source_floor, destination_floor);

  call_msg->source_floor = floor_char_to_int(source_floor);
  call_msg->destination_floor = floor_char_to_int(destination_floor);
}

/* returns the fd of the client which can service the given floors */
int find_car_for_floor(call_msg_info *call_msg, client_info *clients, int client_count, char car_name[CAR_NAME_LENGTH])
{
  // the floors each client (car) can service
  int serviceable_lowest_floor_int;
  int serviceable_highest_floor_int;

  printf("Car called for floor %d to %d\n", call_msg->source_floor, call_msg->destination_floor);

  // loop through the array of clients to find one that can service the floors
  for (int index = 0; index < client_count; index++)
  {
    // if the client's highest_floor and lowest_floor is empty - they're not a car so ignore them
    if (strcmp(clients[index].highest_floor, "") != 0 && strcmp(clients[index].lowest_floor, "") != 0)
    {
      serviceable_highest_floor_int = floor_char_to_int(clients[index].highest_floor);
      serviceable_lowest_floor_int = floor_char_to_int(clients[index].lowest_floor);

      // if the current client can serve the range of floors then we'll return that client's fd
      if (serviceable_highest_floor_int >= call_msg->destination_floor && serviceable_lowest_floor_int <= call_msg->source_floor)
      {
        strcpy(car_name, clients[index].name);
        printf("Car with fd %d can service this request\n", clients[index].fd);
        return clients[index].fd;
      }
    }
  }

  return -1;
}