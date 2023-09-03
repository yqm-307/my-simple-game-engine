#pragma once
#include <functional>
#include "util/network/Connection.hpp"
#include "util/typedef/NamespaceType.hpp"
#include "util/network/IOThread.hpp"
#include <bbt/buffer/Buffer.hpp>
#include <bbt/timer/Clock.hpp>

/**
 * @brief libevent实现
 * 
 */
namespace game::util::network
{
#define NET_HANDLER_ENTRY(errcode_key, capture, lambda_body) {errcode_key, [capture](const bbt::buffer::Buffer& buffer){lambda_body(buffer);}}

static const int evConnection_Timeout_MS = 3000;

void OnRecvCallback(int sockfd, short events, void* args);
void OnHeartBeatCallback(evutil_socket_t sockfd, short events, void* args);

namespace ev
{

/* libevent 到 C++闭包对象中间层，希望使用函数对象统一io操作 */
typedef std::function<void(evutil_socket_t, short, void*)> IOCommCallback;

struct evArgs
{
    IOCommCallback  do_io_callback{nullptr};
};



class evIOThread;
class evConnection;
class evConnMgr;
SmartPtrTypeDef(evConnection);

/**
 * @brief 可靠的双向连接，基于Tcp
 */
class evConnection:
    public Connection,
    public std::enable_shared_from_this<evConnection>
{
    friend void game::util::network::OnRecvCallback(int sockfd, short events, void* args);
    friend class evConnMgr;

    const std::map<int, 
        std::function<void(const bbt::buffer::Buffer&)>> m_errcode_handler
    {
        // 使用宏减少一些代码量，而且显而易见的地方使用宏感觉没啥问题
        // {util::errcode::network::err::Recv_Success, 
        // [this](const bbt::buffer::Buffer& buffer, const util::errcode::ErrCode& err){ NetHandler_RecvData(buffer, err); }},
        NET_HANDLER_ENTRY(util::errcode::network::err::Recv_Success,    this, NetHandler_RecvData),
        NET_HANDLER_ENTRY(util::errcode::network::err::Recv_Eof,        this, NetHandler_ConnClosed),
        NET_HANDLER_ENTRY(util::errcode::network::err::Recv_TryAgain,   this, NetHandler_TryAgain),
        NET_HANDLER_ENTRY(util::errcode::network::err::Recv_Other_Err,  this, NetHandler_OtherErr),
    };
    typedef std::function<void(evConnectionSPtr)>   OnDestoryCallback;
    typedef std::function<bool(evConnectionSPtr)>   OnTimeOutCallback;
public:
    /**
     * @deprecated 不允许被随意使用
     * @brief 构造一个新连接
     * 
     * @param thread 运行在的io线程上
     * @param newfd 套接字
     * @param peer_ip 对端socket地址
     * @param local_ip 本地socket地址
     */
    evConnection(IOThread* thread, int newfd, Address peer_ip, Address local_ip);
    virtual ~evConnection();

    virtual bool IsClosed() override;
    virtual void Close() override;
    virtual size_t Send(const char* buffer, size_t len) override;
    virtual size_t Recv(const char* buffer, size_t len) override;

    virtual const Address& GetPeerIPAddress() const override;
    virtual const Address& GetLocalIPAddress() const override;
    std::pair<char*,size_t> GetRecvBuffer();

    void OnHeartBeat();
protected:

    
    void Init();
    void Destroy();

    void SetOnDestory(const OnDestoryCallback& cb);
    /* IO事件初始化 */
    void InitEvent();
    void InitEventArgs();
private:
    //----------------- Read Only -------------------//
    /* 获取当前连接所在的IO线程 */
    evIOThread* GetIOThread();
    /* 获取当前连接的套接字 */
    evutil_socket_t GetSocket();
    /* 获取当前连接上次心跳包的时间 */
    bbt::timer::clock::Timestamp<bbt::timer::clock::ms>
        GetPrevHeartBeatTimestamp();
private:
    /* read事件派发函数，read事件有很多可能eof、refused等，所以需要通过此函数派发到对应的事件处理函数 */
    void OnRecvEventDispatch(const bbt::buffer::Buffer& buffer, const util::errcode::ErrCode& err);
    //----------- IO Dispatcher  -------------//
    void OnRecv(evutil_socket_t fd, short events, void* args);

    //----------- NetWork Handler -------------//
    void NetHandler_RecvData(const bbt::buffer::Buffer& buffer);
    void NetHandler_ConnClosed(const bbt::buffer::Buffer& buffer);
    void NetHandler_TryAgain(const bbt::buffer::Buffer& buffer);
    void NetHandler_OtherErr(const bbt::buffer::Buffer& buffer);

    //----------- timeout Handler -------------//
    // 心跳的实现修改，由上层解析完心跳协议后，调用到Conenction更新心跳时间
    // void TimeOutHandler(const util::errcode::ErrCode& err);
private:

    IOThread*   m_io_thread;
    int         m_sockfd;
    ConnStatus  m_status;
    event*      m_recv_event;   // 接收事件
    event*      m_timeout_event;// 超时事件
    // event*      m_ev_send;
    util::network::Address  m_local_addr;
    util::network::Address  m_peer_addr;

    char        m_recv_buffer[4096];    // socket 接收缓存，后续可以接入配置中

    /* 在 evConnection 连接被释放时调用。具体由evConnMgr实现 */
    OnDestoryCallback   m_ondestory_cb;
    OnTimeOutCallback   m_is_timeout_cb;

    evArgs m_onrecv_args;

    /** 
     * 上次接收到心跳包的时间戳，默认为连接建立的时间。 
     * 
     * @yqm-307 关于线程安全，这个值在创建时是被认为只在多线程读取
     * ，不会多线程进行写操作。并且心跳对于线程之间的低概率
     * 延迟是不关心的，误差及其小。
     */
    bbt::timer::clock::Timestamp<bbt::timer::clock::ms>    
                m_prev_heart_beat_time;
};
}

}

#ifdef NET_HANDLER_ENTRY
#undef NET_HANDLER_ENTRY
#endif