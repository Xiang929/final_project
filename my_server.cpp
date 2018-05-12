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

using namespace std;

#define PORT 8080
#define QUEUE_MAX_COUNT 5
#define BUFF_SIZE 1024

#define SERVER_STRING "Server: hoohackhttpd/0.1.0\r\n"

int main() {
  int server_fd = -1;
  int client_fd = -1;
  int hello_len = -1;
  u_short port = PORT;
  struct sockaddr_in client_addr;
  struct sockaddr_in server_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  char buffer[BUFF_SIZE];
  char recv_buffer[BUFF_SIZE];
  char display[] = "Hello World!";

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    perror("socket");
    exit(0);
  }
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("bind");
    exit(0);
  }
  if (listen(server_fd, QUEUE_MAX_COUNT) < 0) {
    perror("listen");
    exit(0);
  }
  cout << "http server running on port " << PORT << endl;

  while (1) {
    client_fd =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_fd < 0) {
      perror("accept");
      exit(0);
    }
    cout << "accept a client" << endl;
    cout << "client socket fd: " << client_fd << endl;
    hello_len = recv(client_fd, recv_buffer, BUFF_SIZE, 0);
    sprintf(buffer, "HTTP/1.1 200 OK\r\n");
    send(client_fd, buffer, strlen(buffer), 0);
    strcpy(buffer, SERVER_STRING);
    send(client_fd, buffer, strlen(buffer), 0);
    sprintf(buffer, "Content-Type: text/html\r\n");
    send(client_fd, buffer, strlen(buffer), 0);
    strcpy(buffer, "\r\n");
    send(client_fd, buffer, strlen(buffer), 0);
    sprintf(buffer, "Hello World\r\n");
    send(client_fd, buffer, strlen(buffer), 0);
    close(client_fd);
  }
  close(server_fd);
  return 0;
}