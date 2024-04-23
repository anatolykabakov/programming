package calculator

import "test/storage"

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
	c.Storage.Save("log", string(value))
	return value
}

func (c *CalculatorService) GetLastOperations(depth int) string {
	c.Storage.Get(depth)
}
