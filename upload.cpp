#include <iostream>
#include <string.h>
#include <sstream>
#include <unistd.h>
#include <vector>
class Boundary
{
  public:
    int64_t _start_addr;
    int64_t _data_len;
    std::string _name;
};
//得到请求中某个头部的信息
bool GetHeader(const std::string& key, std::string& val)
{
  std::string body;
  char* ptr = getenv(key.c_str());
  if(ptr == nullptr)
  {
    return false;
  }
  val = ptr;
  return true;
}

//解析正文获得数据
bool BoundaryParse(std::string& body, std::vector<Boundary>& list)
{
  std::string tmp;
  if(GetHeader("Content-Type", tmp) == false)
  {
    return false;
  }
  std::string contentBoundary = "boundary=";
  size_t pos = tmp.find(contentBoundary);
  if(pos == std::string::npos)
  {
    return false;
  }
  std::string boundary = tmp.substr(pos + contentBoundary.size());
  std::string firstBoundary = "--" + boundary;
  std::string middleBoundary = "\r\n--" + boundary + "\r\n";
  std::string lastBoundary = "\r\n--" + boundary + "--\r\n";
  pos = body.find(firstBoundary);
  Boundary node;
  if(pos != std::string::npos)
  {
    size_t idx = pos;
    pos = body.find("\r\n\r\n", idx);
    if(pos != std::string::npos)
    {
      node._start_addr = pos + 4;
      idx = pos + 4;
      size_t pos1 = body.find(middleBoundary, idx);
      if(pos1 != std::string::npos)
      {
        node._data_len = pos - idx;
      }
      else 
      {
        size_t pos2 = body.find(middleBoundary, idx);
      }
    }
  }
  pos = body.find(middleBoundary, idx);
  if(pos != std::string::npos)
  {
  }
  return true;
}
int main(int argc, char* argv[], char* env[])
{
  for(int i = 0; env[i] != NULL; i++)
  {
    std::cerr << "env[i] === [" << env[i] << "]" << std::endl;
  }
  std::string body;
  char* contentLen = getenv("Content-Length");
  if(contentLen != NULL)
  {
    std::stringstream tmp;
    tmp << contentLen;
    int64_t fsize;
    tmp >> fsize;
    int rlen = 0;
    body.resize(fsize);
    while(rlen < fsize)
    {
      int ret = read(0, &body[0] + rlen, fsize - rlen);
      if(ret <= 0)
      {
        exit(-1);
      }
      rlen += ret;
    }
    std::cerr << "body:[" << body << "]" << std::endl;
  }
  return 0;
}
