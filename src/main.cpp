
#include <string>
#include <iostream>
#include "rankingserver.h"
#include <thread>




int main(int argc, const char* argv[])
{   

    std::string host{"127.0.0.1"};
    size_t port{6379};
    CRankingServer ranks(host, port);

    ranks.UpdateRanking("test2", {1,2,3,4,5,6,7}, "Grenade_");
    
    std::string test;
    std::cout << "Test 1";
    std::cin >> test;

    ranks.DeleteRanking("test2", "Grenade_");
    
    std::cout << "Test 2";
    std::cin >> test;
    return 0;
}
