#include "util/network/libevent/evIOThread.hpp"

namespace game::util::network::ev
{


evIOThread::evIOThread()
{
    Init();
}

evIOThread::~evIOThread()
{
    /* 等待线程停止 */
    if(m_status == IOThreadRunStatus::Running)
        Stop();        
    /* 资源回收 */
    Destory();
}

void evIOThread::Init()
{

    SetOnThreadBegin_Hook([](IOThreadID tid){
        GAME_BASE_LOG_INFO("IOThread Start Success! tid = %d", tid);
    });
    
    SetOnThreadEnd_Hook([](IOThreadID tid){
        GAME_BASE_LOG_INFO("IOThread Exit! tid = %d", tid);
    });
}

void evIOThread::Destory()
{
}


void evIOThread::evWorkFunc()
{
    m_io_work_func();
}

void evIOThread::Start()
{
    AssertWithInfo(m_io_work_func != nullptr, "io work need be set!");
    SetWorkTask([=](){ evWorkFunc(); });
    StartWorkFunc();
}

void evIOThread::Stop()
{
    DebugAssert(m_ev_base != nullptr);
    int error = event_base_loopbreak(m_ev_base);
    DebugAssert(error == 0);
}

void evIOThread::SetWorkFunc(const IOWorkFunc& cb)
{
    m_io_work_func = cb;
}

void evIOThread::SetEventBase(event_base* ev_base)
{
    DebugAssertWithInfo(ev_base != nullptr, "this is a fatal bug!");
    m_ev_base = ev_base;
}

}// namespace end