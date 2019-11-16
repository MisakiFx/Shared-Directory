#pragma once
#include <string>
#include <unordered_map>
#include <boost/algorithm/string.hpp>
#include <sstream>
#include "tcpsocket.hpp"
/****************************************************************/
//HTTP请求类，实现解析和存储HTTP请求的功能
class HttpRequest
{
  public:
    //提供请求解析接口
    int RequestParse(TcpSocket& sock);

  private:

    //获取首行+正文信息
    bool RecvHeader(TcpSocket& sock, std::string& header);

    //解析首行信息
    bool FirstLineParse(std::string& line);

    bool PathLegal();

  private:
    std::string _method;                                //请求方法
    std::string _path;                                  //路径信息
    std::unordered_map<std::string, std::string> _param;//参数信息
    std::unordered_map<std::string, std::string> _headers; //头信息
    std::string body;                                   //正文信息
};
/*****************************************************************/

/****************************************************************/
//响应类，用于存储响应信息
class HttpResponse
{
  public:
    //发送错误响应信息
    bool ErrorProcess(TcpSocket& sock);
    
    //发送普通响应信息
    bool NormalProcess(TcpSocket& sock);
    
    //设置_headers接口
    bool SetHeader(const std::string& key, const std::string& value);

  public:
    int _status;                                              //状态信息
    std::string _body;                                         //正文信息
    std::unordered_map<std::string, std::string> _headers;    //头部信息
};
/****************************************************************/
int HttpRequest::RequestParse(TcpSocket& sock)
{
  //1、获取HTTP首行+头部
  std::string header;
  if(RecvHeader(sock, header) == false)
  {
    return 400;
  }
  //2、分析获取信息，得到首行，头部
  std::vector<std::string> header_list;
  boost::split(header_list, header, boost::is_any_of("\r\n"), boost::token_compress_on);
  for(auto e : header_list)
  {
    std::cout << "list = [" << e << "]" << std::endl;
  }
  //3、分析首行信息
  if(FirstLineParse(header_list[0]) == false)
  {
    return 400;
  }
  //4、分析头部信息
  size_t pos = 0;
  for(size_t i = 1; i < header_list.size(); i++)
  {
    pos = header_list[i].find(": ");
    if(pos == std::string::npos)
    {
      std::cerr << "header parse error" << std::endl;
      return 400;
    }
    std::string key = header_list[i].substr(0, pos);
    std::string value = header_list[i].substr(pos + 2);
    _headers[key] = value;
  }
  //5、校验
  std::cout << "method:[" << _method << "]" << std::endl;
  std::cout << "path:[" << _path << "]" << std::endl;
  for(auto e : _param)
  {
    std::cout << e.first << "=" << e.second << std::endl;
  }
  for(auto e : _headers)
  {
    std::cout << e.first << "=" << e.second << std::endl;
  }
  return 200;
}

bool HttpRequest::RecvHeader(TcpSocket& sock, std::string& header)
{
  while(1)
  {
    //1、探测性接收数据
    std::string tmp;
    bool ret = sock.RecvPeek(tmp);
    if(ret == false)
    {
      return false;
    }
    //2、判断是否包含首行+正文
    size_t pos;
    pos = tmp.find("\r\n\r\n", 0);
    //3、判断当前接受的数据长度
    if(pos == std::string::npos && tmp.size() == HEADSIZE)
    {
      return false;
    }
    else if(pos != std::string::npos)
    {
      //4、若包含整个头部，则直接获取头部
      sock.Recv(header, pos);
      sock.Recv(tmp, 4);
      //std::cout << "header:[" << header << "]" << std::endl;
      return true;
    }
  }
}
bool HttpRequest::FirstLineParse(std::string& line)
{
  std::vector<std::string> line_list;
  boost::split(line_list, line, boost::is_any_of(" "), boost::token_compress_on);
  if(line_list.size() != 3)
  {
    std::cerr << "prase first line error" << std::endl;
    return false;
  }
  //获取请求方法
  _method = line_list[0];
  //获取path路径
  size_t pos = line_list[1].find("?", 0);
  if(pos == std::string::npos)
  {
    _path = line_list[1];
  }
  else 
  {
    _path = line_list[1].substr(0, pos);
    std::string query_string = line_list[1].substr(pos + 1);
    //解析参数
    std::vector<std::string> param_list;
    boost::split(param_list, query_string, boost::is_any_of("&"), boost::token_compress_on);
    for(auto e : param_list)
    {
      size_t param_pos = -1;
      param_pos = e.find("=");
      if(param_pos == std::string::npos)
      {
        std::cerr << "path param error" << std::endl;
        return false;
      }
      std::string key = e.substr(0, param_pos);
      std::string value = e.substr(param_pos + 1);
      _param[key] = value;
    }
  }
  return true;
}
bool HttpResponse::ErrorProcess(TcpSocket& sock)
{

  return true;
}
bool HttpResponse::NormalProcess(TcpSocket& sock)
{
  std::string line;
  std::string header;
  std::stringstream tmp;
  tmp << "HTTP/1.1" << " " << _status;
  return true;
}
bool HttpResponse::SetHeader(const std::string& key, const std::string& value)
{
  _headers[key] = value;
  return true;
}
