#include "studyPool.h"
// #include <iostream>
using namespace std;

int main() {
    ThreadPool pool(4);
    // 通过将线程池函数返回的值存放到 std::future<int> 的集合中
    std::vector<std::future<int>> results;
    
    for (int i = 0; i < 8; i++) {
        // emplace_back 的元素实际上是函数的返回值
        results.emplace_back(
            pool.enqueue([i](){
                std::cout << "hello " << i << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                std::cout << "world " << i << std::endl;
                return i*i;
            })
        );
    }

    // 通过循环将存放进入的返回值元素打印出来
    for (auto && result : results) {
        cout << result.get() << endl;
        cout << "===========" << endl;
    }

}