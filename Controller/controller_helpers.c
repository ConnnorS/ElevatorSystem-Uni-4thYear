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

char *receive_message(int fd)
{
  char *message;
  int length;

  if (recv(fd, &length, sizeof(length), 0) != sizeof(length))
  {
    fprintf(stderr, "recv got invalid length value\n");
    return NULL;
  }

  length = ntohl(length);
  message = malloc(length + 1);
  int received_length = recv(fd, message, length, 0);
  if (received_length != length)
  {
    fprintf(stderr, "recv got invalid legnth message\nExpected: %d got %d\n", length, received_length);
    return NULL;
  }

  message[length] = '\0';

  return message;
}

void handle_received_car_message(client_t *client, char *message)
{
  char type[4];
  char name[32];
  char lowest_floor[4];
  char highest_floor[4];
  sscanf(message, "%3s %31s %3s %3s", type, name, lowest_floor, highest_floor);
  strcpy(client->highest_floor, highest_floor);
  strcpy(client->lowest_floor, lowest_floor);
}

void handle_received_status_message(client_t *client, char *message)
{
  // extract all the data from the message
  char message_type[7];
  char status[8];
  char current_floor[4];
  char destination_floor[4];
  sscanf(message, "%6s %7s %3s %3s", message_type, status, current_floor, destination_floor);
}

int send_floor_request(int clientFd, const char *message)
{
  int message_length = strlen(message);
  int network_length = htonl(message_length);
  send(clientFd, &network_length, sizeof(network_length), 0);
  if (send(clientFd, message, message_length, 0) != message_length)
  {
    printf("Send did not send all data\n");
    return -1;
  }
  return 0;
}