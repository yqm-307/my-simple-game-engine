#include "gameserver/init/LoadConfig.hpp"
#include "util/assert/Assert.hpp"

namespace server::init
{



std::string ServerConfig::m_server_inifile_path = "config/gameserver.ini";

ServerConfig::ServerConfig()
    :ConfigHelper(m_server_inifile_path)
{
    Init();
}

ServerConfig::~ServerConfig()
{
}

void ServerConfig::Init()
{
    m_config.server_ip = m_ini_reader->GetString("server", "ip", "");
    AssertWithInfo(m_config.server_ip != "", "fatal: server ip is null!");
    m_config.server_port = m_ini_reader->GetInteger("server", "port", 0);
    AssertWithInfo(m_config.server_port > 0, "fatal: server port is bad!");

    m_config.m_connmgr_cfg.m_heart_beat_once_ms = m_ini_reader->GetInteger("heartbeat", "interval", 0);
    AssertWithInfo(m_config.m_connmgr_cfg.m_heart_beat_once_ms > 0, "fatal: heartbeat is invalid!");
    m_config.m_connmgr_cfg.m_heart_beat_timeout_ms = m_ini_reader->GetInteger("heartbeat", "checkonce", 0);
    AssertWithInfo(m_config.m_connmgr_cfg.m_heart_beat_timeout_ms > 0, "fatal: heartbeat check once invalid!");
    G_SetConfigPtr(game::util::config::evConnMgrConfig, &m_config.m_connmgr_cfg, game::util::config::Cfg_evConnMgr);
}


ServerConfig* ServerConfig::GetInstance()
{
    ServerConfig* instance = nullptr;
    if(instance == nullptr)
    {
        instance = new ServerConfig();
    }
    return instance;
}

std::string ServerConfig::GetServerIP()
{
    return m_config.server_ip;
}

int ServerConfig::GetServerPort()
{
    return m_config.server_port;
}


}