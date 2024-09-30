// basic C stuff
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// networks
#include <arpa/inet.h>
#include <unistd.h>

/* simple function to set up a TCP connection with the controller
then return an int which is the socketFd */
int connect_to_control_system()
{
  // create the address
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) == -1)
  {
    perror("inet_pton()");
    return -1;
  }
  addr.sin_family = AF_INET;
  addr.sin_port = htons(3000);

  // create the socket
  int socketFd = socket(AF_INET, SOCK_STREAM, 0);
  if (socketFd == -1)
  {
    perror("socket()");
    return -1;
  }

  // connect to the server
  if (connect(socketFd, (const struct sockaddr *)&addr, sizeof(addr)) == -1)
  {
    perror("connect()");
    return -1;
  }

  return socketFd;
}

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