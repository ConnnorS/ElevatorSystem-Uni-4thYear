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