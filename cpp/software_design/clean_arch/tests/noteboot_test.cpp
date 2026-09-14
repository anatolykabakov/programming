#include "gtest/gtest.h"
#include "notebook.h"

TEST(NotebookTest, AddStudent)
{
  NotebookApp app(NotebookApp::Config{.filename = "students.txt", .use_file_repository = true, .use_cli = true});
  app.run();
}
