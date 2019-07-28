#include "rankingserver.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <future>

CRankingServer::CRankingServer(std::string host, size_t port, uint32_t timeout) : m_Host{host}, m_Port{port}
{
    try
    {
        m_Client.connect(m_Host, m_Port, nullptr, timeout);
        if (m_Client.is_connected())
        {
            std::cout << "[redis]: " << "successfully connected to " << m_Host << ":" << m_Port << std::endl;
        }
    }
    catch(const std::exception& e)
    {
        std::cout << "[redis]: " << "failed to connect to " << m_Host << ":" << m_Port << std::endl;
    }      
}

void CRankingServer::UpdateRankingSync(std::string nickname,struct CPlayerStats stats, std::string prefix)
{
    int Kills = 0;  
    int Deaths = 0; 
    int TicksCaught = 0; 
    int TicksIngame = 0; 
    int Score = 0; 
    int Fails = 0; 
    int Shots = 0;  

    // lock client access
    std::lock_guard<std::mutex> lock(m_ClientMutex);

    std::future<cpp_redis::reply> existsFuture = m_Client.exists({nickname});
    m_Client.sync_commit();

    cpp_redis::reply existsReply = existsFuture.get();

    if (existsReply.as_integer())
    {
        std::future<cpp_redis::reply> getFuture = m_Client.hmget(nickname, 
        {
            prefix + "Kills", 
            prefix + "Deaths", 
            prefix + "TicksCaught", 
            prefix + "TicksIngame", 
            prefix + "Score", 
            prefix + "Fails", 
            prefix + "Shots"
        });

        m_Client.sync_commit();
        cpp_redis::reply reply = getFuture.get();

        std::vector<cpp_redis::reply> result = reply.as_array();

        Kills =         std::stoi(result.at(0).as_string());
        Deaths =        std::stoi(result.at(1).as_string());
        TicksCaught =   std::stoi(result.at(2).as_string());
        TicksIngame =   std::stoi(result.at(3).as_string());
        Score =         std::stoi(result.at(4).as_string());
        Fails =         std::stoi(result.at(5).as_string());
        Shots =         std::stoi(result.at(6).as_string());
    }

    Kills += stats.m_Kills;
    Deaths += stats.m_Deaths;
    TicksCaught += stats.m_TicksCaught;
    TicksIngame += stats.m_TicksIngame;
    Score += stats.m_Score;
    Fails += stats.m_Fails;
    Shots += stats.m_Shots;

    std::future<cpp_redis::reply> setFuture = m_Client.hmset(nickname,
    {
        { prefix + "Kills", std::to_string(Kills) },
        { prefix + "Deaths", std::to_string(Deaths) },
        { prefix + "TicksCaught", std::to_string(TicksCaught) },
        { prefix + "TicksIngame", std::to_string(TicksIngame) },
        { prefix + "Score", std::to_string(Score) },
        { prefix + "Fails", std::to_string(Fails) },
        { prefix + "Shots", std::to_string(Shots) }
    });
    m_Client.sync_commit();
}

void CRankingServer::UpdateRanking(std::string nickname,struct CPlayerStats stats, std::string prefix)
{
    std::future<void> f = std::async(std::launch::async, &CRankingServer::UpdateRanking, this, nickname, stats, prefix);
    m_Futures.push_back(std::move(f));
}

void CRankingServer::CleanFutures()
{
    int cleaned = 0;

    auto it = std::remove_if(m_Futures.begin(), m_Futures.end(), [&](std::future<void> &f){
        if (std::future_status::ready == f.wait_for(std::chrono::seconds(0)))
        {
           cleaned++;
           return true;
        }
        return false;
    });
    m_Futures.erase(it, m_Futures.end());
    std::cout << "cleaned " << cleaned << " futures" << std::endl;
}

void CRankingServer::WaitFutures()
{
    for (auto &f : m_Futures)
    {
        f.wait();
        f.get();
    }
}

CRankingServer::~CRankingServer()
{
    WaitFutures();
    CleanFutures();
    if (m_Client.is_connected())
    {
        m_Client.disconnect(true);
        std::cout << "[redis]: disconnected from database" << std::endl;
    }   
}

