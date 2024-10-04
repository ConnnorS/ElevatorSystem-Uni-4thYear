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
// headers
#include "controller_helpers.h"

int create_server()
{
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(3000);

  /* create the file descriptor for the socket */
  int serverFd = socket(AF_INET, SOCK_STREAM, 0);
  if (serverFd == -1)
  {
    perror("socket()");
    return -1;
  }

  /* bind the socket to an actual IP and port */
  int opt_enable = 1;
  setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt_enable, sizeof(opt_enable));
  if (bind(serverFd, (const struct sockaddr *)&addr, sizeof(addr)) == -1)
  {
    perror("bind()");
    return -1;
  }

  /* now get the socket to start listening */
  if (listen(serverFd, 10) == -1)
  {
    perror("listen()");
    return -1;
  }

  return serverFd;
}

/* handles a received message that starts with "CAR", extract's
the car's data and saves it in the car's client_info data structure */
void handle_received_car_message(client_t *client, char *message)
{
  /* extract the data */
  char name[64];
  char lowest_floor[4];
  char highest_floor[4];
  sscanf(message, "%*s %31s %3s %3s", name, lowest_floor, highest_floor);
  /* lock mutex and add the data to the object */
  pthread_mutex_lock(&client->mutex);
  strcpy(client->highest_floor, highest_floor);
  strcpy(client->lowest_floor, lowest_floor);
  strcpy(client->name, name);
  pthread_mutex_unlock(&client->mutex);
}

/* extract all the data from the status message, update the client object
and then signal to the queue manager that something has changed for that
car's floor */
void handle_received_status_message(client_t *client, char *message)
{
  /* extract the data */
  char status[8];
  char current_floor[4];
  char destination_floor[4];
  sscanf(message, "%*s %7s %3s %3s", status, current_floor, destination_floor);
  /* lock the mutex and add the data to the object */
  pthread_mutex_lock(&client->mutex);
  strcpy(client->status, status);
  strcpy(client->current_floor, current_floor);
  strcpy(client->destination_floor, destination_floor);
  pthread_cond_signal(&client->cond); // signal that the floors have changed and the queue thread might need to act
  pthread_mutex_unlock(&client->mutex);
}

void initialise_mutex_cond(client_t *client)
{
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(&client->mutex, &attr);

  pthread_condattr_t cond_attr;
  pthread_condattr_init(&cond_attr);
  pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
  pthread_cond_init(&client->cond, &cond_attr);
}