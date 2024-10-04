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
// header file
#include "controller_helpers.h"

/* global variables */
volatile sig_atomic_t running = 1;

/* for n number of connected clients there will be
n number of threads running this function */
void *handle_client(void *arg)
{
  client_t *client = (client_t *)arg;
  pthread_mutex_lock(&client->mutex);
  int fd = client->fd;
  pthread_mutex_unlock(&client->mutex);

  printf("New client connected with fd %d\n", fd);

  while (running)
  {
    
  }

  return NULL;
}

int main(void)
{
  /* create the empty array of clients */
  client_t *clients = malloc(0);
  int client_count = 0;

  /* startup the server */
  int serverFd = create_server();

  struct sockaddr clientaddr;
  socklen_t clientaddr_len;

  /* for each new connected client, handle them on a thread */
  int new_socket;
  while (running)
  {
    if (new_socket >= 0)
    {
      /* increase the clients array */
      client_count++;
      new_socket = accept(serverFd, (struct sockaddr *)&clientaddr, &clientaddr_len);
      clients = realloc(clients, sizeof(client_t) * client_count);
      clients[client_count].fd = new_socket;
      /* create the handler thread */
      pthread_t client_handler_thread;
      pthread_create(&client_handler_thread, NULL, handle_client, (void *)&clients[client_count]);
    }
  }
}