package calculator

import (
	"fmt"
)

type StorageInterface interface {
	Save(key string, value string) (int64, error)
	Get(depth int) ([]string, error)
}

type CalculatorService struct {
	storage StorageInterface
}

func NewCalculatorService(s StorageInterface) *CalculatorService {
	return &CalculatorService{storage: s}
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
	c.storage.Save("log", res)
	return value
}

func (c *CalculatorService) GetLastOperations(depth int) []string {
	results, _ := c.storage.Get(depth)
	return results
}
