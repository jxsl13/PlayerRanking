
#include <string>
#include <iostream>
#include "rankingserver.h"
#include <thread>
#include <chrono>


int main(int argc, const char* argv[])
{   
    using namespace std::chrono_literals;

    std::string host = "127.0.0.1";
    size_t port = 6379;

    CRankingServer ranks(host, port);
    
    ranks.UpdateRanking("test", {1,2,3,4,5,6,7});

    return 0;
}
