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

class EOEntryAnalyzer {
public:
  bool Analyze(const std::string& name)
  {
    if (name == "ON") {
      LogError("Error!!! " + name);
    }
    return isValid(name);
  }

protected:
  virtual bool isValid(const std::string& name) { return dbManager_.isValid(name); }

  virtual void LogError(const std::string& msg) { webService_.LogError("Error!!!"); }

private:
  DatabaseManager dbManager_;
  WebService webService_;
};

struct TestingEntryAnalyzer : public EOEntryAnalyzer {
  bool willBeValid{false};
  std::string lastErr;

  TestingEntryAnalyzer(bool answer) : willBeValid{answer} {}

private:
  bool isValid(const std::string& name) { return willBeValid; }

  void LogError(const std::string& msg) { lastErr = msg; }
};

TEST(EOEntryAnalyzer, AnalyzeReturnsTrueTest)
{
  TestingEntryAnalyzer mock_analyzer(true);
  EXPECT_TRUE(mock_analyzer.Analyze("ON"));
  EXPECT_EQ("Error!!! ON", mock_analyzer.lastErr);
}
