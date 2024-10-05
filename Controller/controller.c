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

client_t *clients;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
int client_count;

/* for n number of connected clients there will be
n number of threads running this function */
void *handle_client(void *arg)
{
  sig_atomic_t thread_running = 1;

  client_t *client = (client_t *)arg;

  pthread_mutex_lock(&clients_mutex);
  initialise_cond(client);
  int fd = client->fd; // local fd variable to avoid constant mutex locks/unlocks
  client->queue = malloc(0);
  client->queue_length = 0;
  pthread_mutex_unlock(&clients_mutex);

  printf("New client connected with fd %d\n", fd);

  while (system_running && thread_running)
  {
    char *message = receive_message(fd);
    pthread_mutex_lock(&clients_mutex);
    if (message == NULL)
    {
      thread_running = 0;
    }
    else if (strncmp(message, "CAR", 3) == 0)
    {
      /* parse in the client's info */
      handle_received_car_message(client, message);
      printf("New car connected: %s %s %s\n", client->name, client->lowest_floor, client->highest_floor);
    }
    else if (strncmp(message, "STATUS", 6) == 0)
    {
      handle_received_status_message(client, message);
      printf("Received status message from %s: %s %s %s\n", client->name, client->status, client->current_floor, client->destination_floor);
    }
    else if (strncmp(message, "CALL", 4) == 0)
    {
      int source_floor, destination_floor;
      extract_call_floors(message, &source_floor, &destination_floor);
      printf("Received call message for %d-%d\n", source_floor, destination_floor);
      char chosen_car[64];
      int found = find_car_for_floor(&source_floor, &destination_floor, clients, client_count, chosen_car);
      if (!found)
      {
        send_message(fd, "UNAVAILABLE");
      }
      else
      {
        char response[68];
        snprintf(response, sizeof(response), "Car %s", chosen_car);
        send_message(fd, response);
      }
      printf("%s can service this request\n", chosen_car);
    }
    else
    {
      printf("%s\n", message);
    }
    pthread_mutex_unlock(&clients_mutex);
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
  clients = malloc(0);
  client_count = 0;

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
      pthread_mutex_lock(&clients_mutex);
      /* increase the clients array */
      clients = realloc(clients, sizeof(client_t) * (client_count + 1));
      clients[client_count].fd = new_socket;
      pthread_mutex_unlock(&clients_mutex);

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