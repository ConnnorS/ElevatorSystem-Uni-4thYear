// basic C stuff
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// networks
#include <arpa/inet.h>
#include <unistd.h>

int send_message(int socketFd, const char *message)
{
  int message_length = strlen(message);
  int network_length = htonl(message_length);
  send(socketFd, &network_length, sizeof(network_length), 0);
  if (send(socketFd, message, message_length, 0) != message_length)
  {
    printf("Send did not send all data\n");
    return -1;
  }
  return 0;
}

char *receive_message(int socketFd)
{
  char *message;
  int length;

  if (recv(socketFd, &length, sizeof(length), 0) != sizeof(length))
  {
    fprintf(stderr, "recv got invalid length value\n");
    return NULL;
  }

  length = ntohl(length);
  message = malloc(length + 1);
  int received_length = recv(socketFd, message, length, 0);
  if (received_length != length)
  {
    fprintf(stderr, "recv got invalid legnth message\nExpected: %d got %d\n", length, received_length);
    return NULL;
  }

  message[length] = '\0';

  return message;
}