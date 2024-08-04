package main

import "fmt"

func main() {
	todoList := [4]string{
		"1",
		"2",
		"3",
		"4",
	}

	slice := todoList[1:4]
	fmt.Println(slice)

	changeByRef(slice)

	fmt.Println(todoList)

	slice = append(slice, "12", "11")
	slice = append(slice, "13")
	fmt.Println(slice)

	fmt.Println(todoList)
}

func changeByRef(tasks []string) {
	tasks[0] = "6"
	tasks[1] = "7"
}
