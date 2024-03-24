#include <iostream>

#include <vector>
#include <queue>
#include <mutex>
// #include <memory>
#include <thread>
#include <future>
#include <functional>
#include <condition_variable>
#include <stdexcept>


class ThreadPool {
public:
    ThreadPool(size_t);

    template<class F, class... Args>
    auto enqueue(F &&f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;

    ~ThreadPool();

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

// 线程池构造函数，通过 for 循环将获取任务的函数塞入 workers 中，顺便创建四个线程不断执行着这个函数。
ThreadPool::ThreadPool(size_t threads)
 : stop(false) {
    for (int i = 0; i < threads; i++) {
        workers.emplace_back(
            [this]
            {
                for(;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);

                        // 条件变量等待，如果 stop为ture 或者 tasks为空 则进去等待
                        this->condition.wait(lock, 
                            [this]{return this->stop || !this->tasks.empty();});

                        // 如果 stop为ture 且 tasks为空时，退出函数
                        if (this->stop && this->tasks.empty()) {
                            return;
                        }
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            }
        );
    }
}

// 使用模版操作，较为复杂
// 将要线程执行的任务塞入 tasks 中
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type> 
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared< std::packaged_task<return_type()> >(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    // 给将来函数的返回值做准备
    std::future<return_type> res = task->get_future();
    {
        // 在该作用域中，加锁
        std::unique_lock<std::mutex> lock(this->queue_mutex);

        if (stop) {
            throw std::runtime_error("enqueue on stopped ThreadPool.");
        }
        tasks.emplace([task]() { (*task)(); });
    }
    condition.notify_one();
    return res;
}

inline ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(this->queue_mutex);
        stop = true;
    }
    this->condition.notify_all();

    // 结束函数
    for (std::thread& worker : workers) {
        worker.join();
    }
}