
#include <iostream>
#include <string>
#include <thread>
#include "rankingserver.h"

int main(int argc, const char* argv[])
{
    std::string host{"127.0.0.1"};
    size_t port{6379};
    CRankingServer ranks(host, port);

    CPlayerStats tmp;
    auto keys = tmp.keys();

    ranks.UpdateRanking("test0", {1, 2, 3, 4, 5, 6, 7});
    ranks.UpdateRanking("test1", {2, 3, 4, 5, 6, 7, 8});
    ranks.UpdateRanking("test2", {3, 4, 5, 6, 7, 8, 9});
    ranks.UpdateRanking("test3", {4, 5, 6, 7, 8, 9, 10});
    ranks.UpdateRanking("test4", {5, 6, 7, 8, 9, 10, 11});
    ranks.UpdateRanking("test5", {6, 7, 8, 9, 10, 11, 1});
    ranks.UpdateRanking("test6", {7, 8, 9, 10, 11, 1, 2});
    ranks.UpdateRanking("test7", {8, 9, 10, 11, 1, 2, 3});
    ranks.UpdateRanking("test8", {9, 10, 11, 1, 2, 3, 4});
    ranks.UpdateRanking("test9", {10, 11, 1, 2, 3, 4, 5});
    ranks.UpdateRanking("test10", {11, 1, 2, 3, 4, 5, 6});

    std::string prefix{"Grenade_"};

    ranks.UpdateRanking("g_test0", {1, 2, 3, 4, 5, 6, 7}, prefix);
    ranks.UpdateRanking("g_test1", {2, 3, 4, 5, 6, 7, 8}, prefix);
    ranks.UpdateRanking("g_test2", {3, 4, 5, 6, 7, 8, 9}, prefix);
    ranks.UpdateRanking("g_test3", {4, 5, 6, 7, 8, 9, 10}, prefix);
    ranks.UpdateRanking("g_test4", {5, 6, 7, 8, 9, 10, 11}, prefix);
    ranks.UpdateRanking("g_test5", {6, 7, 8, 9, 10, 11, 1}, prefix);
    ranks.UpdateRanking("g_test6", {7, 8, 9, 10, 11, 1, 2}, prefix);
    ranks.UpdateRanking("g_test7", {8, 9, 10, 11, 1, 2, 3}, prefix);
    ranks.UpdateRanking("g_test8", {9, 10, 11, 1, 2, 3, 4}, prefix);
    ranks.UpdateRanking("g_test9", {10, 11, 1, 2, 3, 4, 5}, prefix);
    ranks.UpdateRanking("g_test10", {11, 1, 2, 3, 4, 5, 6}, prefix);

    ranks.UpdateRanking("g_test00", {1, 1, 1, 1, 1, 1, 1}, prefix);

    // enforce synchronization
    ranks.AwaitFutures();
    for (size_t idx = 0; idx < keys.size(); idx++)
    {
        // show top 3 players per attribute
        ranks.GetTopRanking(3, keys.at(idx), [&keys, idx](std::vector<std::pair<std::string, CPlayerStats> >& dataList) {
            int rank = 1;
            for (auto& [nickname, stats] : dataList)
            {
                std::cout << rank <<". " << "[" << keys.at(idx) << ":" << stats[keys.at(idx)] << "] " << nickname << std::endl;;
                rank++;
            }
        }); // lambda end
        ranks.AwaitFutures();
        std::cout << std::endl;

        // 4 instead of 3
        ranks.GetTopRanking(4, keys.at(idx), [&keys, idx](std::vector<std::pair<std::string, CPlayerStats> >& dataList) {
            int rank = 1;
            for (auto& [nickname, stats] : dataList)
            {
                std::cout << rank <<". " << "[" << keys.at(idx) << ":" << stats[keys.at(idx)] << "] " << nickname << std::endl;;
                rank++;
            }
        }, prefix); // lambda end, prefix added
        ranks.AwaitFutures();
        std::cout << std::endl;
    }

    std::string nickname{"test0"};
    ranks.GetRanking(nickname, [nickname](CPlayerStats& stats)
    {
        std::cout << nickname << " : " << stats << std::endl;
        std::cout << "is valid: " << (stats.IsValid() ? "valid" : "invalid") << std::endl;
    }); // no prefix needed, as this entry was not created with one.

    ranks.AwaitFutures();


    nickname = "g_test00";
    std::cout << "explicitly set prefix:" << std::endl;
    ranks.GetRanking(nickname, [nickname](CPlayerStats& stats)
    {
        std::cout << nickname << " : " << stats << std::endl;
        std::cout << "is valid: " << (stats.IsValid() ? "valid" : "invalid") << std::endl;
    }, prefix); // explicitly set prefix!

    ranks.AwaitFutures();

    std::cout << "" << std::endl;
    ranks.GetRanking(nickname, [nickname](CPlayerStats& stats)
    {
        std::cout << nickname << " : " << stats << std::endl;
        std::cout << "is valid: " << (stats.IsValid() ? "valid" : "invalid") << std::endl;
    }); // without prefix this will result in an invalid/ empty type

    ranks.AwaitFutures();


    
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
        ranks.DeleteRanking(nickname);
        ranks.AwaitFutures();
    }




    return 0;
}
