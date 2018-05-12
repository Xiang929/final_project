#ifndef _HTTPGETMETHOD_HPP_
#define _HTTPGETMETHOD_HPP_

#include <iostream>

using namespace std;

class GetMethod {
 public:
  GetMethod();
  ~GetMethod();
  unsigned int httpGet(string host, string path, string get_content);
  string get_requests_return();
  string get_main_text();
  unsigned int get_status_code();

 private:
  unsigned int status_code;
  string request_return;
  string main_text;
  string HttpSocket(string host, string request_str);
  void anlyzeData();
};
#endif