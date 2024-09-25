int connect_to_control_system();
int sendMessage(int socketFd, const char *message);
char *receive_floor(int serverFd);