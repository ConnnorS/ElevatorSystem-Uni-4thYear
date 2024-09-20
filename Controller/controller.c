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
  // listen for data from the cars
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

    char *message = receive_message(clientFd);
    if (message == NULL) printf("Failed\n");
    printf("Received %s\n", message);
  }
  
  return 0;
}