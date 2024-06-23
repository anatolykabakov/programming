package main

import (
	"fmt"
	"sync"
)

func race_foo(counter *int, ch chan bool) {
	for i := 0; i < 1000000; i++ {
		*counter++
	}
	ch <- true
}

func safe_foo(counter *int, ch chan bool, mutex *sync.Mutex) {
	for i := 0; i < 1000000; i++ {
		mutex.Lock()
		*counter++
		mutex.Unlock()
	}
	ch <- true
}

func race() {
	counter := 0

	ch := make(chan bool)

	for i := 0; i < 3; i++ {
		go race_foo(&counter, ch)
	}

	for i := 0; i < 3; i++ {
		<- ch
	}

	fmt.Println(counter)
}

func safe() {
	counter := 0

	ch := make(chan bool)

	var mutex sync.Mutex

	for i := 0; i < 3; i++ {
		go safe_foo(&counter, ch, &mutex)
	}

	for i := 0; i < 3; i++ {
		<- ch
	}

	fmt.Println(counter)
}

func main() {
	race()
	safe()
}
