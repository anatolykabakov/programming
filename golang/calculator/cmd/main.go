package main

import (
  "calculator/service"
  "calculator/handler"
  "calculator/storage"
  "calculator/config"
)

func main() {
	cfg := config.MustLoad()
	storage := sqllite.NewStorage(cfg.StoragePath)
	calculator := calculator.NewCalculatorService(storage)
	handler.NewCalculatorHandler(calculator, cfg.Address)
}
