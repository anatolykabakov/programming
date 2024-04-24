package main

import (
	// "net/http"
  "calculator/service"
  "calculator/handler"
  "calculator/storage"
)

func main() {
	storage, _ := sqllite.New("db.sql")
	calculator := calculator.NewCalculatorService(storage)
	handler.NewCalculatorHandler(calculator)
}
