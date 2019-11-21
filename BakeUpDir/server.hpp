//实现Server类，完成服务端整体架构流程
#pragma once
#include <vector>
#include "tcpsocket.hpp"
#include "epollwait.hpp"
#include "threadpool.hpp"
#include <boost/filesystem.hpp>
#include "http.hpp"
#define ROOT "./BakeUpDir"
/***************************************************/
//Server类，负责实现服务端连接框架
//利用epoll模型和线程池实现
class Server
{
  public:

    //服务器开始工作
    bool Start(int port);

    //线程任务函数
    static void ThreadHandler(int sockfd);

    //具体处理请求方法，将请求req处理转换为响应rsp
    static bool HttpProcess(const HttpRequest& req, HttpResponse& rsp);

    //目录列表请求
    static bool ListShow(const std::string& path, std::string& body);
  private:

    TcpSocket _lst_sock;  //监听套接字
    ThreadPool _pool;     //线程池
    Epoll _epoll;         //多路转接IO，epoll模型
};
/****************************************************/

bool Server::Start(int port)
{
  bool ret = _lst_sock.SocketInit(port);
  if(ret == false)
  {
    return false;
  }
  ret = _epoll.Init();
  if(ret == false)
  {
    return false;
  }
  ret = _pool.PoolInit();
  if(ret == false)
  {
    return false;
  }
  _epoll.Add(_lst_sock);
  while(1)
  {
    std::vector<TcpSocket> list;
    ret = _epoll.Wait(list);
    if(ret == false)
    {
      continue;
    }
    for(int i = 0; i < list.size(); i++)
    {
      //监听套接字
      if(list[i].GetFd() == _lst_sock.GetFd())
      {
        TcpSocket cli_sock;
        ret = _lst_sock.Accept(cli_sock);
        if(ret == false)
        {
          continue;
        }
        cli_sock.SetNonBlock();
        _epoll.Add(cli_sock);
      }
      //通信套接字
      else 
      {
        ThreadTask tt(list[i].GetFd(), ThreadHandler);
        //将任务加入线程池
        _pool.TaskPush(tt);
        //在这个套接字中数据未读取完成前不再监听这个套接字
        _epoll.Del(list[i]);
      }
    }
  }
  _lst_sock.Close();
  return true;
}

void Server::ThreadHandler(int sockfd)
{
  TcpSocket sock;
  sock.SetFd(sockfd);
  HttpRequest req;
  HttpResponse rsp;
  //解析数据
  int status = req.RequestParse(sock);
  //解析失败，直接响应错误
  if(status != 200)
  {
    rsp._status = status;
    rsp.ErrorProcess(sock);
    sock.Close();
    return;
  }
  //根据请求进行处理，处理结果放到rsp中
  HttpProcess(req, rsp);
  //处理结果返回给客户端
  rsp.NormalProcess(sock);
  //短链接，处理完就关闭
  sock.Close();
}

bool Server::HttpProcess(const HttpRequest& req, HttpResponse& rsp)
{
  //请求分析，执行请求，组织响应信息
  //文件上传请求
  std::string realPath = ROOT + req._path;
  if(!boost::filesystem::exists(realPath))
  {
    rsp._status = 404;
    return false;
  }
  if(req._method == "GET" && req._param.size() != 0 || req._method == "POST")
  {


  }
  //文件下载或者目录列表请求
  else 
  {
    //目录列表请求
    if(boost::filesystem::is_directory(realPath))
    {
      ListShow(realPath, rsp._body);
      rsp.SetHeader("Content-Type", "text/html");
    }
    //文件下载请求
    else 
    {

    }
  }
  return true;
}

bool Server::ListShow(const std::string& path, std::string& body)
{
  std::stringstream ss;
  ss << "<html><head><style>";
  ss << "*{margin : 0;}";
  ss << ".main-window {height : 100%;width : 80%;margin: 0 auto;}";
  ss << ".upload {position : relative;height : 20%;width : 100%;background-color : rgb(253, 224, 255)}";
  ss << ".listshow {position : relative;height : 80%;width : 100%;background : rgb(212, 241, 241)}";
  ss << "</style></head>";
  ss << "<body><div class='main-window'>";
  ss << "<div class='upload'></div><hr />";
  ss << "<div class='listshow'><ol>";
  
  //组织结点信息
  boost::filesystem::directory_iterator begin(path);
  boost::filesystem::directory_iterator end;
  for(; begin != end; ++begin)
  {
    std::string name = begin->path().filename().string();
    int64_t mtime = boost::filesystem::last_write_time(begin->path());
    int64_t size = boost::filesystem::file_size(begin->path());
    std::cout << "name:[" << name << "]" << std::endl;
    std::cout << "mtime:[" << mtime << "]" << std::endl;
    std::cout << "size:[" << size << "]" << std::endl;
  }
  ss << "<li><strong><a href='#'>a.txt</a><br /></strong>";
  ss << "<small>modified:Thu Nov 21 04:45:51 PST 2019";
  ss << "<br/> filetype:application-ostream 16kb";
  ss << "</small></li>";

  ss << "</ol></div><hr /></div></body></html>";
  body = ss.str();
  return true;
}
