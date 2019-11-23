//实现Server类，完成服务端整体架构流程
#pragma once
#include <vector>
#include <stdlib.h>
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

    //利用外部程序解析上传文件时的正文数据，拿到文件数据
    static bool CGIProcess(const HttpRequest& req, HttpResponse& rsp);
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
    std::cerr << "no file" << std::endl;
    return false;
  }
  //文件上传请求
  if(req._method == "GET" && req._param.size() != 0 || req._method == "POST")
  {
    //for(auto e : req._headers)
    //{
    //  std::cout << e.first << ": " << e.second << std::endl;
    //}
    //std::cout << "body:[" << req._body << "]" << std::endl;
    CGIProcess(req, rsp);
  }
  //文件下载或者目录列表请求
  else 
  {
    //目录列表请求
    if(boost::filesystem::is_directory(realPath))
    {
      if(ListShow(realPath, rsp._body) == false)
      {
        std::cerr << "wrong file path" << std::endl;
        return false;
      }
      rsp.SetHeader("Content-Type", "text/html");
    }
    //文件下载请求
    else 
    {

    }
  }
  rsp._status = 200;
  return true;
}

bool Server::CGIProcess(const HttpRequest& req, HttpResponse& rsp)
{
  //创建两个管道负责进程间通讯
  //pipe_in负责父进程从中读取子进程数据
  //pipe_out负责父进程向子进程发送数据
  int pipe_in[2], pipe_out[2];
  if(pipe(pipe_in) < 0 || pipe(pipe_out) < 0)
  {
    std::cerr << "create pipe error" << std::endl;
    return false;
  }
  //利用子进程解析正文
  int pid = fork();
  if(pid < 0)
  {
    return false;
  }
  else if(pid == 0)
  {
    close(pipe_in[0]);
    close(pipe_out[1]);
    //为了让子进程在进程替换后依然可以找到管道，
    //直接用重定向将标准输入重定向到pipe_out管道读端
    //将标准输出重定向到pipe_in的写端
    dup2(pipe_out[0], 0);
    dup2(pipe_in[1], 1);
    
    //为了让正文数据和头部信息不混乱，避免再次解析，我们使用两种手段分别传输两个数据
    //用环境变量传输req的头部信息
    //用管道传输req的正文信息
    //设置环境变量
    setenv("METHOD", req._method.c_str(), 1);
    setenv("PATH", req._path.c_str(), 1);
    for(auto e : req._headers)
    {
      setenv(e.first.c_str(), e.second.c_str(), 1);
    }

    std::string realPath = ROOT + req._path;
    execl(realPath.c_str(), realPath.c_str(), nullptr);
    exit(0);
  }
  close(pipe_in[1]);
  close(pipe_out[0]);
  //通过管道写入正文
  write(pipe_out[1], &req._body[0], req._body.size());
  while(1)
  {
    char buf[1024] = {0};
    int ret = read(pipe_in[0], buf, 1024);
    //没有人向管道里写数据
    if(ret == 0)
    {
      break;
    }
    buf[ret] = '\0';
    rsp._body += buf;
  }
  close(pipe_in[0]);
  close(pipe_out[1]);
  return true;
}

bool Server::ListShow(const std::string& path, std::string& body)
{
  std::string root = ROOT;
  size_t pos = path.find(root);
  if(pos == std::string::npos)
  {
    return false;
  }
  std::string jumpPath = path.substr(root.size());
  std::stringstream ss;
  ss << "<html><head><style>";
  ss << "*{margin : 0;}";
  ss << ".main-window {height : 100%;width : 80%;margin: 0 auto;}";
  ss << ".upload {position : relative;height : 20%;width : 100%;background-color : rgb(253, 224, 255); text-align : center;}";
  ss << ".listshow {position : relative;height : 80%;width : 100%;background : rgb(212, 241, 241)}";
  ss << "</style></head>";
  ss << "<body><div class='main-window'>";
  ss << "<div class='upload'>";
  ss << "<form action='/upload' method='POST' enctype='multipart/form-data'>";
  ss << "<div class='upload-btn'>";
  ss << "<input type='file' name='fileupload'>";
  ss << "<input type='submit' name='submit'>";
  ss << "</div></form>";
  ss << "</div><hr />";
  ss << "<div class='listshow'><ol>";
  
  //组织结点信息
  boost::filesystem::directory_iterator begin(path);
  boost::filesystem::directory_iterator end;
  for(; begin != end; ++begin)
  {
    std::string name = begin->path().filename().string();
    std::string uri = jumpPath + name;
    //如果目录中还是目录
    if(boost::filesystem::is_directory(begin->path()))
    {
      ss << "<li><strong><a href='" << uri;
      ss << "'>" << name << "/" << "</a><br /></strong>";
      ss << "<small>";
      ss << "filetype:directory";
      ss << "</small></li>";
    }
    //普通文件处理
    else 
    {
      int64_t mtime = boost::filesystem::last_write_time(begin->path());
      int64_t size = boost::filesystem::file_size(begin->path());
      //std::cout << "name:[" << name << "]" << std::endl;
      //std::cout << "mtime:[" << mtime << "]" << std::endl;
      //std::cout << "size:[" << size << "]" << std::endl;
      ss << "<li><strong><a href='" << uri;
      ss << "'>" << name << "</a><br /></strong>";
      ss << "<small>modified:" << mtime;
      ss << "<br/> filetype:application-oct-ostream " << size / 1024 << "kb";
      ss << "</small></li>";
    }
  }

  ss << "</ol></div><hr /></div></body></html>";
  body = ss.str();
  return true;
}
