//实现Server类，完成服务端整体架构流程
#pragma once
#include <vector>
#include "tcpsocket.hpp"
#include "epollwait.hpp"
#include "threadpool.hpp"
#include "http.hpp"
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
    static bool HttpProcess(HttpRequest& req, HttpResponse& rsp);
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

bool Server::HttpProcess(HttpRequest& req, HttpResponse& rsp)
{
  //请求分析，执行请求，组织响应信息
  //文件上传请求
  if(req._method == "GET" && req._param.size() != 0 || req._method == "POST")
  {
    
  }
  else //文件下载或者目录列表请求
  {

  }
  return true;
}
