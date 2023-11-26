
#include <iostream>
#include <memory>

struct Data {
  virtual void findById() = 0;
};

struct UI {
  virtual void findById() = 0;
};

struct UseCase {
  virtual void findById() = 0;
};

struct CliUI : UI {
  CliUI(std::unique_ptr<UseCase> usecase) : usecase_(std::move(usecase)) {}

  void findById() override
  {
    std::cout << "CliUI::findById()" << std::endl;
    usecase_->findById();
  }

  std::unique_ptr<UseCase> usecase_;
};

struct MyUseCase : UseCase {
  MyUseCase(std::unique_ptr<Data> dataProvider) : dataProvider_(std::move(dataProvider)) {}

  void findById() override
  {
    std::cout << "MyUseCase::findById()" << std::endl;
    dataProvider_->findById();
  }

  std::unique_ptr<Data> dataProvider_;
};

struct MyDB : Data {
  MyDB() {}

  void findById() override { std::cout << "MyDB::findById()" << std::endl; }
};

struct Config {
  int param = 0;
};

struct App {
  App(Config config) : config_(config) {}

  void run()
  {
    std::cout << "App::run()" << std::endl;
    auto db = std::make_unique<MyDB>();                         // data layer
    auto usecase = std::make_unique<MyUseCase>(std::move(db));  // logic layer
    auto ui = std::make_unique<CliUI>(std::move(usecase));      // ui/api layer
    ui->findById();
  }

  Config config_;
};

int main()
{
  App app(Config{1});
  app.run();
  return 0;
}
