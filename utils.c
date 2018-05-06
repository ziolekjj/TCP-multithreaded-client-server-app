#include "utils.h"

volatile sig_atomic_t exit_flag = 0;

void usage(char *fileName) {
  fprintf(stderr, "Usage: %s port groupSize\n", fileName);
  exit(EXIT_FAILURE);
}

void sigIntHandler(int signal) {
  exit_flag = 1;
}

void set_handler(int signal, void (*handler)(int)) {
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = handler;
  if (sigaction(signal, &action, NULL) < 0)
    ERR("sigaction");
}

void set_signal_handlers(sigset_t *mask, sigset_t *oldmask) {
  set_handler(SIGINT, sigIntHandler);
  sigset_t blockMask;
  sigemptyset(&blockMask);
  sigaddset(&blockMask, SIGINT);
  sigaddset(&blockMask, SIGPIPE);
  if (sigprocmask(SIG_BLOCK, &blockMask, mask) != 0)
    ERR("sigprocmask");
  memcpy(oldmask, mask, sizeof(sigset_t));
  sigaddset(oldmask, SIGPIPE);
}

void parse_arguments(int argc, char **argv, int16_t *port) {
  if (argc != 2) usage(argv[0]);
  *port = atoi(argv[1]);
}

int make_socket(int domain, int type) {
  int socketDes = socket(domain, type, 0);
  if (socketDes < 0)
    ERR("socket");
  return socketDes;
}

void bind_tcp_socket(int serverSocket, int16_t port, int BACKLOG) {
  struct sockaddr_in serverAddr;
  memset(&serverAddr, 0, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serverAddr.sin_port = htons(port);
  int t = 1;
  if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(int)) < 0)
    ERR("setsockopt");
  if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    ERR("bind");
  if (listen(serverSocket, BACKLOG) < 0)
    ERR("listen");
}

ssize_t bulk_read(int fd, char *buf, size_t count) {
  int c;
  size_t len = 0;
  do {
    // c = TEMP_FAILURE_RETRY(read(fd, buf, count));
    c = read(fd, buf, count);
    if (c < 0) return c;
    if (c == 0) return len;
    buf += c;
    len += c;
    count -= c;
  } while (count > 0);
  return len;
}

ssize_t bulk_write(int fd, char *buf, size_t count) {
  int c;
  size_t len = 0;
  do {
    // c = TEMP_FAILURE_RETRY(write(fd, buf, count));
    c = write(fd, buf, count);
    if(c < 0) return c;
    buf += c;
    len += c;
    count -= c;
  } while (count > 0);
  return len;
}

void remove_all_server_data(struct server_data *server_data) {
  // if (TEMP_FAILURE_RETRY(close(server_data->serverSocket)) < 0)
  if (close(server_data->serverSocket) < 0)
    ERR("close");

  join_threads(*(server_data->threads), server_data->threadMutex);
  remove_all_threads(server_data->threads);

  if (pthread_mutex_destroy(server_data->threadMutex) != 0)
    ERR("pthread_mutex_destroy");

  if (pthread_mutex_destroy(server_data->clientMutex) != 0)
    ERR("pthread_mutex_destroy");

  remove_all_clients(server_data->clients);

  if (sigprocmask(SIG_SETMASK, server_data->oldmask, NULL) != 0)
    ERR("sigprocmask");
}
