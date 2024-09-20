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

int sendMessage(int socketFd, const char *message)
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