// basic C
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
// pthreads
#include <pthread.h>
// signal
#include <signal.h>
// networks
// helpers
#include "controller_helpers.h"
#include "../type_conversions.h"
#include "../common_comms.h"

/* global variables */
volatile sig_atomic_t system_running = 1;
client_t **clients; // array of pointers for each client_t object
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
int client_count = 0;

/* function declarations */
void *queue_manager(void *arg);  // manages the queue for each connected client_t and sends FLOOR messages
void *client_handler(void *arg); // manages the incoming messages for each connected client_t and updates its info

int main(void)
{
  return 0;
}

void *client_handler(void *arg)
{
  pthread_t queue_manager_thread;

  client_t *client = (client_t *)arg;

  pthread_mutex_lock(&clients_mutex);
  pthread_cond_init(&client->queue_cond, NULL);
  int fd = client->fd; // local variable to avoid constant mutex locking/unlocking
  client->queue = malloc(0);
  client->queue_length = 0;
  printf("New client %s connected with fd %d\n", client->name, client->fd);
  pthread_mutex_unlock(&clients_mutex);

  while (system_running && client->connected)
  {
    char *message = receive_message(fd);
    pthread_mutex_lock(&clients_mutex);
    /* client disconnected */
    if (message == NULL)
    {
      pthread_mutex_unlock(&clients_mutex);
      break;
    }
    /* new car connected */
    else if (strcmp(message, "CAR") == 0)
    {
      handle_received_car_message(client, message);
      printf("New car connected: %s %s %s\n", client->name, client->lowest_floor, client->highest_floor);
      pthread_create(&queue_manager_thread, NULL, queue_manager, (void *)client);
      client->type = IS_CAR;
    }
    /* received status message */
    else if (strcmp(message, "STATUS") == 0)
    {
      handle_received_status_message(client, message);
      printf("Received status message from %s: %s %s %s\n", client->name, client->status, client->current_floor, client->destination_floor);
    }
    /* call pad connected */
    else if (strcmp(message, "CALL") == 0)
    {
      handle_received_call_message(client, message, clients, &client_count);
      /* call pads don't need to stay connected so thread can end */
      pthread_mutex_unlock(&clients_mutex);
      break;
    }
    /* car in service mode */
    else if (strcmp(message, "INDIVIDUAL SERVICE") == 0)
    {
      printf("Car %s is in %s\n", client->name, message);
    }
    /* car is in emergency mode */
    else if (strcmp(message, "EMERGENCY") == 0)
    {
      printf("Car %s is in %s\n", client->name, message);
    }

    pthread_mutex_unlock(&clients_mutex);
  }

  /* wait for the queue manager thread to end if it was a car that was connected */
  if (client->type == IS_CAR)
  {
    pthread_mutex_lock(&clients_mutex);
    client->connected = 0;
    pthread_cond_signal(&client->queue_cond);
    pthread_mutex_unlock(&clients_mutex);

    pthread_join(queue_manager_thread, NULL);
  }

  pthread_mutex_lock(&clients_mutex);
  printf("Client handler thread for %s ending\n", client->name);
  remove_client(client, &clients, &client_count);
  pthread_mutex_unlock(&clients_mutex);

  close(fd);
  return NULL;
}

void *queue_manager(void *arg)
{
  client_t *client = arg;

  while (system_running && client->connected)
  {
    pthread_mutex_lock(&clients_mutex);

    while (
        client->connected && // the client is connected

        (strcmp(client->current_floor, client->destination_floor) != 0 || // the current floor isn't the destination floor
         strcmp(client->status, "Closed") != 0 ||                         // the doors are not closed
         client->queue_length == 0)                                       // the queue is empty
    )
    {
      pthread_cond_wait(&client->queue_cond, &clients_mutex);
    }

    /* if client disconnects, end the thread */
    if (client->connected == 0)
    {
      pthread_mutex_unlock(&clients_mutex);
      break;
    }

    /* otherwise, send the next floor request to the client */
    printf("Car %s ready - sending request to floor %d\n", client->name, client->queue[0]);

    char message[64];
    char next_floor[32];
    floor_int_to_char(client->queue[0], (char *)next_floor);
    snprintf(message, sizeof(message), "FLOOR %s", next_floor);
    send_message(client->fd, message);

    /* update the destination floor and status server side to prevent the while loop from sending another request too quickly */
    floor_int_to_char(client->queue[0], client->destination_floor);
    strcpy(client->status, "Opening");

    remove_from_queue(client);

    pthread_mutex_unlock(&clients_mutex);
  }

  /* upon thread end */
  pthread_mutex_lock(&clients_mutex);
  printf("Queue manager thread for %s ending\n", client->name);
  pthread_mutex_unlock(&clients_mutex);
  return NULL;
}