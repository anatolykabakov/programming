package handler

import (
	"net/http"
	"encoding/json"
)

type Request struct {
	A int
	B int
	Operation string
}

type HistoryRequest struct {
	Depth int
}

type HistoryResponce struct {
	Value string
}

type Responce struct {
	Value int
}

type CalculatorInterface interface {
	Calculate(a int, b int, operation string) int
	GetLastOperations(depth int) []string
}

type CalculatorHanlder struct {
	Calculator CalculatorInterface
}

func NewCalculatorHandler(calculator_service CalculatorInterface, address string) {
	rest := &CalculatorHanlder{Calculator: calculator_service}
	http.HandleFunc("/calculator", rest.HandleCalc)
	http.HandleFunc("/history", rest.HandleHistory)
	http.ListenAndServe(address, nil)
}

func (h *CalculatorHanlder) HandleCalc(rw http.ResponseWriter, req * http.Request) {
	var t Request
	err := json.NewDecoder(req.Body).Decode(&t)
	if (err != nil) {
		panic(err)
	}
  var responce Responce
	responce.Value = h.Calculator.Calculate(t.A, t.B, t.Operation)

	responce_m, _ := json.Marshal(responce)

	rw.Header().Set("Content-Type", "application/json")
	rw.WriteHeader(http.StatusOK)
	rw.Write([]byte(responce_m))
}

func (h *CalculatorHanlder) HandleHistory(rw http.ResponseWriter, req * http.Request) {
	var t HistoryRequest
	err := json.NewDecoder(req.Body).Decode(&t)
	if (err != nil) {
		panic(err)
	}

  result := h.Calculator.GetLastOperations(t.Depth)

	responce_m, _ := json.Marshal(result)

	rw.Header().Set("Content-Type", "application/json")
	rw.WriteHeader(http.StatusOK)
	rw.Write([]byte(responce_m))
}
