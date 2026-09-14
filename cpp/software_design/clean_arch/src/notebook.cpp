#include "notebook.h"

#include <iostream>
#include <json/reader.h>
#include <limits>

void FileStudentRepository::add(const Student& s)
{
  std::ofstream file(filename_, std::ios::app);
  file << s.id << "," << s.name << std::endl;
}

std::vector<std::string> FileStudentRepository::split(const std::string& s, char delimiter)
{
  std::vector<std::string> result;
  std::string word;
  for (char ch : s) {
    if (ch == delimiter) {
      result.push_back(word);
      word.clear();
      continue;
    }
    word += ch;
  }
  result.push_back(word);
  return result;
}

std::vector<Student> FileStudentRepository::getAll()
{
  std::ifstream file(filename_, std::ios::in);
  std::vector<Student> students;

  std::string line;
  while (std::getline(file, line)) {
    if (line.empty()) {
      continue;
    }
    auto parts = split(line, ',');
    students.push_back(Student{std::stoi(parts[0]), parts[1]});
  }
  return students;
}

Json::Value toJson(const std::vector<Student>& students)
{
  Json::Value result(Json::arrayValue);
  for (const auto& student : students) {
    Json::Value item;
    item["id"] = student.id;
    item["name"] = student.name;
    result.append(item);
  }
  return result;
}

Json::Value parseJson(const std::string& json)
{
  Json::Value result;
  Json::Reader reader;
  reader.parse(json, result);
  return result;
}

void CliController::run()
{
  while (true) {
    std::cout << "Choose action:\n";
    std::cout << "1. Add student\n";
    std::cout << "2. Print students\n";
    std::cout << "3. Exit\n";

    int action;
    std::cin >> action;
    if (!std::cin) {
      return;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    switch (action) {
      case 1: {
        std::cout << "Enter student name:\n";
        std::string name;
        std::getline(std::cin, name);
        try {
          add_student_use_case_.execute(Student{0, name});
        } catch (const std::invalid_argument& e) {
          std::cout << e.what() << '\n';
        }
        break;
      }
      case 2: {
        auto students = list_students_use_case_.execute();
        for (const auto& student : students) {
          std::cout << student.name << '\n';
        }
        break;
      }
      case 3:
        return;
      default:
        std::cout << "Invalid action\n";
        break;
    }
  }
}

void HttpController::run()
{
  server_.Get("/students", [this](const httplib::Request&, httplib::Response& res) {
    auto students = list_.execute();
    const auto json = toJson(students).toStyledString();
    res.set_content(json, "application/json");
  });

  server_.Post("/students", [this](const httplib::Request& req, httplib::Response& res) {
    auto body = parseJson(req.body);
    if (!body.isMember("name") || !body["name"].isString()) {
      res.status = 400;
      res.set_content(R"({"error":"name required"})", "application/json");
      return;
    }

    try {
      add_.execute(Student{0, body["name"].asString()});
    } catch (const std::invalid_argument&) {
      res.status = 400;
      res.set_content(R"({"error":"name cannot be empty"})", "application/json");
      return;
    } catch (const std::exception&) {
      res.status = 500;
      res.set_content(R"({"error":"internal server error"})", "application/json");
      return;
    }

    res.status = 200;
    res.set_content(R"({"message":"student added"})", "application/json");
  });

  std::cout << "Server is running on http://0.0.0.0:8080\n";
  server_.listen("0.0.0.0", 8080);
  std::cout << "Server is stopped\n";
}

NotebookApp::NotebookApp(const Config& config) : config_(config)
{
  if (config_.use_file) {
    repository_ = std::make_unique<FileStudentRepository>(config_.filename);
  } else {
    repository_ = std::make_unique<InMemoryStudentRepository>();
  }
  add_ = std::make_unique<AddStudentUseCase>(*repository_);
  list_ = std::make_unique<ListStudentsUseCase>(*repository_);
  if (config_.use_http) {
    http_controller_ = std::make_unique<HttpController>(*add_, *list_);
  } else {
    cli_controller_ = std::make_unique<CliController>(*add_, *list_);
  }
}

void NotebookApp::run()
{
  if (config_.use_http) {
    http_controller_->run();
  } else {
    cli_controller_->run();
  }
}
