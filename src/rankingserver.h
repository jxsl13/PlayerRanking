#ifndef GAME_SERVER_RANKINGSERVER_H
#define GAME_SERVER_RANKINGSERVER_H

#include <mutex>
#include <string>
#include <future>
#include <vector>
#include <cpp_redis/cpp_redis>
 

class CRankingServer
{
public:
    struct CPlayerStats
    {
        int m_Kills;
        int m_Deaths;
        int m_TicksCaught; 
        int m_TicksIngame; 
        int m_Score;
        int m_Fails;
        int m_Shots;
        void Reset()
        {
            m_Kills = 0;
            m_Deaths = 0;
            m_TicksCaught = 0; 
            m_TicksIngame = 0; 
            m_Score = 0;
            m_Fails = 0;
            m_Shots = 0;
        }

        CPlayerStats(int kills, int deaths, int ticksCaught, int ticksIngame, int score, int fails, int shots) :
            m_Kills(kills), m_Deaths(deaths), m_TicksCaught(ticksCaught), m_TicksIngame(ticksIngame), m_Score(score), m_Fails(fails), m_Shots(shots)
        {
            // constructor
        }
    };

private:
    std::string m_Host;
    size_t m_Port;

    std::mutex m_ClientMutex;
    cpp_redis::client m_Client;
    
    std::vector<std::future<void> > m_Futures;


    void CleanFutures();
    void WaitFutures();

    void UpdateRankingSync(std::string nickname,struct CPlayerStats stats, std::string prefix = "");
    
public:

    CRankingServer(std::string host, size_t port, uint32_t timeout = 0);
    void UpdateRanking(std::string nickname,struct CPlayerStats stats, std::string prefix = "");


    ~CRankingServer();
};




#endif // GAME_SERVER_RANKINGSERVER_H