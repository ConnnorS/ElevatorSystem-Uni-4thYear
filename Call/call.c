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
  printf("Attempting to connect to control system...\n");
  int serverFd = connect_to_control_system();
  printf("Connection successful\n");

  /* prepare and send the message */
  char message[64];
  snprintf(message, sizeof(message), "CALL %s %s", source_floor, destination_floor);
  printf("Sending request from floor %s to floor %s\n", source_floor, destination_floor);
  send_message(serverFd, message);

  /* await a response */
  char *response = receive_message(serverFd);
  if (strcmp(response, "UNAVAILABLE") == 0)
  {
    printf("Sorry, no car is available to take this request.\n");
  }
  else
  {
    printf("%s is arriving.\n", response);
  }
  return 0;
}