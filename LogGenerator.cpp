#include "LogGenerator.h"
#include <random>
#include <algorithm>
#include <ranges>
#include <errno.h>
#include "logger.h"


LogGenerator::LogGenerator(std::string fileName)
{
    _fileName = fileName;
    _fd = open(_fileName.c_str(),O_WRONLY | O_CREAT | O_APPEND | O_SYNC,0644);
    if (_fd == -1){
        if (errno == EINTR){
            std::cout << "File Not Found";
        }
        return;
    }

    // worker thread
    workerThread = std::thread(&LogGenerator::writeLog,this);

}

LogGenerator::~LogGenerator() {
    if (_fd != -1) {
        close(_fd);
    }
    workerThread.join();
}



void LogGenerator::writeLog() {

    auto& queue = LogGenerator::boundedQueue;
    
    while (queue.size() != 0){
        Block block = queue.front();
        queue.pop();
        size_t bytesWrittenThisBlock = 0;

        while (bytesWrittenThisBlock < block._blockSize) {
            
            long int bytesRemaining = block._blockSize - bytesWrittenThisBlock;
            
            // Write into Disk with O_SYNC .. 
            ssize_t bytesWritten = write(_fd, block._block.c_str() + bytesWrittenThisBlock, bytesRemaining);
            
            if (bytesWritten == -1) {  
                std::cout << "Failed to Write" << std::endl;
                break; // Exit loop on a fatal write error
            }
            
            bytesWrittenThisBlock += bytesWritten;
        } 
        LOG_INFO("Block Written successfully...");
    }
    return;
}

/// @brief sends the block to boundedQueue;
void LogGenerator::blockPublisher()
{
    std::lock_guard<std::mutex> lock(queueMutex);
    auto& queue = LogGenerator::boundedQueue;
    if (queue.size() < LogGenerator::maxBoundedQueueCapacity){
        queue.push(Block());
        // notify the worker to wake up and start processing. 
        cv.notify_one();
    }
    else{
        std::cout << "Queue limit reached. Please Wait" << std::endl;
    }
    return;
}