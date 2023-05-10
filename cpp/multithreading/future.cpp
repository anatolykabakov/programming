// ref http://scrutator.me/post/2012/06/03/parallel-world-p2.aspx
#include <future>
#include <iostream>

std::future<bool> submit_form(const std::string &form) {
    auto handler = [](const std::string &form) {
        std::cout << "Handle the form" << std::endl;
        return true;
    };

    std::packaged_task<bool(const std::string &)> task(handler);
    auto future = task.get_future();
    std::thread t(std::move(task), form);
    t.detach();
    return std::move(future);
}

int main() {
    auto answer = submit_form("my form");
    if (answer.get()) {
        std::cout << "true" << std::endl;
    } else {
        std::cout << "false" << std::endl;
    }
    return 0;
}
