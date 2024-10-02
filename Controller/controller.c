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
#include "../common_networks.h"

/* global variables & mutex */
client_info *clients;
int client_count;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

/* for n number of connected clients there will be
n number of threads running this function */
void *handle_client(void *arg)
{
  pthread_mutex_lock(&clients_mutex);
  client_info *client = (client_info *)arg;
  // save the fd to a local variable to avoid constant mutex locking and unlocking
  int fd = client->fd;
  pthread_mutex_unlock(&clients_mutex);

  printf("New Client Handler Thread Created with fd %d\n", fd);

  while (1)
  {
    char *message = receive_message(fd);
    if (message == NULL)
    {
      break;
    }
    pthread_mutex_lock(&clients_mutex);
    if (strncmp(message, "CAR", 3) == 0)
    {
      handle_received_car_message(client, message);
      printf("Received car message: %s\n", message);
    }
    else if (strncmp(message, "STATUS", 6) == 0)
    {
      handle_received_status_message(client, message);
      printf("Received status message: %s\n", message);
    }
    else if (strncmp(message, "CALL", 4) == 0)
    {
      /* if the client is a call pad, we'll need to initialise
      its lowest_floor and highest_floor values to nothing */
      strcpy(client->highest_floor, "");
      strcpy(client->lowest_floor, "");
      strcpy(client->name, "");

      call_msg_info call_msg;
      handle_received_call_message(message, &call_msg);
      printf("Received call message: %s\n", message);

      char car_name[CAR_NAME_LENGTH]; // the name of the car to service the request
      int car_fd = find_car_for_floor(&call_msg, clients, client_count, car_name);
      if (car_fd == -1)
      {
        send_message(fd, "UNAVAILABLE");
      }
      else
      {
        char response[64];
        snprintf(response, sizeof(response), "Car %s", car_name);
        printf("Sending car %s\n", response);
        send_message(fd, response);
      }
    }
    pthread_mutex_unlock(&clients_mutex);
  }
  printf("Thread ending - client disconnected\n");
  return NULL;
}

int main(void)
{
  pthread_mutex_lock(&clients_mutex);
  clients = malloc(0);
  client_count = 0;
  pthread_mutex_unlock(&clients_mutex);

  int new_socket;
  int serverFd = create_server();

  struct sockaddr clientaddr;
  socklen_t clientaddr_len;

  while (1)
  {
    new_socket = accept(serverFd, (struct sockaddr *)&clientaddr, &clientaddr_len);
    if (new_socket >= 0)
    {
      pthread_mutex_lock(&clients_mutex);
      clients = realloc(clients, (client_count + 1) * sizeof(client_info));
      clients[client_count].fd = new_socket;
      /* create a new thread to handle each new client */
      pthread_t new_client_infohread;
      pthread_create(&new_client_infohread, NULL, handle_client, (void *)&clients[client_count]);

      client_count++;
      pthread_mutex_unlock(&clients_mutex);
    }
    else
    {
      printf("Max clients reached. Connection rejected\n");
      close(new_socket);
    }
  }

  free(clients);
  return 0;
}