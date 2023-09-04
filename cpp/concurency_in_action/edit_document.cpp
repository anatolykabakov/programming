#include <thread>
#include <iostream>
#include <mutex>

std::mutex m;

enum class command_type {
    open_new_doc
};

struct user_command {
    command_type type;
};

void open_document_and_display_gui(const std::string &filename) {
    std::lock_guard<std::mutex> lock(m);
    std::cout << __func__ << " " << filename << std::endl;
}

bool done_editing() {

    return false;
}

user_command get_user_input() {
    return user_command{command_type::open_new_doc};
}

std::string get_file_name_from_user() {
    std::string user_input;
    std::lock_guard<std::mutex> lock(m);
    std::cin >> user_input;
    return user_input;
}

void process() {

}

void edit_document(const std::string &filename) {
    open_document_and_display_gui(filename);

    while (!done_editing()) {
        user_command cmd = get_user_input();

        if (cmd.type == command_type::open_new_doc) {
            std::string new_name = get_file_name_from_user();
            std::thread t(edit_document, new_name);
            t.detach();
        } else {
            process();
        }
    }
}

int main() {
    std::thread t(edit_document, "doc1");
    t.join();
    return 0;
}