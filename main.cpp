#include "LogGenerator.h"
#include <fstream>
#include <thread>
#include <vector>
#include "Block.h"

void worker(LogGenerator& logger) {
    for (int i = 0; i < 2000; i++) {
        logger.blockPublisher();
    }
}

int main()
{   
    std::vector<std::thread> threads;
    LogGenerator logger("/mnt/usb/app.log");
    
    // Create 3 working threads
    for (int i = 0; i < 3; i++) {
        threads.emplace_back(worker, std::ref(logger));
    }

    for (auto& t : threads) {
        t.join();
    }


    return 0;
}