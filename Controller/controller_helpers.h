int create_server();

typedef struct
{
  int fd;                // file descriptor of the client
  pthread_mutex_t mutex; // mutex for this client specifically
  pthread_cond_t cond;   // condition to indicate some value has been updated
} client_t;