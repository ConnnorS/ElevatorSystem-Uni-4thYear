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
#include <fcntl.h>
#include <sys/socket.h>
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
void system_shutdown(int sig);   // handle CTRL + C

int main(void)
{
  signal(SIGINT, system_shutdown);

  clients = malloc(0);

  /* start up the server and make non-blocking */
  int serverFd = create_server();
  int flags = fcntl(serverFd, F_GETFL, 0);
  fcntl(serverFd, F_SETFL, flags | O_NONBLOCK);

  struct sockaddr clientaddr;
  socklen_t clientaddr_len;

  /* for each new connected client, handle them in a thread */
  int new_socket = -1;
  while (system_running)
  {
    new_socket = accept(serverFd, (struct sockaddr *)&clientaddr, &clientaddr_len);
    if (new_socket >= 0)
    {
      /* allocate a section of memory for a new client */
      client_t *new_client = malloc(sizeof(client_t));

      new_client->fd = new_socket;
      new_client->connected = 1;

      pthread_mutex_lock(&clients_mutex);
      /* increase the array of pointers to clients */
      clients = realloc(clients, sizeof(client_t *) * (client_count + 1));
      clients[client_count] = new_client;
      client_count++;
      pthread_mutex_unlock(&clients_mutex);

      /* create the handler thread */
      pthread_t client_handler_thread;
      pthread_create(&client_handler_thread, NULL, client_handler, new_client);
    }
  }

  return 0;
}

void system_shutdown(int sig)
{
  system_running = 0;
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
  printf("New client connected with fd %d\n", client->fd);
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
    else if (strncmp(message, "CAR", 3) == 0)
    {
      handle_received_car_message(client, message);
      printf("New car connected: %s %s %s\n", client->name, client->lowest_floor, client->highest_floor);
      pthread_create(&queue_manager_thread, NULL, queue_manager, (void *)client);
      client->type = IS_CAR;
    }
    /* received status message */
    else if (strncmp(message, "STATUS", 6) == 0)
    {
      handle_received_status_message(client, message);
      printf("Received status message from %s: %s %s %s\n", client->name, client->status, client->current_floor, client->destination_floor);
      printf("Client's direction is currently: %s\n", client->direction == UP ? "UP" : client->direction == DOWN ? "DOWN"
                                                                                                                 : "STILL");
    }
    /* call pad connected */
    else if (strncmp(message, "CALL", 4) == 0)
    {
      handle_received_call_message(client, message, clients, &client_count);
      /* call pads don't need to stay connected so thread can end */
      pthread_mutex_unlock(&clients_mutex);
      break;
    }
    /* car in service mode or emergency mode */
    else if (strcmp(message, "INDIVIDUAL SERVICE") == 0 || strcmp(message, "EMERGENCY") == 0)
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

    /* wait while */
    while (
        client->connected && // the client is connected

        /* any of the following are true */
        (strcmp(client->current_floor, client->destination_floor) != 0 || // the client is not at its destination floor
         strcmp(client->status, "Closed") != 0 ||                         // the client is not "Opening"
         client->queue_length == 0) &&                                    // the queue is empty

        /* any of the following are true */
        (client->queue_length < 1 ||                                  // prevents a seg fault caused by strcmp if queue is empty
         strcmp(client->destination_floor, client->queue[0] + 1) == 0 // the client's destination floor matches the first floor in the queue
         ))
    {
      pthread_cond_wait(&client->queue_cond, &clients_mutex);
    }

    /* if client disconnects, end the thread */
    if (client->connected == 0)
    {
      pthread_mutex_unlock(&clients_mutex);
      break;
    }

    /* if the destination floor does not match the first floor in the queue - send a new floor request */
    if (client->queue_length > 0 && strcmp(client->destination_floor, client->queue[0] + 1) != 0)
    {
      char message[16];
      strcpy(client->destination_floor, client->queue[0] + 1);
      snprintf(message, sizeof(message), "FLOOR %s", client->destination_floor);
      send_message(client->fd, message);
    }
    /* if the client hits the destination floor we can now remove that floor from the queue triggering the above if statement */
    if (client->queue_length > 0 && strcmp(client->current_floor, client->destination_floor) == 0)
    {
      remove_from_queue(client);
    }

    /* also work out the current direction of the car */
    int client_current_int = floor_char_to_int(client->current_floor);
    int client_destination_int = floor_char_to_int(client->destination_floor);
    client->direction = client_current_int < client_destination_int ? UP : client_current_int == client_destination_int ? STILL
                                                                                                                        : DOWN;

    pthread_mutex_unlock(&clients_mutex);
  }

  /* upon thread end */
  pthread_mutex_lock(&clients_mutex);
  printf("Queue manager thread for %s ending\n", client->name);
  pthread_mutex_unlock(&clients_mutex);
  return NULL;
}