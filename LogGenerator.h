#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include "Block.h"
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

class LogGenerator{
    public:
        explicit LogGenerator(std::string _fileName);
        void writeLog();
        ~LogGenerator();
        void blockPublisher();

    private:
        std::string _fileName = "app.log";
        size_t numberOfBlocks = 10;
        int _fd;
        std::ofstream logFile_;
        std::queue<Block> boundedQueue;
        size_t maxBoundedQueueCapacity = 1000;
        
        // background worker thread
        std::thread workerThread;
        std::mutex queueMutex;
        std::condition_variable cv;
        
};