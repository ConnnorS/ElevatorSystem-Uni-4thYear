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
#include <arpa/inet.h>
// signal
#include <signal.h>
// header file
#include "controller_helpers.h"
#include "../common_comms.h"
#include "../type_conversions.h"

/* global variables */
volatile sig_atomic_t system_running = 1;

int **clients; // array of pointers to each client_t object for each connected client
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
int client_count;

/* seperate thread for each connected car to manage its queue
and send requests to move to the floors */
void *queue_manager(void *arg)
{
  client_t *client = arg;
  // printf("New queue manager thread created for client %s\n", client->name);

  while (1)
  {
    pthread_mutex_lock(&clients_mutex);

    /* wait while the client is not at its destination floor, the doors are not closed, and the queue is empty */
    while (strcmp(client->current_floor, client->destination_floor) != 0 || strcmp(client->status, "Closed") != 0 || client->queue_length == 0)
    {
      pthread_cond_wait(&client->queue_cond, &clients_mutex);
    }
    printf("Car %s ready - sending next floor request\n", client->name);

    /* send the next floor message */
    char message[64];
    char next_floor[32];
    floor_int_to_char(client->queue[0], (char *)next_floor);
    snprintf(message, sizeof(message), "FLOOR %s", next_floor);

    send_message(client->fd, message);

    // update the destination floor server side to prevent it from jumping the gun and sending another request
    floor_int_to_char(client->queue[0], client->destination_floor);

    remove_from_queue(client);

    pthread_mutex_unlock(&clients_mutex);
  }

  pthread_mutex_lock(&clients_mutex);
  printf("Queue manager thread for fd %d ending\n", client->fd);
  pthread_mutex_unlock(&clients_mutex);
  return NULL;
}

/* for n number of connected clients there will be
n number of threads running this function */
void *handle_client(void *arg)
{
  sig_atomic_t thread_running = 1;

  client_t *client = (client_t *)arg;

  pthread_mutex_lock(&clients_mutex);
  pthread_cond_init(&client->queue_cond, NULL);
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
      /* create the queue manager thread */
      pthread_t queue_manager_thread;
      pthread_create(&queue_manager_thread, NULL, queue_manager, (void *)client);
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
        printf("%s can service this request\n", chosen_car);
        char response[68];
        snprintf(response, sizeof(response), "Car %s", chosen_car);
        send_message(fd, response);
      }

      /* kill the thread */
      thread_running = 0;
    }
    pthread_mutex_unlock(&clients_mutex);
  }

  pthread_mutex_unlock(&clients_mutex);
  printf("fd %d handler thread ending\n", fd);
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
      client_t *new_client = malloc(sizeof(client_t));

      new_client->fd = new_socket;

      pthread_mutex_lock(&clients_mutex);
      /* increase the array of pointers to clients */
      clients = (int **)realloc(clients, sizeof(int *) * (client_count + 1));
      clients[client_count] = (int *)new_client;
      client_count++;
      pthread_mutex_unlock(&clients_mutex);

      /* create the handler thread */
      pthread_t client_handler_thread;
      pthread_create(&client_handler_thread, NULL, handle_client, new_client);
    }
    else
    {
      printf("Max clients reached. Connection rejected\n");
      close(new_socket);
    }
  }

  /* free each individual client_t */
  printf("Freeing client_t objects\n");
  for (int index = 0; index < client_count; index++)
  {
    client_t *current = (client_t *)clients[index];
    free(current->queue);
    free(current);
  }
  /* and free the array of pointers to the clients array */
  printf("Freeing the clients array\n");
  free(clients);
  printf("Closing socket\n");
  close(serverFd);

  return 0;
}