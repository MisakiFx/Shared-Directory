#include <string>
#include <unordered_map>
//HTTP请求类
class HttpRequest
{
  public:
    int RequestParse(TcpSocket& sock);
  private:

  private:
    std::string _method;                                //存储首行信息
    std::string _path;                                  //路径信息
    std::unordered_map<std::string, std::string> _param;//参数信息
    std::unordered_map<std::string, std::string> _head; //头信息
    std::string body;                                   //正文信息
};
HttpRequest::HttpRequest()
{

}
