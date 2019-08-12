
#include "rankingserver.h"

int main(int argc, const char* argv[])
{
    std::string host{"127.0.0.1"};
    size_t port{6379};
    IRankingServer *pRanks = new CRedisRankingServer{host, port};


    CPlayerStats tmp;
    auto keys = tmp.keys();

    pRanks->SetRanking("test0", {1, 2, 3, 4, 5, 6, 7});
    pRanks->SetRanking("test1", {2, 3, 4, 5, 6, 7, 8});
    pRanks->SetRanking("test2", {3, 4, 5, 6, 7, 8, 9});
    pRanks->UpdateRanking("test3", {4, 5, 6, 7, 8, 9, 10});
    pRanks->UpdateRanking("test4", {5, 6, 7, 8, 9, 10, 11});
    pRanks->UpdateRanking("test5", {6, 7, 8, 9, 10, 11, 1});
    pRanks->UpdateRanking("test6", {7, 8, 9, 10, 11, 1, 2});
    pRanks->UpdateRanking("test7", {8, 9, 10, 11, 1, 2, 3});
    pRanks->UpdateRanking("test8", {9, 10, 11, 1, 2, 3, 4});
    pRanks->UpdateRanking("test9", {10, 11, 1, 2, 3, 4, 5});
    pRanks->UpdateRanking("test10", {11, 1, 2, 3, 4, 5, 6});

    std::string prefix{"Grenade_"};

    pRanks->UpdateRanking("g_test0", {1, 2, 3, 4, 5, 6, 7}, prefix);
    pRanks->UpdateRanking("g_test1", {2, 3, 4, 5, 6, 7, 8}, prefix);
    pRanks->UpdateRanking("g_test2", {3, 4, 5, 6, 7, 8, 9}, prefix);
    pRanks->UpdateRanking("g_test3", {4, 5, 6, 7, 8, 9, 10}, prefix);
    pRanks->UpdateRanking("g_test4", {5, 6, 7, 8, 9, 10, 11}, prefix);
    pRanks->UpdateRanking("g_test5", {6, 7, 8, 9, 10, 11, 1}, prefix);
    pRanks->UpdateRanking("g_test6", {7, 8, 9, 10, 11, 1, 2}, prefix);
    pRanks->UpdateRanking("g_test7", {8, 9, 10, 11, 1, 2, 3}, prefix);
    pRanks->UpdateRanking("g_test8", {9, 10, 11, 1, 2, 3, 4}, prefix);
    pRanks->UpdateRanking("g_test9", {10, 11, 1, 2, 3, 4, 5}, prefix);
    pRanks->UpdateRanking("g_test10", {11, 1, 2, 3, 4, 5, 6}, prefix);

    pRanks->UpdateRanking("g_test00", {1, 1, 1, 1, 1, 1, 1}, prefix);

    // enforce synchronization
    pRanks->AwaitFutures();
    for (size_t idx = 0; idx < keys.size(); idx++)
    {
        // show top 3 players per attribute
        pRanks->GetTopRanking(3, keys.at(idx), [&keys, idx](std::vector<std::pair<std::string, CPlayerStats> >& dataList) {
            int rank = 1;
            for (auto& [nickname, stats] : dataList)
            {
                std::cout << rank <<". " << "[" << keys.at(idx) << ":" << stats[keys.at(idx)] << "] " << nickname << std::endl;;
                rank++;
            }
        }); // lambda end
        pRanks->AwaitFutures();
        std::cout << std::endl;

        // 4 instead of 3
        pRanks->GetTopRanking(4, keys.at(idx), [&keys, idx](std::vector<std::pair<std::string, CPlayerStats> >& dataList) {
            int rank = 1;
            for (auto& [nickname, stats] : dataList)
            {
                std::cout << rank <<". " << "[" << keys.at(idx) << ":" << stats[keys.at(idx)] << "] " << nickname << std::endl;;
                rank++;
            }
        }, prefix); // lambda end, prefix added
        pRanks->AwaitFutures();
        std::cout << std::endl;
    }

    std::string nickname{"test0"};
    pRanks->GetRanking(nickname, [nickname](CPlayerStats& stats)
    {
        std::cout << nickname << " : " << stats << std::endl;
        std::cout << "is valid: " << (stats.IsValid() ? "valid" : "invalid") << std::endl;
    }); // no prefix needed, as this entry was not created with one.

    pRanks->AwaitFutures();


    nickname = "g_test00";
    std::cout << "explicitly set prefix:" << std::endl;
    pRanks->GetRanking(nickname, [nickname](CPlayerStats& stats)
    {
        std::cout << nickname << " : " << stats << std::endl;
        std::cout << "is valid: " << (stats.IsValid() ? "valid" : "invalid") << std::endl;
    }, prefix); // explicitly set prefix!

    pRanks->AwaitFutures();

    std::cout << "" << std::endl;
    pRanks->GetRanking(nickname, [nickname](CPlayerStats& stats)
    {
        std::cout << nickname << " : " << stats << std::endl;
        std::cout << "is valid: " << (stats.IsValid() ? "valid" : "invalid") << std::endl;
    }); // without prefix this will result in an invalid/ empty type

    pRanks->AwaitFutures();


    
    std::vector<std::string> nicknames = {
        "test0", "test1", "test2", "test3", "test4", "test5",
        "test6", "test7", "test8", "test9", "test10", "g_test0",
        "g_test1", "g_test2", "g_test3", "g_test4", "g_test5",
        "g_test6", "g_test7", "g_test8", "g_test9", "g_test10", "g_test00"};

    std::string s;
    std::cout << "Please confirm deletion of all the previously created nicks." << std::endl;
    std::cin >> s;

    for (auto &nickname : nicknames)
    {
        std::cout << "deleting: " << nickname << std::endl; 
        pRanks->DeleteRanking(nickname);
        pRanks->AwaitFutures();
    }



    delete pRanks;
    return 0;
}
