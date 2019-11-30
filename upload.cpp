#include <iostream>
#include <string.h>
#include <sstream>
#include <unistd.h>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#define ROOT "./BakeUpDir/"
class Boundary
{
  public:
    int64_t _start_addr;
    int64_t _data_len;
    std::string _name;
    std::string _fileName;
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
//头部信息解析
bool headerParse(const std::string& header, Boundary& file)
{
  std::vector<std::string> list;
  boost::split(list, header, boost::is_any_of("\r\n"), boost::token_compress_on);
  for(size_t i = 0; i < list.size(); i++)
  {
    //std::cerr << "-----[" << list[i] << "]" << std::endl;
    std::string sep = ": ";
    size_t pos = list[i].find(sep);
    if(pos == std::string::npos)
    {
      std::cerr << "find : error" << std::endl;
      return false;
    }
    std::string key = list[i].substr(0, pos);
    std::string value = list[i].substr(pos + sep.size());
    if(key != "Content-Disposition")
    {
      std::cerr << "can not find disposition" << std::endl;
      continue;
    }
    std::string name_filed = "fileupload";
    std::string filename_sep = "filename=\"";
    //不包含文件名信息，继续查找
    pos = value.find(name_filed);
    if(pos == std::string::npos)
    {
      std::cerr << "have no fileupload filed" << std::endl;
      continue;
    }
    pos = value.find(filename_sep);
    if(pos == std::string::npos)
    {
      std::cerr << "have no filename" << std::endl;
      return false;
    }
    pos += filename_sep.size();
    size_t next_pos = value.find("\"", pos);
    if(next_pos == std::string::npos)
    {
      std::cerr << "have no \"" << std::endl;
      return false;
    }
    file._fileName = value.substr(pos, next_pos - pos);
    file._name = "fileupload";
  }
  return true;
}

//解析正文获得数据
bool BoundaryParse(const std::string& body, std::vector<Boundary>& list)
{
  std::string tmp;
  if(GetHeader("Content-Type", tmp) == false)
  {
    return false;
  }
  std::string content_boundary = "boundary=";
  size_t pos = tmp.find(content_boundary);
  if(pos == std::string::npos)
  {
    return false;
  }
  //获得头信息中的boundary信息
  std::string boundary = tmp.substr(pos + content_boundary.size());
  std::string dash = "--";
  std::string craf = "\r\n";
  std::string tail = "\r\n\r\n";
  std::string first_boundary = dash + boundary + craf;
  std::string middle_boundary = craf + dash + boundary;
  size_t first_pos, next_pos;
  first_pos= body.find(first_boundary);
  if(first_pos != 0)
  {
    std::cerr << "first boundary error" << std::endl;
    return false;
  }
  first_pos += first_boundary.size();
  while(first_pos < body.size())
  {
    next_pos = body.find(tail, first_pos);//头部结
    if (next_pos == std::string::npos)
    {
      return false;
    }
    std::string header = body.substr(first_pos, next_pos - first_pos);
    first_pos = next_pos + tail.size();//数据起始地址
    next_pos = body.find(middle_boundary, first_pos);//数据结束位置
    if(next_pos == std::string::npos)
    {
      return false;
    }
    int64_t offset = first_pos;
    int64_t length = next_pos - first_pos;
    next_pos += middle_boundary.size();
    first_pos = body.find(craf, next_pos);
    //std::cerr << "**body:[" << body.substr(offset, length) << "]" << std::endl;
    if(first_pos == std::string::npos)
    {
      return false;
    }
    first_pos += craf.size();
    Boundary file;
    file._data_len = length;
    file._start_addr = offset;
    
    //解析头部
    if(headerParse(header, file) == false)
    {
      std::cerr << "header parse error" << std::endl;
      return false;
    }
    list.push_back(file);
  }
  std::cerr << "parse boundary over" << std::endl;
  return true;
}

//保存文件数据
bool StorageFile(std::string& body, std::vector<Boundary>& list)
{
  for(size_t i = 0; i < list.size(); i++)
  {
    if(list[i]._name != "fileupload")
    {
      continue;
    }
    std::string realpath = ROOT + list[i]._fileName;
    std::ofstream file(realpath);
    //文件打开失败
    if(!file.is_open())
    {
      std::cerr << "open file " << realpath << " error" << std::endl;
      return false;
    }
    //std::cerr << "--body:[" << body.substr(list[i]._start_addr, list[i]._data_len) << std::endl;
    file.write(&body[list[i]._start_addr], list[i]._data_len);
    //写入失败
    if(!file.good())
    {
      std::cout << "write file error" << std::endl;
      file.close();
      return false;
    }
    file.close();
  }
  return true;
}

int main(int argc, char* argv[], char* env[])
{
  std::string body;
  char* content_len = getenv("Content-Length");
  std::string err = "<html>Failed</html>";
  std::string success = "<html>Success</html>";
  if(content_len != NULL)
  {
    //将正文数据从主进程读取过来
    std::stringstream tmp;
    tmp << content_len;
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

    //开始解析正文获得文件数据和文件名
    //std::cerr << "body:[" << body << "]" << std::endl;
    std::vector<Boundary> list;
    bool ret = BoundaryParse(body, list);
    if(ret == false)
    {
      std::cerr << "boundary parse error" << std::endl;
      std::cout << err;
      return -1;
    }
    for(auto i : list)
    {
      std::cerr << "name " << i._name << std::endl;
      std::cerr << "filename " << i._fileName << std::endl;
    }

    //存储文件数据
    StorageFile(body, list);
    if(ret == false)
    {
      std::cerr << "storge error" << std::endl;
      std::cout << err;
      return -1;
    }
    std::cout << success;
    return 0;
  }
  std::cout << err;
  return 0;
}
