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
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

const int BUFFSIZE = 4096;
const size_t SSIZE_MAX = 2000000000 - 1;
const string path = "./Resource";

class Task {
 private:
  int client_fd;
  bool client_state;
  char order[BUFFSIZE];

 public:
  Task(){};
  Task(int client_fd, char* str, bool client_state)
      : client_fd(client_fd), client_state(client_state) {
    strcpy(order, str);
  };
  ~Task(){};
  void response(string message, int status);
  void response_file(int size, int status, string content_type);
  void response_post(string filename, string command);
  void response_get(string filename);
  void response_head(string filename);
  void run();

 private:
  string get_content_type(string suffix);
};

void Task::response(string message, int status) {
  stringstream out_message;
  out_message << "HTTP/1.1 " << to_string(status) << " OK\r\n"
              << "Connection: Close\r\n"
              << "Content-length:" << to_string(message.size()) << "\r\n\r\n";
  const char* buffer = out_message.str().c_str();
  write(client_fd, buffer, out_message.str().size());
}

void Task::response_file(int size, int status, string content_type) {
  stringstream out_message;
  out_message << "HTTP/1.1 " << to_string(status) << " OK\r\n"
              << "Connection: Close\r\n"
              << "Content-length:" << to_string(size) << "\r\n"
              << "Content-Type: " << content_type << "\r\n\r\n";
  const char* buffer = out_message.str().c_str();
  cout << buffer << endl;
  write(client_fd, buffer, out_message.str().size());
}

void Task::run() {
  char buffer[BUFFSIZE];
  strcpy(buffer, order);
  int size = 0;
  // while (client_state) {
  // size = read(client_fd, buffer, BUFFSIZE);
  // if (size > 0) {
  int i = 0;
  string method;
  string filename;
  while (buffer[i] != ' ' && buffer[i] != '\0') {
    method += buffer[i++];
  }
  i++;

  while (buffer[i] != ' ' && buffer[i] != '\0' && buffer[i] != '?') {
    filename += buffer[i++];
  }
  cout << method << endl;
  cout << filename << endl;
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
    string command;
    if (pos != -1)
      command = content.substr(pos, content.length() - pos);
    else
      command = "";
    cout << command << endl;
    if (filename[0] == '/' && filename.length() == 1)
      response_post("/index.html", command);
    else
      response_post(filename, command);
  } else {
    stringstream message;
    message << "<html><title>Myhttpd Error</title>"
            << "<body>\r\n"
            << " 501\r\n"
            << " <p>" << method << "Httpd does not implement this method"
            << "<hr><h3>The Tiny Web Server<h3></body>";
    response(message.str(), 501);
  }
  // }
  // else {
  //   client_state = false;
  //   continue;
  // }
  // }
  // sleep(2);
  close(client_fd);
}

void Task::response_get(string filename) {
  bool is_dynamic = false;
  string command;
  string file = path;
  file += filename;
  // cout << filename << endl;
  if (filename[0] == '/' && filename.length() == 1) {
    file += "index.html";
  }
  cout << file << endl;

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

  int filefd = open(file.c_str(), O_RDONLY);
  int pos = file.rfind('.', file.length() - 1);
  string suffix = file.substr(pos + 1, file.length() - pos);
  string content_type = get_content_type(suffix);
  cout << content_type << endl;
  response_file(filestat.st_size, 200, content_type);
  cout << "filesize: " << filestat.st_size << endl;
  // size_t sd = sendfile(client_fd, filefd, NULL, filestat.st_size);
  // cout << "sendsize: " << sd << endl;
  __off64_t offset = 0;
  ssize_t sent;
  for (size_t size_to_send = filestat.st_size; size_to_send > 0;) {
    sent = sendfile64(client_fd, filefd, &offset, size_to_send);
    if (sent <= 0) {
      cout << "sent:" << sent << endl;
      if (sent != 0) perror("sendfile");
      break;
    }
    size_to_send -= sent;
    // cout << "offset: " << offset << endl;
    // cout << "size_to_send: " << size_to_send << endl;
    // cout << "sent:" << sent << endl;
    close(filefd);
  }
}

void Task::response_post(string filename, string command) {
  string file = path;
  file += filename;
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

string Task::get_content_type(string suffix) {
  if (suffix == "asc" || suffix == "txt" || suffix == "text" ||
      suffix == "pot" || suffix == "brf" || suffix == "srt") {
    return "text/plain";
  }
  if (suffix == "jpeg" || suffix == "jpg") {
    return "image/jpeg";
  }
  if (suffix == "html" || suffix == "htm" || suffix == "shtml") {
    return "text/html";
  }
  if (suffix == "ico") {
    return "image/x-icon";
  }

  return "";
}

#endif
