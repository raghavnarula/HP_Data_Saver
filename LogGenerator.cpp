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
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        stopping = true;
    }

    cv.notify_one();

    if (workerThread.joinable()) {
        workerThread.join();
    }

    if (_fd != -1) {
        close(_fd);
    }
}

void LogGenerator::writeLog() {
    auto& queue = boundedQueue;

    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex);

        // Sleep until logs arrive or shutdown is requested
        cv.wait(lock, [this]() {
            return !boundedQueue.empty() || stopping;
        });

        // Exit only when shutting down and all logs are processed
        if (stopping && queue.empty()) {
            break;
        }

        // Safely retrieve a block
        Block block = std::move(queue.front());
        queue.pop();

        // Release mutex before slow disk I/O
        lock.unlock();

        size_t bytesWrittenThisBlock = 0;
        bool success = true;

        while (bytesWrittenThisBlock < block._blockSize) {
            size_t bytesRemaining =
                block._blockSize - bytesWrittenThisBlock;

            ssize_t bytesWritten = write(
                _fd,
                block._block.c_str() + bytesWrittenThisBlock,
                bytesRemaining
            );

            if (bytesWritten == -1) {
                if (errno == EINTR) {
                    continue;
                }

                std::cout << "Failed to Write" << std::endl;
                success = false;
                break;
            }

            if (bytesWritten == 0) {
                success = false;
                break;
            }

            bytesWrittenThisBlock += bytesWritten;
        }

        if (success) {
            LOG_INFO("Block Written successfully...");
        }
    }
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