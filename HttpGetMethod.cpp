#include "HttpGetMethod.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <string>

using namespace std;

GetMethod::GetMethod() : status_code(0), request_return(""), main_text("") {}

GetMethod::~GetMethod() {}

unsigned int GetMethod::httpGet(std::string host, std::string path,
                                std::string get_content) {
  stringstream request_str;
}
