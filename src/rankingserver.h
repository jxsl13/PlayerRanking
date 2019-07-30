#ifndef GAME_SERVER_RANKINGSERVER_H
#define GAME_SERVER_RANKINGSERVER_H

#include "playerstats.h"

#include <mutex>
#include <string>
#include <functional>
#include <future>
#include <vector>
#include <tuple>
#include <stdexcept>
#include <cpp_redis/cpp_redis>
 

class CRankingServer
{   

private:
    // redis server host & port
    std::string m_Host;
    size_t m_Port;

    // redis client
    cpp_redis::client m_Client;

    std::vector<std::string> m_InvalidNicknames;
    bool IsValidNickname(const std::string& nickname);
    
    
    // saving futures for later cleanup
    std::vector<std::future<void> > m_Futures;

    std::mutex m_ReconnectHandlerMutex;
    bool m_IsReconnectHandlerRunning;
    int m_ReconnectIntervalMilliseconds;
    void HandleReconnecting();
    void StartReconnectHandler();

    // when we get a disconnect, we safe out db changing actions in a backlog.
    std::mutex m_BacklogMutex;
    // action, nickname, stats data, prefix
    std::vector<std::tuple<std::string, std::string, CPlayerStats, std::string> > m_Backlog;

    // remove finished futures from vector
    void CleanFutures();

    // wait for all futures to finish execution(used in destructor)
    void WaitFutures();

    // retrieve player data syncronously
    CPlayerStats GetRankingSync(std::string nickname, std::string prefix = "");

    // synchronous execution of ranking update
    void UpdateRankingSync(std::string nickname, CPlayerStats stats, std::string prefix = "");

    // delete player's ranking
    void DeleteRankingSync(std::string nickname, std::string prefix = "");
    
public:

    // constructor
    CRankingServer(std::string host, size_t port, uint32_t timeout = 10000, uint32_t reconnect_ms = 5000);


    // gets data and does stuff that's defined in callback with it.
    // if no callback is provided, nothing is done.
    void GetRanking(std::string nickname, std::function<void(CPlayerStats&)> calback = nullptr, std::string prefix = "");

    // starts async execution of if nickname is valid
    bool UpdateRanking(std::string nickname,  CPlayerStats stats, std::string prefix = "");


    // if prefix is empty, the whole player is deleted.
    void DeleteRanking(std::string nickname, std::string prefix = "");

    // once in a while, the backlog can be cleaned
    // from the main thread
    void CleanupBacklog(std::string text = "");

    ~CRankingServer();
};




#endif // GAME_SERVER_RANKINGSERVER_H