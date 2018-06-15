#ifndef _TASK_HPP_
#define _TASK_HPP_

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

const int BUFFSIZE = 4096;

class Task {
 private:
  int client_fd;
  bool client_state;

 public:
  Task(){};
  Task(int client_fd, bool client_state)
      : client_fd(client_fd), client_state(client_state){};
  ~Task(){};
  void response(string message, int status);
  void response_file(int size, int status);
  void response_post(string filename, string command);
  void response_get(string filename);
  void response_head(string filename);
  void run();
};

void Task::response(string message, int status) {
  char buffer[BUFFSIZE];
  sprintf(buffer,
          "HTTP/1.1 %d OK\r\nConnection: Close\r\n"
          "content-length:%zu\r\n\r\n",
          status, message.size());
  sprintf(buffer, "%s%s", buffer, message.c_str());
  write(client_fd, buffer, strlen(buffer));
}

void Task::response_file(int size, int status) {
  char buffer[BUFFSIZE];
  sprintf(buffer,
          "HTTP/1.1 %d OK\r\nConnection: Close\r\ncontent-length:%d\r\n\r\n",
          status, size);
  write(client_fd, buffer, strlen(buffer));
}

void Task::run() {
  char buffer[BUFFSIZE];
  int size = 0;
  while (client_state) {
    size = read(client_fd, buffer, BUFFSIZE);
    if (size > 0) {
      string method;
      string filename;
      int i = 0;
      while (buffer[i] != ' ' && buffer[i] != '\0') {
        method += buffer[i++];
      }
      i++;

      while (buffer[i] != ' ' && buffer[i] != '\0') {
        filename += buffer[i++];
      }
      // cout << method << endl;
      // cout << filename << endl;
      if (method == "GET") {
        response_get(filename);
      } else if (method == "HEAD") {
        response_head(filename);
      } else if (method == "POST") {
        string content;
        int pos = content.find("username=");
        while (pos == -1) {
          content.clear();
          while (buffer[i] != ' ' && buffer[i] != '\0') {
            content += buffer[i++];
          }
          i++;
          pos = content.find("username=");
        }
        string command = content.substr(pos, content.length() - pos);
        cout << command << endl;
        response_post("/index.html", command);
      } else {
        client_state = false;
        continue;
      }
    } else {
      client_state = false;
      continue;
    }
  }
  sleep(2);
  close(client_fd);
}

void Task::response_get(string filename) {
  int i = 0;
  bool is_dynamic = false;
  string command;
  string file = filename;
  file.insert(0, ".");
  int pos = filename.find('?', 0);
  if (pos != -1) {
    command = filename.substr(pos + 1, filename.length() - pos);

    file = filename.substr(0, pos);
    is_dynamic = true;
  }
  // cout << filename << endl;
  if (filename[0] == '/' && filename.length() == 1) {
    file += "index.html";
  }
  // cout << file << endl;

  struct stat filestat;
  int ret = stat(file.c_str(), &filestat);
  // cout << ret << endl;
  if (ret < 0 || S_ISDIR(filestat.st_mode)) {
    string message;
    message += "<html><title>Myhttpd Error</title>";
    message += "<body>\r\n";
    message += " 404\r\n";
    message += " <p>GET: Can't find the file";
    message += " <hr><h3>My Web Server<h3></body>";
    response(message, 404);
    return;
  }

  if (is_dynamic) {
    if (fork() == 0) {
      // Redirect output to the client
      dup2(client_fd, STDOUT_FILENO);
      // Perform subroutines
      execl(file.c_str(), command.c_str(), NULL);
    }
    wait(NULL);
  } else {
    int filefd = open(file.c_str(), O_RDONLY);
    response_file(filestat.st_size, 200);
    // cout << filestat.st_size << endl;
    sendfile(client_fd, filefd, 0, filestat.st_size);
    close(filefd);
  }
}

void Task::response_post(string filename, string command) {
  string file = filename;
  file.insert(0, ".");
  cout << file << endl;
  struct stat filestat;
  int ret = stat(file.c_str(), &filestat);
  if (ret < 0 || S_ISDIR(filestat.st_mode)) {
    string message;
    message += "<html><title>Myhttpd Error</title>";
    message += "<body>\r\n";
    message += " 404\r\n";
    message += " <p>GET: Can't find the file";
    message += " <hr><h3>My Web Server<h3></body>";
    response(message, 404);
    return;
  }
  response_get(filename);
  if (fork() == 0) {
    // Redirect output to the client
    dup2(client_fd, STDOUT_FILENO);
    // Perform subroutines
    execl(file.c_str(), command.c_str(), NULL);
  }
  wait(NULL);
}

void Task::response_head(string filename) {}

#endif
