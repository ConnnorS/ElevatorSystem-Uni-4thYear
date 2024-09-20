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

/* expects no command line arguments */
int main(void)
{
  controller_car_info *car_list = malloc(sizeof(controller_car_info));
  int car_list_count = 1;

  car_status_info *car_status_list = malloc(sizeof(car_status_info));
  int car_status_list_count = 1;

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
      if (strncmp(message, "CAR", 3) == 0)
      {
        handle_received_car_message(&car_list, message, &car_list_count);
      }
      else if (strncmp(message, "STATUS", 6) == 0)
      {
        handle_received_status_message(&car_status_list, message, &car_status_list_count);
      }
      if (message == NULL) // connection closed by elevator, break the loop and listen again
      {
        break;
      }
      free(message);
    }
  }
  close(clientFd);

  free(car_list);

  return 0;
}