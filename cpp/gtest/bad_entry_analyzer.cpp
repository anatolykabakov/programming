// ref https://habr.com/ru/companies/tensor/articles/347358/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>
#include <thread>

struct DatabaseManager {
  bool isValid(const std::string& name)
  {
    using namespace std::chrono_literals;
    // read from database. long work.
    std::this_thread::sleep_for(5s);
    return true;
  }
};

struct WebService {
  void LogError(const std::string& msg)
  {
    using namespace std::chrono_literals;
    // logic, includes working with network
    std::this_thread::sleep_for(2s);
  }
};

class EntryAnalyzerBad {
public:
  bool Analyze(const std::string& name)
  {
    if (name == "ON") {
      webService_.LogError("Error!!!");
    }
    return dbManager_.isValid(name);
  }

private:
  DatabaseManager dbManager_;
  WebService webService_;
};

TEST(EntryAnalyzerBad, AnalyzeTest)
{
  EntryAnalyzerBad analyzer;
  EXPECT_TRUE(analyzer.Analyze("ON"));
}
