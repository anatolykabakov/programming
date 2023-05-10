// ref http://scrutator.me/post/2012/06/03/parallel-world-p2.aspx
#include <future>
#include <iostream>

int main() {
    auto promise = std::make_shared<std::promise<int>>();
    auto answer = promise->get_future();
    std::array<int, 5> arr = {0, 1, 2, 3, 4};
    auto handler = [promise](const std::array<int, 5> &arr, int value) {
        for (int i = 0; i < arr.size(); ++i) {
            if (arr[i] == value) {
                promise->set_value(i);
            }
        }
    };
    std::thread t(handler, arr, 3);
    t.detach();
    auto result = answer.get();
    std::cout << result << std::endl;

    return 0;
}
