package main

import (
	"fmt"
	"errors"
)

const pi = 3.1415

func circleArea(radius int) (float32, error) {
	if radius < 0 {
		return 0, errors.New("Circle cannot have negative radius")
	}
	return float32(radius) * float32(radius) * pi, nil
}

func main() {
	radius := 3
	area, err := circleArea(radius)
	if err != nil {
		fmt.Printf(err.Error())
	}
	fmt.Printf("The area of the circle with radius %d is %f", radius, area)
}
