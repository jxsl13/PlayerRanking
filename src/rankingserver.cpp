#include "rankingserver.h"
#include <algorithm>
#include <chrono>
#include <future>
#include <iostream>

CRankingServer::CRankingServer(std::string host, size_t port, uint32_t timeout, uint32_t reconnect_ms) : m_Host{host}, m_Port{port}
{
    m_ReconnectIntervalMilliseconds = reconnect_ms;
    try
    {
        m_Client.connect(m_Host, m_Port, nullptr, timeout, 0, reconnect_ms);
        if (m_Client.is_connected())
        {
            // no reconnection handling necessary
            std::lock_guard<std::mutex> lock(m_ReconnectHandlerMutex);
            m_IsReconnectHandlerRunning = false;
            std::cout << "[redis]: "
                      << "successfully connected to " << m_Host << ":" << m_Port << std::endl;
        }
    }
    catch (const cpp_redis::redis_error& e)
    {
        std::cout << "[redis]: "
                  << "failed to connect to " << m_Host << ":" << m_Port << std::endl;
        StartReconnectHandler();
    }
}

void CRankingServer::HandleReconnecting()
{
    while (!m_Client.is_connected())
    {
        try
        {
            m_Client.connect(m_Host, m_Port);
        }
        catch (const cpp_redis::redis_error& e)
        {
            std::cout << "[redis]: Reconnect failed...\n";
        }

        // wait
        std::this_thread::sleep_for(std::chrono::milliseconds(m_ReconnectIntervalMilliseconds));

        m_ReconnectHandlerMutex.lock();
        if (!m_IsReconnectHandlerRunning)
        {
            std::cout << "[redis]: Shutting down reconnect handler.\n";
            // forceful shutdown
            m_ReconnectHandlerMutex.unlock();

            // forceful shutdown, is done, when the ranking server is 
            // shutting down.
            CleanupBacklog();
            return;
        }
        m_ReconnectHandlerMutex.unlock();
    }

    // connection established
    std::lock_guard<std::mutex> lock(m_ReconnectHandlerMutex);
    m_IsReconnectHandlerRunning = false;

    std::cout << "[redis]: Successfully reconnected!\n";
    // if connection established, try purging the db backlog
    CleanupBacklog();
}

void CRankingServer::StartReconnectHandler()
{
    std::lock_guard<std::mutex> lock(m_ReconnectHandlerMutex);
    if (m_IsReconnectHandlerRunning)
        return; // already running

    m_IsReconnectHandlerRunning = true;
    m_Futures.push_back(std::async(std::launch::async, &CRankingServer::HandleReconnecting, this));
}

bool CRankingServer::IsValidNickname(const std::string& nickname)
{
    if (nickname.size() == 0)
        return false; // empty string nick -> no rankings for you
    else if (m_InvalidNicknames.size() == 0)
        return true; // no invalid nicks -> your nick is valid

    for (auto name : m_InvalidNicknames)
    {
        if (nickname == name)
            return false;
    }
    return true;
}

CPlayerStats CRankingServer::GetRankingSync(std::string nickname, std::string prefix)
{
    CPlayerStats stats;

    try
    {
        std::future<cpp_redis::reply> existsFuture = m_Client.exists({nickname});
        m_Client.sync_commit();

        cpp_redis::reply existsReply = existsFuture.get();

        if (existsReply.as_integer())
        {
            std::future<cpp_redis::reply> getFuture = m_Client.hmget(nickname, stats.keys(prefix));

            m_Client.sync_commit();
            cpp_redis::reply reply = getFuture.get();

            std::vector<cpp_redis::reply> result = reply.as_array();

            // set every key.
            int idx = 0;
            for (auto& key : stats.keys())
            {
                if (result.at(idx).is_string())
                {
                    stats[key] = std::stoi(result.at(idx).as_string());
                }
                else if (result.at(idx).is_integer())
                {
                    stats[key] = result.at(idx).as_integer();
                }
                else if (result.at(idx).is_null())
                {
                    // result is null
                    break; // entry does not exist yet.
                }
                else
                {
                    std::cerr << "[redis_error]: unkown result type\n";
                }
                idx++;
            }
        }
        return stats;
    }
    catch (const cpp_redis::redis_error& e)
    {
        std::cout << "[redis]: lost connection: " << e.what() << std::endl;
        stats.Invalidate();
        StartReconnectHandler();
        return stats;
    }
}

void CRankingServer::GetRanking(std::string nickname, std::function<void(CPlayerStats&)> callback, std::string prefix)
{
    // fix disconnect state that might have occurred in the mean time
    CleanFutures();

    if (callback)
    {
        m_Futures.push_back(std::async(
            std::launch::async, [this](std::string nick, std::function<void(CPlayerStats&)> cb, std::string pref) {
                CPlayerStats stats = this->GetRankingSync(nick, pref); // get data from server
                cb(stats);                                             // call callback on data
            },
            nickname, callback, prefix));
    }
}

void CRankingServer::DeleteRanking(std::string nickname, std::string prefix)
{
    CleanFutures();

    m_Futures.push_back(std::async(std::launch::async, &CRankingServer::DeleteRankingSync, this, nickname, prefix));
}

void CRankingServer::UpdateRankingSync(std::string nickname, CPlayerStats stats, std::string prefix)
{
    try
    {
        CPlayerStats dbStats;

        dbStats = GetRankingSync(nickname, prefix);

        if (!dbStats.IsValid())
            throw cpp_redis::redis_error(""); // jump to lost connection

        dbStats += stats;

        std::future<cpp_redis::reply> setFuture = m_Client.hmset(nickname, dbStats.GetStringPairs(prefix));

        m_Client.sync_commit();
    }
    catch (const cpp_redis::redis_error& e)
    {
        std::cout << "[redis]: lost connection: " << e.what() << std::endl;
        std::lock_guard<std::mutex> lock(m_BacklogMutex);
        m_Backlog.push_back({"update", nickname, stats, prefix});
        StartReconnectHandler();
        return;
    }
}

