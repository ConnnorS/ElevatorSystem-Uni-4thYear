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
// signal
#include <signal.h>
// comms
#include "../common_comms.h"

/* global variables */
volatile sig_atomic_t system_running = 1;

/* for n number of connected clients there will be
n number of threads running this function */
void *handle_client(void *arg)
{
  sig_atomic_t thread_running = 1;

  client_t *client = (client_t *)arg;
  pthread_mutex_lock(&client->mutex);
  int fd = client->fd;
  pthread_mutex_unlock(&client->mutex);

  printf("New client connected with fd %d\n", fd);

  while (system_running && thread_running)
  {
    char *message = receive_message(fd);
    if (message == NULL)
    {
      thread_running = 0;
    }
    else
    {
      printf("%s\n", message);
    }
  }

  printf("fd %d handler thread ending - client disconnected\n", fd);
  return NULL;
}

void thread_cleanup(int signal)
{
  system_running = 0;
}

int main(void)
{
  signal(SIGINT, thread_cleanup);

  /* create the empty array of clients */
  client_t *clients = malloc(0);
  int client_count = 0;

  /* startup the server */
  int serverFd = create_server();

  struct sockaddr clientaddr;
  socklen_t clientaddr_len;

  /* for each new connected client, handle them on a thread */
  int new_socket = -1;
  while (system_running)
  {
    new_socket = accept(serverFd, (struct sockaddr *)&clientaddr, &clientaddr_len);
    if (new_socket >= 0)
    {
      /* increase the clients array */
      clients = realloc(clients, sizeof(client_t) * (client_count + 1));
      clients[client_count].fd = new_socket;
      /* create the handler thread */
      pthread_t client_handler_thread;
      pthread_create(&client_handler_thread, NULL, handle_client, (void *)&clients[client_count]);

      client_count++;
    }
    else
    {
      printf("Max clients reached. Connection rejected\n");
      close(new_socket);
    }
  }

  free(clients);
  close(serverFd);

  return 0;
}