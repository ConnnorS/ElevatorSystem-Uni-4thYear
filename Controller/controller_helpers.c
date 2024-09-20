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

void handle_received_car_message(controller_car_info **car_list, char *message, int *count)
{
  // extract all the data from the message
  char message_type[4];
  char car_name[32];
  char lowest_floor[4];
  char highest_floor[4];
  sscanf(message, "%3s %31s %3s %3s", message_type, car_name, lowest_floor, highest_floor);

  for (int i = 0; i < *count; i++)
  {
    // check if the car is already in the list
    if (strcmp(car_name, (*car_list)[i].name) == 0)
    {
      return; // do nothing
    }
  }

  // allocate memory for the new car
  *car_list = realloc(*car_list, (*count + 1) * sizeof(controller_car_info));
  if (*car_list == NULL)
  {
    perror("Failed to realloc memory");
    exit(EXIT_FAILURE);
  }

  strcpy((*car_list)[*count].name, car_name);
  strcpy((*car_list)[*count].lowest_floor, lowest_floor);
  strcpy((*car_list)[*count].highest_floor, highest_floor);
  *count += 1;
}

void handle_received_status_message(car_status_info **car_status_list, char *message, int *count)
{
  // extract all the data from the message
  char message_type[7];
  char name[32];
  char status[8];
  char current_floor[4];
  char destination_floor[4];
  sscanf(message, "%6s %31s %7s %3s %3s", message_type, name, status, current_floor, destination_floor);

  for (int i = 0; i < *count; i++)
  {
    // find the car_status value in the list
    if (strcmp(name, (*car_status_list)[i].name) == 0)
    {
      strcpy((*car_status_list)[i].current_floor, current_floor);
      strcpy((*car_status_list)[i].destination_floor, destination_floor);
      strcpy((*car_status_list)[i].status, status);
      return;
    }
  }

  // if we didn't find the car, add it to the list
  *car_status_list = realloc(*car_status_list, (*count + 1) * sizeof(car_status_info));
  if (*car_status_list == NULL)
  {
    perror("Failed to realloc memory");
    exit(EXIT_FAILURE);
  }
  strcpy((*car_status_list)[*count].name, name);
  strcpy((*car_status_list)[*count].status, status);
  strcpy((*car_status_list)[*count].current_floor, current_floor);
  strcpy((*car_status_list)[*count].destination_floor, destination_floor);
  *count = *count + 1;
}