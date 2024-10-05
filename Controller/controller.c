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

client_floors *client_floors_list;
int client_floors_list_length;
pthread_mutex_t client_floors_mutex = PTHREAD_MUTEX_INITIALIZER;

/* a thread which is spun off from a handle_client thread
if it receives a CALL message */
void *call_pad_manager(void *arg)
{
  char *arriving_car = (char *)arg;

  printf("Call message received - creating new handler thread\n");

  return NULL;
}

/* for n number of connected clients there will be
n number of threads running this function */
void *handle_client(void *arg)
{
  sig_atomic_t thread_running = 1;

  client_t *client = (client_t *)arg;
  initialise_mutex_cond(client);

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
    else if (strncmp(message, "CAR", 3) == 0)
    {
      printf("%s\n", message);
      /* parse in the client's info */
      handle_received_car_message(client, message);
      printf("New car connected: %s %s %s\n", client->name, client->lowest_floor, client->highest_floor);
      /* copy that to the floors list too */
      pthread_mutex_lock(&client_floors_mutex);
      add_to_client_floors_list(client, client_floors_list, &client_floors_list_length);
      pthread_mutex_unlock(&client_floors_mutex);
    }
    else if (strncmp(message, "STATUS", 6) == 0)
    {
      handle_received_status_message(client, message);
      printf("Received status message from %s: %s %s %s\n", client->name, client->status, client->current_floor, client->destination_floor);
    }
    else if (strncmp(message, "CALL", 4) == 0)
    {
      /* off another thread to handle this message, wait for a response,
      and then close both threads */
      pthread_t call_pad_manager_thread;
      char arriving_car[64];
      pthread_create(&call_pad_manager_thread, NULL, call_pad_manager, (void *)&arriving_car);

      pthread_join(call_pad_manager_thread, NULL);
      printf("%s\n", arriving_car);
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

  /* setup the floors list */
  client_floors_list = malloc(0);
  client_floors_list_length = 0;

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