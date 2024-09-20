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

void *handle_client(void *arg)
{
  printf("New Car Thread Created\n");
  client_t client = *(client_t *)arg;
  while (1)
  {
    char *message = receive_message(client.fd);
    if (message == NULL)
    {
      return NULL;
    }
    else if (strncmp(message, "CAR", 3) == 0)
    {
      handle_received_car_message(&client, message);
    }
    else if (strncmp(message, "STATUS", 6) == 0)
    {
      handle_received_status_message(&client, message);
    }
  }

  return NULL;
}

int main(void)
{
  client_t *clients = malloc(0);
  int client_count = 0;

  int new_socket;
  int serverFd = create_server();

  struct sockaddr clientaddr;
  socklen_t clientaddr_len;

  while (1)
  {
    new_socket = accept(serverFd, (struct sockaddr *)&clientaddr, &clientaddr_len);
    if (new_socket >= 0)
    {
      clients = realloc(clients, (client_count + 1) * sizeof(client_t));
      clients[client_count].fd = new_socket;
      pthread_t new_client_thread;
      pthread_create(&new_client_thread, NULL, handle_client, (void *)&clients[client_count]);
      client_count++;
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