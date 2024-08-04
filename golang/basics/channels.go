package main

import (
	"fmt"
	"time"
)

func main() {
	ch := make(chan int)

	go func() {
		ch <- 42
	}()

	time.Sleep(time.Millisecond * 100)

	n := <- ch
  fmt.Println(n)
}
