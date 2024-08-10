package main

import "fmt"

func main() {
	nums := map[string]int{
		"one": 1,
		"two": 2,
		"three": 3,
	}
	fmt.Println(nums)
	nums["one"] = 2
	fmt.Println(nums)

	var uninitialized map[string]int
	fmt.Println(uninitialized)
	uninitialized["blabla"] = 1
	fmt.Println(uninitialized)
}
