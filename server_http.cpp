#include <ctype.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include "task.hpp"
#include "thread_pool.hpp"

#define QUEUE_MAX_COUNT 10

using namespace std;

int main(int argc, char *argv[]) {
  int port = 80;
  if (argc == 3) {
    port = stoi(argv[2]);
  }
  int server_fd = -1;
  int client_fd = -1;
  int opt = -1;
  struct sockaddr_in client_addr;
  struct sockaddr_in server_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    perror("socket error");
    exit(0);
  }

  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setsockopt error");
    exit(0);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("bind error");
    exit(0);
  }
  if (listen(server_fd, QUEUE_MAX_COUNT) < 0) {
    perror("listen error");
    exit(0);
  }

  cout << "http server running on port " << port << endl;

  ThreadPool<Task> pool(20);
  pool.start();

  while (true) {
    client_fd =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    // cout << "client accept" << endl;
    Task *task = new Task(client_fd, true);
    pool.add_task(task);
  }
  close(server_fd);
  return 0;
}