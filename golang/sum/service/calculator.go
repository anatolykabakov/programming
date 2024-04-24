package calculator

import (
	"test/storage"
	"fmt"
)

type CalculatorService struct {
	Storage *sqllite.Storage
}

func (c *CalculatorService) Calculate(a int, b int, operation string) int {
	var value int = 0
 	if operation == "+" {
		value = a + b
	}
	if operation == "-" {
		value = a - b
	}
	if operation == "*" {
		value = a * b
	}
	if operation == "/" {
		value = a / b
	}
	res := fmt.Sprintf("%d %s %d = %d", a, operation, b, value)
	c.Storage.Save("log", res)
	return value
}

func (c *CalculatorService) GetLastOperations(depth int) []string {
	results, _ := c.Storage.Get(depth)
	return results
}
