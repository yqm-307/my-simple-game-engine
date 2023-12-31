#include "share/ecs/network/entity/Network.hpp"
#include "util/network/libevent/evIOCallbacks.hpp"
#include "share/ecs/Define.hpp"

namespace share::ecs::entity::network
{

struct IOThreadInfo
{
    std::string info;
    int tid;
};

void Network::OnFixUpdate(int index)
{
    auto& thread = m_io_threads[index];
    if(thread == nullptr) {
        return;
    }

    // GAME_BASE_LOG_DEBUG("[OnFixUpdate] thread=[%d] status=[%d]", thread->GetTid(), thread->IsRunning());
}

void Network::RegistFixUpdate(int index)
{
    auto weak_this = weak_from_this();
    auto event = util::network::ev::evEvent::Create([weak_this, index](evutil_socket_t fd, short event, void* args){
        auto shared_this = std::static_pointer_cast<Network>(weak_this.lock());
        if(shared_this == nullptr){
            GAME_EXT1_LOG_ERROR("io thread fix update function, exec failed! sockfd=%d event=%d", fd, event);
            return;
        }

        shared_this->OnFixUpdate(index);
    }, -1, EV_PERSIST, 50);

    auto err = m_io_threads[index]->RegisterEventSafe(event);
    if(err < 0) {
        GAME_EXT1_LOG_ERROR("thread regist evEvent failed! eventid=%d", event->GetEventID());
    }
}

Network::Network(const std::string& ip, short port)
    :engine::ecs::GameObject(share::ecs::EM_ENTITY_TYPE_GAMESERVER_NETWORK),
    m_acceptor(ip, port),
    m_io_thread_num(3)
{
    Init();
    GAME_BASE_LOG_INFO("Network init success!");
}

Network::~Network() 
{
    Destory();
    GAME_BASE_LOG_INFO("Network exit success!");
}

void Network::Init()
{
    /* 主线程 + accept线程 + io线程 */
    m_thread_latch = new bbt::thread::lock::CountDownLatch(m_io_thread_num + 1);

    /* IO线程和libevent初始化，关于初始化顺序，防止创建新线程立即关闭。所以先初始化每个线程的event_base */
    for(int i = 0; i < m_io_thread_num; ++i)
    {
        /* 初始化libevent */
        auto ev_base = OnCreateEventBase();
        Assert(ev_base != nullptr);

        auto io_thread = std::make_shared<util::network::ev::evIOThread>();
        /* 初始化io线程的回调 */
        m_io_threads.push_back(io_thread);
        if(i == 0)
            io_thread->SetWorkFunc([=](){ AcceptWork(i, io_thread); });
        else        
            io_thread->SetWorkFunc([=](){ IOWork(i); });

        io_thread->SetEventBase(ev_base);
    }

    // 设置 loadblance callback    
    m_acceptor.SetLoadBlance([this](){
        return NewConnLoadBlance();
    });
}

void Network::Destory()
{
    delete m_thread_latch;
    m_thread_latch = nullptr;

    /* 同步等待线程结束 */
    SyncStopAllThread();
    
    /* 释放内存 */
    for (int i = 0; i < m_io_thread_num; i++)
    {
        auto thread = m_io_threads[i]; 
        thread = nullptr;
        OnDestoryEventBase(m_ev_bases[i]);
        m_ev_bases[i] = nullptr;
    }
}

bool Network::SyncStart()
{
    if(m_status != emNetworkRunStatus::Default)
        return false;

    m_status = emNetworkRunStatus::Starting;

    for(int i = 0; i < m_io_thread_num; ++i)
    {
        m_io_threads[i]->Start();
    }
    WaitForOtherIOThreadStart();

    m_status = emNetworkRunStatus::Active;
    return true;
}

void Network::SyncStop()
{
    SyncStopAllThread();
}

void Network::SyncStopAllThread()
{
    if (m_status != emNetworkRunStatus::Active)
        return;
    
    for (int i = 0; i < m_io_thread_num; i++)
    {
        if (!m_io_threads[i]->IsRunning())
            continue;

        m_io_threads[i]->Stop();
    }

    m_status = emNetworkRunStatus::Stopping;
}

bool Network::IsStoped()
{
    return (m_status == emNetworkRunStatus::Inactive);
}

void Network::IOWork(int index)
{
    WaitForOtherIOThreadStart();
    auto ev_base = m_ev_bases[index];
    GAME_BASE_LOG_INFO("IO thread start!");
    RegistFixUpdate(index);

    int error = event_base_loop(ev_base, EVLOOP_NO_EXIT_ON_EMPTY);
    AssertWithInfo(error == 0, "libevent error!");
}

void Network::AcceptWork(int index, evIOThreadSPtr this_thread)
{
    /* 等待所有IO线程初始化完毕 */
    WaitForOtherIOThreadStart();
    auto ev_base = m_ev_bases[index];

    GAME_BASE_LOG_INFO("Accept thread start!");
    m_acceptor.Start(this_thread);
    RegistFixUpdate(index);

    int error = event_base_loop(ev_base, EVLOOP_NO_EXIT_ON_EMPTY);
    AssertWithInfo(error == 0, "libevent error!");
}

void Network::WaitForOtherIOThreadStart()
{
    m_thread_latch->down();
    m_thread_latch->wait();
}

void Network::OnUpdate()
{
    if(!m_is_in_loop)
        return;
    
    SyncStopAllThread();
}

void Network::SetOnConnect(const OnConnectCallback& cb)
{
    DebugAssertWithInfo(m_onconnect_event == nullptr, "repeat register event!");
    m_onconnect_event = cb;
}

void Network::SetOnAccept(const OnAcceptCallback& cb)
{
    DebugAssertWithInfo(m_onaccept_event == nullptr, "repeat register event!");
    m_onaccept_event = cb;
    m_acceptor.SetOnConnect([this](const util::network::ConnectionSPtr& conn){ this->m_onconnect_event(conn); });
}

void Network::SetOnClose(const OnCloseCallback& cb)
{
    DebugAssertWithInfo(m_onclose_event == nullptr, "repeat register event!");
    m_onclose_event = cb;
}

void Network::SetOnTimeOut(const OnTimeOutCallback& cb)
{
    DebugAssertWithInfo(m_ontimeout_event == nullptr, "repeat register event!");
    m_ontimeout_event = cb;
}

#pragma region "工具函数"

Network::evIOThreadSPtr Network::NewConnLoadBlance()
{
    static std::atomic_int m_current_idx = 0;
    /* 第一个是acceptor线程，实际的io线程是后面的（m_io_thread_num - 1）个 */
    int idx = m_current_idx++ % (m_io_thread_num - 1) + 1;
    DebugAssert(idx != 0 && idx < m_io_threads.size());
    return m_io_threads[idx];
}

event_base* Network::OnCreateEventBase()
{
    auto base = event_base_new();
    m_ev_bases.push_back(base);
    return base;
}

void Network::OnDestoryEventBase(event_base* base)
{
    auto it = std::find_if(m_ev_bases.begin(), m_ev_bases.end(), 
        [=](event_base* val){
            return val == base;
        }
    );
    DebugAssertWithInfo(it != m_ev_bases.end(), "event base is unexpected value!"); // 出乎意料的结果
    event_base_free(base);
}

#pragma endregion

}// namespace end