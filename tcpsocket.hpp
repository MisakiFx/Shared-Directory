//封装套接字管理类
#include <iostream>
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#define HEADSIZE 8192
/****************************************************/
//套接字管理类，负责套接字的管理
class TcpSocket
{

  public:
    //构造
    TcpSocket();

    //初始化监听套接字，绑定地址信息，开始监听
    bool SocketInit(int port);

    //获取套接字
    int GetFd();

    //设置套接字
    void SetFd(int fd);

    //设置非阻塞
    void SetNonoBlock();

    //获取通信套接字，通过sock返回，并且设置其为非阻塞
    bool Accept(TcpSocket& sock);

    //试探性读取数据，从接收缓冲区读取一部分数据但并不将其移出缓冲区
    bool RecvPeek(std::string& buf);

    //获取指定长度的数据，如果缓冲区中数据不足则阻塞一直读取
    bool Recv(std::string& buf, int len);

  private:
    int _sockfd;
};

/*****************************************************/

TcpSocket::TcpSocket()
  :_sockfd(-1)
{

}
bool TcpSocket::SocketInit(int port)
{
  //创建套接字
  _sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(_sockfd < 0)
  {
    std::cerr << "socket error" << std::endl;
    return false;
  }
  //绑定地址信息
  int opt = 1;
  setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, (void*)&opt, sizeof(opt));
  struct sockaddr_in addr;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);
  addr.sin_family = AF_INET;
  socklen_t len = sizeof(addr);
  int ret = bind(_sockfd, (struct sockaddr*)&addr, len);
  //绑定失败
  if(ret < 0)
  {
    std::cerr << "bind error" << std::endl;
    close(_sockfd);
    return false;
  }
  //开始监听
  ret = listen(_sockfd, 10);
  //监听失败
  if(ret < 0)
  {
    std::cerr << "listen error" << std::endl;
    close(_sockfd);
    return false;
  }
  return true;
}

int TcpSocket::GetFd()
{
  return _sockfd;
}

void TcpSocket::SetFd(int fd)
{
  _sockfd = fd;
}

void TcpSocket::SetNonoBlock()
{
  int nowFlag = fcntl(_sockfd, F_GETFL, 0);
  fcntl(_sockfd, F_SETFL, nowFlag | O_NONBLOCK);
}

bool TcpSocket::Accept(TcpSocket& sock)
{
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  int clientSock = accept(_sockfd, (struct sockaddr*)&addr, &len);
  if(clientSock < 0)
  {
    std::cerr << "accept error" << std::endl;
    return false;
  }
  sock.SetFd(clientSock);
  sock.SetNonoBlock();
  return true;
}

bool TcpSocket::RecvPeek(std::string& buf)
{
  buf.clear();
  char tmp[HEADSIZE] = {0};
  int ret = recv(_sockfd, tmp, HEADSIZE, MSG_PEEK);
  //读取出错或者连接断开
  if(ret <= 0)
  {
    //此时缓冲区中没有数据，非阻塞EAGAIN表示缓冲区中所有数据已经读完
    if(errno == EAGAIN)
    {
      return true;
    }
    std::cerr << "recv error, conect maybe lost!" << std::endl;
    return false;
  }
  buf.assign(tmp, ret);
  return true;
}
bool TcpSocket::Recv(std::string& buf, int len)
{
  buf.resize(len);
  int rlen = 0;
  while(rlen < len)
  {
    int ret = recv(_sockfd, &buf[0] + rlen, len - rlen, 0);
    if(ret <= 0)
    {
      if(errno == EAGAIN)
      {
        usleep(1000);
        continue;
      }
      std::cerr << "recv error, connect lost" << std::endl;
      return false;
    }
    rlen += ret;
  }
  return true;
}
