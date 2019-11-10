//实现Server类，完成服务端整体架构流程
#include <vector>
class Server
{
  public:
    bool Start(int port)
    {
      bool ret = _lst_sock.Init(port);
      if(ret == false)
      {
        return false;
      }
      ret = _epoll.Init();
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
            ThreadTask tt;
            tt.SetTask(list[i], HttpHandler);
            //将任务加入线程池
            _pool.PushTask(tt);
            //在这个套接字中数据未读取完成前不再监听这个套接字
            _epoll.Del(list[i]);
          }
        }
      }
      _lst_sock.Close();
      return true;
    }
    static void HttpHandler(TcpSocket& cli_sock)
    {
      Request req;
      //解析数据
      int status;
      status = req.ParseHeader(cli_sock);
      //解析失败，直接响应错误
      if(status != 200)
      {
        rsp.SendError(status);
        cli_sock.Close();
        return;
      }
      Responce rsp;
      Process(req, rsp);
      rsp.SendResponce(cli_sock);
      cli_sock.Close();
    }
  private:
    TcpSocket _lst_sock;
    ThreadPool _pool;
    Epoll _epoll;
};
