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
    bool IsValidNickname(const std::string& nickname, const std::string& prefix = "");
    
    
    // saving futures for later cleanup
    std::vector<std::future<void> > m_Futures;

    // remove finished futures from vector
    void CleanupFutures();

    std::mutex m_ReconnectHandlerMutex;
    bool m_IsReconnectHandlerRunning;
    int m_ReconnectIntervalMilliseconds;
    void HandleReconnecting();
    void StartReconnectHandler();

    // when we get a disconnect, we safe out db changing actions in a backlog.
    std::mutex m_BacklogMutex;
    // action, nickname, stats data, prefix
    std::vector<std::tuple<std::string, std::string, CPlayerStats, std::string> > m_Backlog;

    // cleanup backlog, when the conection has been established again.
    void CleanupBacklog();

    // retrieve player data syncronously
    CPlayerStats GetRankingSync(std::string nickname, std::string prefix = "");

    // synchronous execution of ranking update
    void UpdateRankingSync(std::string nickname, CPlayerStats stats, std::string prefix = "");

    // delete player's ranking
    void DeleteRankingSync(std::string nickname, std::string prefix = "");

    // retrieve top x player ranks based on their key property(like score, kills etc.).
    std::vector<std::pair<std::string, CPlayerStats> > GetTopRankingSync(int topNumber, std::string key, std::string prefix = "", bool biggestFirst = true);
    
public:

    // constructor
    CRankingServer(std::string host, size_t port, uint32_t timeout = 10000, uint32_t reconnect_ms = 5000);


    // gets data and does stuff that's defined in callback with it.
    // if no callback is provided, nothing is done.
    void GetRanking(std::string nickname, std::function<void(CPlayerStats&)> calback = nullptr, std::string prefix = "");

    // possible keys CPlayerStats::keys()
    void GetTopRanking(int topNumber, std::string key, std::function<void(std::vector<std::pair<std::string, CPlayerStats> >&)> callback = nullptr,  std::string prefix = "", bool biggestFirst = true);

    // starts async execution of if nickname is valid
    bool UpdateRanking(std::string nickname,  CPlayerStats stats, std::string prefix = "");

    // if prefix is empty, the whole player is deleted.
    void DeleteRanking(std::string nickname, std::string prefix = "");

    // This functions can, but should not necessarily be used.
    // It can be used to synchronize execution.
    // wait for all futures to finish execution(used in destructor)
    void AwaitFutures();

    ~CRankingServer();
};




#endif // GAME_SERVER_RANKINGSERVER_H