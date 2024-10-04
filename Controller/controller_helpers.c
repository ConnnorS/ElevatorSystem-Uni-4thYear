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
  char name[64];
  char lowest_floor[4];
  char highest_floor[4];
  sscanf(message, "%*s %31s %3s %3s", name, lowest_floor, highest_floor);
  strcpy(client->highest_floor, highest_floor);
  strcpy(client->lowest_floor, lowest_floor);
  strcpy(client->name, name);
}