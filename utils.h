#include "list.h"

struct server_data {
  int connections;
  int serverSocket;
  sigset_t *oldmask;
  thread_t **threads;
  pthread_mutex_t *threadMutex;
  client_t **clients;
  pthread_mutex_t *clientMutex;
};

volatile sig_atomic_t exit_flag;
void usage(char *fileName);
void sigIntHandler(int signal);
void set_handler(int signal, void (*handler)(int));
void set_signal_handlers(sigset_t *mask, sigset_t *oldmask);
void parse_arguments(int argc, char **argv, int16_t *port);
int make_socket(int domain, int type);
void bind_tcp_socket(int serverSocket, int16_t port, int BACKLOG);
ssize_t bulk_read(int fd, char *buf, size_t count);
ssize_t bulk_write(int fd, char *buf, size_t count);
void remove_all_server_data(struct server_data *server_data);
