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
    printf("Unable to connect to elevator system.\n");
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

#define TIMEOUT_SEC 1

/* modified receive_message functions stops recv from I/O blocking */
char *receive_message(int socketFd)
{
  char *message;
  int length;
  fd_set read_fds;
  struct timeval timeout;

  // set up the timeout for 1 second
  timeout.tv_sec = TIMEOUT_SEC;
  timeout.tv_usec = 0;

  // initialise the file descriptor set
  FD_ZERO(&read_fds);
  FD_SET(socketFd, &read_fds);

  /* wait for data to be available on the socket */
  int result = select(socketFd + 1, &read_fds, NULL, NULL, &timeout);
  if (result == -1)
  {
    perror("select failed");
    return "";
  }
  else if (result == 0)
  {
    return "";
  }

  /* then actually receive the message */
  if (recv(socketFd, &length, sizeof(length), 0) != sizeof(length))
  {
    fprintf(stderr, "recv got invalid length value\n");
    return NULL;
  }

  length = ntohl(length);
  message = malloc(length + 1);
  if (!message)
  {
    perror("malloc failed");
    return "";
  }

  /* read the message */
  int received_length = recv(socketFd, message, length, 0);
  if (received_length != length)
  {
    fprintf(stderr, "recv got invalid message length\nExpected: %d got: %d\n", length, received_length);
    free(message);
    return "";
  }

  message[length] = '\0';

  return message;
}