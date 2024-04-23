package main

import (
	"fmt"
	"net/http"
	"encoding/json"
  "test/storage"
)

type Request struct {
	A int
	B int
}

type Responce struct {
	Sum int
}

type Sum struct {}

func (s *Sum) HandleSum(rw http.ResponseWriter, req * http.Request) {
	var t Request
	err := json.NewDecoder(req.Body).Decode(&t)
	if (err != nil) {
		panic(err)
	}
	fmt.Println(t.A, t.B)

	var responce Responce
	responce.Sum = t.A + t.B

	responce_m, _ := json.Marshal(responce)

	rw.Header().Set("Content-Type", "application/json")
	rw.WriteHeader(http.StatusOK)
	rw.Write([]byte(responce_m))
}


func main() {
	sqllite.New("db.sql")
	sum := &Sum{}
	// slog.Info("hello, world")
	http.HandleFunc("/sum", sum.HandleSum)
	http.ListenAndServe(":8080", nil)
}
