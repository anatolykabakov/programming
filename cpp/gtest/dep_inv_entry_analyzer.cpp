// ref https://habr.com/ru/companies/tensor/articles/347358/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>
#include <thread>

struct IDBManager {
  virtual bool isValid(const std::string&) = 0;
  virtual ~IDBManager() = default;
};

struct IWebService {
  virtual void LogError(const std::string& msg) = 0;
  virtual ~IWebService() = default;
};

struct DatabaseManager : IDBManager {
  bool isValid(const std::string& name)
  {
    using namespace std::chrono_literals;
    // read from database. long work.
    std::this_thread::sleep_for(5s);
    return true;
  }
};

struct WebService : IWebService {
  void LogError(const std::string& msg)
  {
    using namespace std::chrono_literals;
    // logic, includes working with network.
    std::this_thread::sleep_for(2s);
  }
};

class DIEntryAnalyzer {
public:
  DIEntryAnalyzer(std::unique_ptr<IDBManager> dbmanager, std::shared_ptr<IWebService> web)
    : dbManager_(std::move(dbmanager)), webService_(std::move(web))
  {
  }
  bool Analyze(const std::string& name)
  {
    if (name == "ON") {
      webService_->LogError("Error!!! " + name);
    }
    return dbManager_->isValid(name);
  }

private:
  std::unique_ptr<IDBManager> dbManager_;
  std::shared_ptr<IWebService> webService_;
};

struct FakeDBManager : IDBManager {
  bool isValid(const std::string& name) { return true; }
};

struct FakeWebService : IWebService {
  FakeWebService(bool called) {}
  void LogError(const std::string& msg) { lastErr = msg; }

  std::string lastErr;
};

TEST(DIEntryAnalyzer, AnalyzeTest)
{
  auto web = std::make_shared<FakeWebService>(true);
  DIEntryAnalyzer analyzer(std::make_unique<FakeDBManager>(), web);
  EXPECT_TRUE(analyzer.Analyze("ON"));
  EXPECT_EQ("Error!!! ON", web->lastErr);
}
