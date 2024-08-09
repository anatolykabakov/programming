#include <client/linux/handler/exception_handler.h>
#include <client/linux/handler/minidump_descriptor.h>

bool callback(const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool succeeded)
{
  std::cout << "dump callback called" << std::endl;
  if (!succeeded) {
    return false;
  }

  Framework* f = (Framework*)context;

  ::rename(descriptor.path(), p->dumpPath().c_str());

  return false;  // Return false allows other handlers to handle exception.
}

class Framework {
  Framework(const std::string& dumpDir) : dumpDir_(dumpDir)
  {
    exception_handler = std::make_unique<google_breakpad::ExceptionHandler>(
        google_breakpad::MinidumpDescriptor(dumpDir),  // dump descriptor
        nullptr,                                       // no filter
        callback,                                      // to call after writing the minidump
        this,                                          // callback context
        true,                                          // install handler
        -1                                             // server_fd
    );
    std::cout << "Exception handler registered" << std::endl;
  }

  std::string dumpPath() { return dumpPath_ + "/minidump.dmp"; }

  ~Framework() { std::cout << "Exception handler destroyed" << std::endl; }

private:
  std::unique_ptr<google_breakpad::ExceptionHandler> exception_handler;
  std::string dumpDir_;
};

int main() { Framework f; }
