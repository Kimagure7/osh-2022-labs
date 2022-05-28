#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

int main()
{
    int n = 0;
    int data;
    bool ready = false;
    std::mutex mutex;
    std::condition_variable cv;
    auto f = [&]
    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [&]
                { return ready; });
        std::cout << data << std::endl;
    };
    std::thread thread(f);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    {
        std::lock_guard<std::mutex> lock(mutex);
        data = 1234;
        ready = true;
        cv.notify_one();
    }
    thread.join();
    return 0;
}
