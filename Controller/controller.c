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
#include "./Car/car_helpers.h"

typedef struct car_status
{
  char name[CAR_NAME_LENGTH];
  char status[8];
  char current_floor[4];
  char destination_floor[4];
} car_status_t;

/* expects no command line arguments */
int main(void)
{
  car_status_t *car_status_list[] = malloc(sizeof(car_status_t));

  int serverFd = create_server();

  struct sockaddr clientaddr;
  socklen_t clientaddr_len;
  int clientFd;
  while (1)
  {
    clientFd = accept(serverFd, &clientaddr, &clientaddr_len);
    if (clientFd == -1)
    {
      perror("accept()");
      continue; // try again, don't run the rest of the code
    }
    while (1)
    {
      char *message = receive_message(clientFd);
      if (strncmp(message, "STATUS", 6) == 0)
      {
        printf("Received status message\n");
      }
      if (message == NULL) // connection closed by elevator, break the loop and listen again
      {
        break;
      }
      free(message);
    }
  }
  close(clientFd);

  free(car_status_list);
  return 0;
}