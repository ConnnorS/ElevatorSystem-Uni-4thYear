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
#include "../common_networks.h"
#include "../common_helpers.h"

/* expect CL-arguments
{source floor} {destination floor} */
int main(int argc, char **argv)
{
  /* check user input */
  if (argc != 3)
  {
    printf("Useage %s {source floor} {destination floor}\n", argv[0]);
    exit(1);
  }

  char source_floor[4];
  strcpy(source_floor, argv[1]);
  char destination_floor[4];
  strcpy(destination_floor, argv[2]);

  /* connect to the control system */
  int serverFd = connect_to_control_system();

  /* prepare and send the message */
  char message[64];
  snprintf(message, sizeof(message), "CALL %s %s", source_floor, destination_floor);
  send_message(serverFd, message);

  /* await a response */

  while (1)
    ;

  printf("%s", message);

  return 0;
}