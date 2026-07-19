#pragma once

#include <fstream>
#include <httplib.h>
#include <json/value.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// domain layer
struct Student {
  int id{0};
  std::string name;
};

// application layer (port)
class IStudentRepository {
public:
  virtual void add(const Student& s) = 0;
  virtual std::vector<Student> getAll() = 0;
  virtual ~IStudentRepository() = default;
};

// infrastructure layer
class InMemoryStudentRepository : public IStudentRepository {
public:
  void add(const Student& s) override { students_.push_back(s); }
  std::vector<Student> getAll() override { return students_; }

private:
  std::vector<Student> students_;
};

class FileStudentRepository : public IStudentRepository {
public:
  explicit FileStudentRepository(const std::string& filename) : filename_(filename) {}
  void add(const Student& s) override;
  std::vector<Student> getAll() override;

private:
  std::vector<std::string> split(const std::string& s, char delimiter);
  std::string filename_;
};

// use case layer
class AddStudentUseCase {
public:
  explicit AddStudentUseCase(IStudentRepository& repository) : repository_(repository) {}
  void execute(const Student& s)
  {
    if (s.name.empty()) {
      throw std::invalid_argument("Student name cannot be empty");
    }
    repository_.add(s);
  }

private:
  IStudentRepository& repository_;
};

class ListStudentsUseCase {
public:
  explicit ListStudentsUseCase(IStudentRepository& repository) : repository_(repository) {}
  std::vector<Student> execute() { return repository_.getAll(); }

private:
  IStudentRepository& repository_;
};

class CliController {
public:
  CliController(AddStudentUseCase& add_student_use_case, ListStudentsUseCase& list_students_use_case)
    : add_student_use_case_(add_student_use_case), list_students_use_case_(list_students_use_case)
  {
  }
  void run();

private:
  AddStudentUseCase& add_student_use_case_;
  ListStudentsUseCase& list_students_use_case_;
};

Json::Value toJson(const std::vector<Student>& students);
Json::Value parseJson(const std::string& json);

class HttpController {
public:
  HttpController(AddStudentUseCase& add_, ListStudentsUseCase& list_) : add_(add_), list_(list_) {}
  void run();

private:
  AddStudentUseCase& add_;
  ListStudentsUseCase& list_;
  httplib::Server server_;
};

class NotebookApp {
public:
  struct Config {
    std::string filename;
    bool use_file{false};
    bool use_http{false};
  };
  explicit NotebookApp(const Config& config);
  void run();

private:
  Config config_;
  std::unique_ptr<AddStudentUseCase> add_;
  std::unique_ptr<ListStudentsUseCase> list_;
  std::unique_ptr<IStudentRepository> repository_;
  std::unique_ptr<CliController> cli_controller_;
  std::unique_ptr<HttpController> http_controller_;
};
