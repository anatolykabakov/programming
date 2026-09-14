#include "notebook.h"
#include <tclap/CmdLine.h>
#include <tclap/ValueArg.h>
#include <tclap/SwitchArg.h>

int main(int argc, char** argv)
{
  TCLAP::CmdLine cmd("notebook app", ' ', "0.1");
  TCLAP::ValueArg<std::string> filename_arg("f", "filename", "Filename to use for the notebook", false, "students.txt",
                                            "filename");
  TCLAP::SwitchArg file_repository_arg("", "file", "Use file repository", false);
  TCLAP::SwitchArg http_server_arg("", "http", "Use HTTP server", false);
  cmd.add(filename_arg);
  cmd.add(file_repository_arg);
  cmd.add(http_server_arg);
  try {
    cmd.parse(argc, argv);
  } catch (const TCLAP::ArgException& e) {
    std::cerr << "error: " << e.what() << std::endl;
    return 1;
  }
  NotebookApp app(NotebookApp::Config{.filename = filename_arg.getValue(),
                                      .use_file = file_repository_arg.getValue(),
                                      .use_http = http_server_arg.getValue()});
  app.run();
  return 0;
}
