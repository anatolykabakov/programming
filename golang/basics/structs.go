package main

import "fmt"

type Employee struct {
	name string
	age int
}

func newEmployee(name string) *Employee {
	return &Employee{name: name}
}

func (i *Employee) info() {
	fmt.Printf("Name %s, age %d", i.name, i.age)
}

func (i *Employee) setAge(age int) {
	i.age = age
}

func main() {
	employee := newEmployee("Anatoly")
	employee.info()
	employee.setAge(27)
	employee.info()
}
