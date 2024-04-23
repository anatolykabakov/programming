package main

import (
	"net/http"
  "test/service"
  "test/handler"
  "test/storage"
)

func main() {
	storage, _ := sqllite.New("db.sql")
	calculator_service := &calculator.CalculatorService{Storage: storage}
	hanlder := &handler.CalculatorHanlder{Calculator: calculator_service}
	http.HandleFunc("/calculator", hanlder.HandleCalc)
	http.HandleFunc("/history", hanlder.HandleHistory)
	http.ListenAndServe(":8080", nil)
}
