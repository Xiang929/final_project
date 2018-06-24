#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include "task.hpp"
#include "thread_pool.hpp"

#define QUEUE_MAX_COUNT 1024
#define MAX_EVENT 1024
#define READ_BUF_LEN 4096

using namespace std;

static int set_non_blocking(int fd) {
  int old_flags, new_flags;
  old_flags = fcntl(fd, F_GETFL, 0);
  if (old_flags == -1) {
    perror("Get fd status");
    return -1;
  }

  old_flags |= O_NONBLOCK;

  new_flags = fcntl(fd, F_SETFL, old_flags);
  if (new_flags == -1) {
    perror("Set fd status");
    return -1;
  }
  return 0;
}

int main(int argc, char *argv[]) {
  int epfd = 0;
  struct epoll_event ev, event[MAX_EVENT];
  int result = 0;
  int port = 80;
  if (argc == 3) {
    if (strcmp(argv[1], "--port")) {
      printf("Please enter \"--port \" to specify the port");
      return 0;
    }
    port = stoi(argv[2]);
  }
  int server_fd = -1;
  int client_fd = -1;
  int opt = -1;
  struct sockaddr_in server_addr;
  // socklen_t client_addr_len = sizeof(client_addr);
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    perror("Socket");
    exit(0);
  }

  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("Setsockopt");
    exit(0);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("Bind");
    exit(0);
  }
  result = set_non_blocking(server_fd);
  if (result == -1) {
    return 0;
  }

  if (listen(server_fd, QUEUE_MAX_COUNT) < 0) {
    perror("Listen");
    exit(0);
  }

  cout << "http server running on port " << port << endl;

  ThreadPool<Task> pool(20);
  pool.start();

  epfd = epoll_create1(0);
  ev.data.fd = server_fd;
  ev.events = EPOLLIN | EPOLLET;

  result = epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);

  if (result == -1) {
    perror("Set epoll_ctl");
    return 0;
  }

  for (;;) {
    int wait_count;
    wait_count = epoll_wait(epfd, event, MAX_EVENT, -1);
    for (int i = 0; i < wait_count; i++) {
      uint32_t events = event[i].events;
      // IP BUFFER
      char host_buf[NI_MAXHOST];
      // PORT BUFFER
      char port_buf[NI_MAXSERV];

      int __result;

      if (events & EPOLLERR || events & EPOLLHUP || (!events & EPOLLIN)) {
        printf("Epoll has error\n");
        close(event[i].data.fd);
        continue;
      } else if (server_fd == event[i].data.fd) {
        for (;;) {
          struct sockaddr client_addr = {0};
          socklen_t client_addr_len = sizeof(client_addr);
          int client_fd = accept(server_fd, &client_addr, &client_addr_len);
          if (client_fd == -1) {
            perror("Accept");
            break;
          }
          __result =
              getnameinfo(&client_addr, sizeof(client_addr), host_buf,
                          sizeof(host_buf) / sizeof(host_buf[0]), port_buf,
                          sizeof(port_buf) / sizeof(port_buf[0]),
                          NI_NUMERICHOST | NI_NUMERICSERV);

          if (!__result) {
            printf("New connection: host = %s, port = %s\n", host_buf,
                   port_buf);
          }

          __result = set_non_blocking(client_fd);
          if (result == -1) {
            return 0;
          }

          ev.data.fd = client_fd;
          ev.events = EPOLLIN | EPOLLET;
          __result = epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);
          if (__result == -1) {
            perror("epoll_ctl");
            return 0;
          }
        }
        continue;
      } else {
        for (;;) {
          ssize_t result_len = 0;
          char buf[READ_BUF_LEN] = {0};

          result_len =
              read(event[i].data.fd, buf, sizeof(buf) / sizeof(buf[0]));
          if (result_len == -1) {
            if (EAGAIN != errno) {
              perror("Read data");
            }
            break;
          } else if (result_len == 0) {
            struct epoll_event __ev;
            __ev.events = EPOLLIN;
            __ev.data.fd = event[i].data.fd;
            epoll_ctl(epfd, EPOLL_CTL_DEL, event[i].data.fd, &__ev);
            shutdown(event[i].data.fd, SHUT_RDWR);
            printf("%d logout\n", event[i].data.fd);
            break;
          }
          Task *task = new Task(event[i].data.fd, buf, false);
          pool.add_task(task);
        }
      }
    }
  }

  // while (true) {
  //   client_fd =
  //       accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
  //   // cout << "client accept" << endl;
  //   Task *task = new Task(client_fd, true);
  //   pool.add_task(task);
  // }
  close(server_fd);
  return 0;
}