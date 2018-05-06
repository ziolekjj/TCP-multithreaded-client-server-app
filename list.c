#include "list.h"

void add_thread(thread_t **head, pthread_t tid, int id) {
  thread_t *newNode = malloc(sizeof(thread_t));
  if (newNode == NULL)
    ERR("malloc");

  newNode->tid = tid;
  newNode->id = id;
  newNode->next = *head;
  *head = newNode;
}

void remove_thread(thread_t **head, pthread_t tid) {
  if (*head == NULL)
    return;

  if ((*head)->tid == tid) {
    thread_t *nodeToRemove = *head;
    *head = nodeToRemove->next;
    free(nodeToRemove);
    return;
  }

  thread_t *previousNode = *head;
  while (previousNode->next != NULL) {
    thread_t *currentNode = previousNode->next;
    if (currentNode->tid == tid) {
      previousNode->next = currentNode->next;
      free(currentNode);
      return;
    }
    previousNode = currentNode;
  }
}

void block_remove_thread(thread_t **head, pthread_t tid, pthread_mutex_t *mutex)
{
    if (pthread_mutex_lock(mutex) != 0)
        ERR("pthread_mutex_lock");

    remove_thread(head, tid);

    if (pthread_mutex_unlock(mutex) != 0)
        ERR("pthread_mutex_unlock");
}

void remove_all_threads(thread_t **head) {
  while (*head != NULL) {
    thread_t *next = (*head)->next;
    free(*head);
    *head = next;
  }
}

thread_t *find_thread(thread_t *head, pthread_t tid) {
  while (head != NULL) {
    if (head->tid == tid)
      return head;
      head = head->next;
  }

  return NULL;
}

void join_threads(thread_t *head, pthread_mutex_t *listMutex) {
  if (listMutex != NULL && pthread_mutex_lock(listMutex) != 0)
    ERR("pthread_mutex_lock");

  thread_t *node = head;
    while (node != NULL) {
      if (pthread_join(node->tid, NULL) != 0)
        ERR("pthread_join");
      node = node->next;
    }

  if (listMutex != NULL && pthread_mutex_unlock(listMutex) != 0)
    ERR("pthread_mutex_unlock");
}


void add_client(client_t **head, int clientSocket, struct sockaddr_in *clientAddr, socklen_t *clientAddrLen) {
  client_t *newNode = malloc(sizeof(client_t));
  if (newNode == NULL)
    ERR("malloc");

  newNode->clientSocket = clientSocket;
  newNode->clientAddr = *clientAddr;
  newNode->clientAddrLen = *clientAddrLen;
  newNode->next = *head;
  *head = newNode;
}

client_t *pop_client(client_t **head) {
  if (*head == NULL)
    return NULL;
  client_t *nodeToReturn = *head;
  *head = (*head)->next;
  return nodeToReturn;
}

client_t *block_pop_client(client_t **head, pthread_mutex_t *mutex) {
  if (pthread_mutex_lock(mutex) != 0)
    ERR("pthread_mutex_lock");

  client_t *client = pop_client(head);

  if (pthread_mutex_unlock(mutex) != 0)
    ERR("pthread_mutex_unlock");

  return client;
}

void remove_all_clients(client_t **head) {
  while (*head != NULL) {
    client_t *next = (*head)->next;
    // if (TEMP_FAILURE_RETRY(close((*head)->clientSocket)) < 0)
    if (close((*head)->clientSocket) < 0)
      ERR("close");
    free(*head);
    *head = next;
  }
}
