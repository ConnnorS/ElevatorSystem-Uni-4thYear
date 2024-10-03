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

/* reallocs memory and removes the disconnected client from
the clients array */
void remove_client(int fd)
{
  pthread_mutex_lock(&clients_mutex);

  int index = -1;
  for (int i = 0; i < client_count; i++)
  {
    if (clients[i].fd == fd)
    {
      index = i;
      break;
    }
  }

  // shift clients to remove it from the array
  if (index != -1)
  {
    free(clients[index].queue);
    for (int i = index; i < client_count - 1; i++)
    {
      clients[i] = clients[i + 1];
    }
    client_count--;
    clients = realloc(clients, client_count * sizeof(client_info));
  }

  pthread_mutex_unlock(&clients_mutex);
}

/* spawned off from a handle_client thread to look for
updates to the car's queue and send messages accordingly */
void *client_queue_manager(void *arg)
{
  client_info *client = (client_info *)arg;
  pthread_mutex_lock(&client->mutex);
  int fd = client->fd;
  printf("New queue manager thread created for fd %d\n", fd);

  while (1)
  {
    pthread_cond_wait(&client->queue_cond, &client->mutex);
    if (client->queue_length > 0) // put in this check to avoid spontaneous wakeups
    {
      int next_floor = client->queue[0];
      char floor_message[64];
      snprintf(floor_message, sizeof(floor_message), "FLOOR %d", next_floor);
      printf("Sending %s\n", floor_message);
      send_message(fd, floor_message);
      remove_floor(client);
    }
    else
    {
      printf("Queue empty\n");
    }
  }
  pthread_mutex_unlock(&client->mutex);
}

/* for n number of connected clients there will be
n number of threads running this function */
void *handle_client(void *arg)
{
  /* initialise the client data */
  client_info *client = (client_info *)arg;
  // save the fd to a local variable to avoid constant mutex locking and unlocking
  pthread_mutex_lock(&clients_mutex);
  int fd = client->fd;
  char name[CAR_NAME_LENGTH];
  client->queue = malloc(0);
  client->queue_length = 0;
  pthread_mutex_unlock(&clients_mutex);

  printf("New Client Handler Thread Created with fd %d\n", fd);

  while (1)
  {
    char *message = receive_message(fd);
    if (message == NULL)
    {
      break;
    }
    pthread_mutex_lock(&client->mutex);
    if (strncmp(message, "CAR", 3) == 0)
    {
      handle_received_car_message(client, message, name);
      printf("New car: %s\n", message);

      /* spin off the car's queue handler thread */
      pthread_t client_queue_handler_thread;
      pthread_create(&client_queue_handler_thread, NULL, client_queue_manager, (void *)client);
    }
    else if (strncmp(message, "STATUS", 6) == 0)
    {
      handle_received_status_message(client, message);
      printf("%s %s\n", name, message);
    }
    else if (strncmp(message, "CALL", 4) == 0)
    {
      /* extract the call message details */
      client->type = IS_CALL_PAD;
      call_msg_info call_msg;
      parse_received_call_message(message, &call_msg);
      printf("%s\n", message);
      /* find a car to service the call */
      char chosen_car_name[CAR_NAME_LENGTH];
      int call_direction;
      int car_fd = find_car_for_floor(&call_msg, clients, &clients_mutex, client_count, chosen_car_name, &call_direction);
      /* send response to the call pad */
      if (car_fd == -1)
      {
        send_message(fd, "UNAVAILABLE");
      }
      else
      {
        char response[64];
        snprintf(response, sizeof(response), "Car %s", chosen_car_name);
        send_message(fd, response);
        /* add the floors to the chosen car's queue */
        for (int index = 0; index < client_count; index++)
        {
          if (strcmp(chosen_car_name, clients[index].name) == 0)
          {
            add_to_car_queue(&clients[index], &call_msg);
            break;
          }
        }
      }
    }
    pthread_mutex_unlock(&client->mutex);
  }
  printf("Thread ending - client disconnected\n");
  remove_client(fd);
  return NULL;
}

int main(void)
{
  /* setup the clients array */
  pthread_mutex_lock(&clients_mutex);
  clients = malloc(0);
  client_count = 0;
  pthread_mutex_unlock(&clients_mutex);

  int new_socket;
  int serverFd = create_server();

  struct sockaddr clientaddr;
  socklen_t clientaddr_len;
  /* create new threads for each connected client */
  while (1)
  {
    new_socket = accept(serverFd, (struct sockaddr *)&clientaddr, &clientaddr_len);
    if (new_socket >= 0)
    {
      pthread_mutex_lock(&clients_mutex);
      clients = realloc(clients, (client_count + 1) * sizeof(client_info));
      clients[client_count].fd = new_socket;
      clients[client_count].direction = CAR_NEUTRAL;
      pthread_cond_init(&clients[client_count].queue_cond, NULL);
      /* create a new thread to handle each new client */
      pthread_t new_client_infothread;
      pthread_create(&new_client_infothread, NULL, handle_client, (void *)&clients[client_count]);

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