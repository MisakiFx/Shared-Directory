#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <sys/epoll.h>
#include <unistd.h>
#include "tcpsocket.hpp"
#define MAX_EPOLL 1024
class Epoll
{
  public:
    
    //构造
    Epoll();

    //初始化
    bool Init();

    //新增套接字
    bool Add(TcpSocket& sock);

    //开始监控
    bool Wait(std::vector<TcpSocket>& list, int timeout = 3000);

    bool Del(TcpSocket& sock);
  private:
    int _epfd;      //epoll模型句柄
};

Epoll::Epoll()
  :_epfd(-1)
{

}

bool Epoll::Init()
{
  _epfd = epoll_create(MAX_EPOLL);
  if(_epfd < 0)
  {
    std::cerr << "create epoll error" << std::endl;
    return false;
  }
  return true;
}

bool Epoll::Add(TcpSocket& sock)
{
  struct epoll_event ev;
  int fd = sock.GetFd();
  ev.events = EPOLLIN;
  ev.data.fd = fd;
  int ret = epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &ev);
  if(ret < 0)
  {
    std::cerr << "append monitor error" << std::endl;
    return false;
  }
  return true;
}
bool Epoll::Wait(std::vector<TcpSocket>& list, int timeout)
{
  struct epoll_event evs[MAX_EPOLL];
  int nfds = epoll_wait(_epfd, evs, MAX_EPOLL, timeout);
  if(nfds < 0)
  {
    std::cerr << "epoll monitor error" << std::endl;
    return false;
  }
  else if(nfds == 0)
  {
    std::cerr << "epoll monitor timeout" << std::endl;
    return false;
  }
  for(int i = 0; i < nfds; i++)
  {
    int fd = evs[i].data.fd;
    TcpSocket sock;
    sock.SetFd(fd);
    list.push_back(sock);
  }
  return true;
}

bool Epoll::Del(TcpSocket& sock)
{
  int fd = sock.GetFd();
  int ret = epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL);
  if(ret < 0)
  {
    std::cerr << "remove monitor error" << std::endl;
    return false;

  }
  return true;

}                                                         

