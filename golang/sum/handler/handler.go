package handler

import (
	"fmt"
	"net/http"
	"encoding/json"
  "test/service"
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

type CalculatorHanlder struct {
	Calculator *calculator.CalculatorService
}

func (h *CalculatorHanlder) HandleCalc(rw http.ResponseWriter, req * http.Request) {
	var t Request
	err := json.NewDecoder(req.Body).Decode(&t)
	if (err != nil) {
		panic(err)
	}
	fmt.Println(t.A, t.B)

	responce := h.Calculator.Calculate(t.A, t.B, t.Operation)

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
	fmt.Println(t.Depth)

  result := h.Calculator.GetLastOperations(t.Depth)

	responce_m, _ := json.Marshal(result)

	rw.Header().Set("Content-Type", "application/json")
	rw.WriteHeader(http.StatusOK)
	rw.Write([]byte(responce_m))
}
