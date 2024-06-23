package main

import (
	"fmt"
	"time"
)

func doWork(t int, res chan int) {
	time.Sleep(time.Millisecond * 1000)
	res <- t
}

func main() {
	ch1 := make(chan int)
	t := time.Now()
	fmt.Printf("Start: %s\n", t.Format(time.RFC3339))
	go doWork(1000, ch1)
	go doWork(2000, ch1)
	fmt.Println(<- ch1)
	fmt.Println(<- ch1)
	fmt.Printf("Since: %s\n", time.Since(t))
}