bool CRankingServer::UpdateRanking(std::string nickname, CPlayerStats stats, std::string prefix)
{
    CleanFutures();

    if (!IsValidNickname(nickname))
        return false;

    m_Futures.push_back(std::async(std::launch::async, &CRankingServer::UpdateRankingSync, this, nickname, stats, prefix));

    return true;
}

void CRankingServer::DeleteRankingSync(std::string nickname, std::string prefix)
{
    try
    {
        CPlayerStats stats;

        std::future<cpp_redis::reply> existsFuture = m_Client.exists({nickname});
        m_Client.sync_commit();

        cpp_redis::reply existsReply = existsFuture.get();

        if (existsReply.as_integer()) // exists
        {
            std::future<cpp_redis::reply> allKeysFuture = m_Client.hkeys(nickname);
            m_Client.sync_commit();
            cpp_redis::reply keysReply = allKeysFuture.get();

            // contains keys that ought to be deleted
            std::vector<std::string> keys;
            if (keysReply.is_array())
            {
                for (auto& key : keysReply.as_array())
                {
                    if (key.is_string())
                    {
                        keys.push_back(key.as_string());
                    }
                    else if (key.is_integer())
                    {
                        keys.push_back(std::to_string(key.as_integer()));
                    }
                    else
                    {
                        // invalid case
                        continue;
                    }
                }
            }
            else
            {
                std::cout << "failed to retrieve all field names, returned not an array." << std::endl;
                return;
            }

            std::future<cpp_redis::reply> delFuture;

            if (prefix.size() > 0)
            {
                // remove keys that don't have the correct prefix
                keys.erase(std::remove_if(keys.begin(), keys.end(), [&prefix](const std::string& key) {
                               if (key.size() < prefix.size())
                               {
                                   return true; // delete invalid key from vector
                               }

                               // expect key and prefix length to be at least equal
                               for (size_t i = 0; i < prefix.size(); i++)
                               {
                                   // prefix doesn't match
                                   if (key.at(i) != prefix.at(i))
                                       return true; // remove key, cuz invalid prefix
                               }
                               // full prefix matches, don't remove
                               return false;
                           }),
                           keys.end());

                if (keys.size() == 0)
                {
                    std::cout << "no keys matching prefix" << std::endl;
                    return;
                }

                // delete only specified fields with prefix
                delFuture = m_Client.hdel(nickname, keys);
            }
            else
            {
                // delete all player data
                delFuture = m_Client.del({nickname});
            }

            m_Client.sync_commit();
            cpp_redis::reply reply = delFuture.get();

            int result = 0;
            if (reply.is_integer())
            {
                result = reply.as_integer();
            }
            else
            {
                std::cout << reply << std::endl;
            }

            if (!result)
            {
                throw cpp_redis::redis_error("deletion failed");
            }
        }
    }
    catch (const cpp_redis::redis_error& e)
    {
        std::cout << "[redis]: lost connection: " << e.what() << std::endl;
        std::lock_guard<std::mutex> lock(m_BacklogMutex);
        m_Backlog.push_back({"delete", nickname, CPlayerStats(), prefix});
        StartReconnectHandler();
    }
}

void CRankingServer::CleanupBacklog(std::string text)
{
    std::lock_guard<std::mutex> lock(m_BacklogMutex);
    if (m_Backlog.size() > 0)
    {
        // we start new threads from here.
        // if those threads fail, they will keep on waiting
        // for this mutex, until they add their failed information
        // back to the backlog.

        int counter = 0;
        for (size_t i = 0; i < m_Backlog.size(); i++)
        {
            auto [action, nickname, stats, prefix] = m_Backlog.back();
            m_Backlog.pop_back();

            if (action == "update")
            {
                UpdateRanking(nickname, stats, prefix);
                counter++;
            }
            else if (action == "delete")
            {
                DeleteRanking(nickname, prefix);
                counter++;
            }
        }

        std::cout << "[redis]: Cleaned up " << counter << " backlog tasks." << std::endl;
    }
    else
    {
        // backlog empty
    }
}

void CRankingServer::CleanFutures()
{
    if (m_Futures.size() == 0)
        return;

    auto it = std::remove_if(m_Futures.begin(), m_Futures.end(), [&](std::future<void>& f) {
        if (std::future_status::ready == f.wait_for(std::chrono::milliseconds(0)))
        {
            try
            {
                if (f.valid())
                {
                    f.get();
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << e.what() << '\n';
            }

            return true;
        }
        return false;
    });
    m_Futures.erase(it, m_Futures.end());
}

void CRankingServer::WaitFutures()
{
    for (auto& f : m_Futures)
    {
        f.wait();
        if (f.valid())
        {
            try
            {
                f.get();
            }
            catch (const cpp_redis::redis_error& e)
            {
                std::cerr << e.what() << '\n';
            }
        }
    }
}

CRankingServer::~CRankingServer()
{
    // we still fail to reconnect at shutdown -> force shutdown
    m_ReconnectHandlerMutex.lock();
    m_IsReconnectHandlerRunning = false;
    m_ReconnectHandlerMutex.unlock();

    WaitFutures();

    if (m_Client.is_connected())
    {
        m_Client.disconnect(true);
        std::cout << "[redis]: disconnected from database" << std::endl;
    }
}
