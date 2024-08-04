package main

import (
	"fmt"
	"errors"
)

type Employee struct {
	Id     int
	Name   string
	Age    int
	Salary int
}

type Storage interface {
	Insert(e Employee) error
	Get(id int) (Employee, error)
	Delete(id int) error
}

type MemoryStorage struct {
	data map[int]Employee
}

func NewMemoryStorage() *MemoryStorage {
	return &MemoryStorage{data: make(map[int]Employee)}
}

func (m *MemoryStorage) Insert(e Employee) error {
	m.data[e.Id] = e
	fmt.Printf("MemoryStorage: Insert employee with id %d \n", e.Id)
	return nil
}

func (m *MemoryStorage) Get(id int) (Employee, error) {
	e, exists := m.data[id]
	if !exists {
		return Employee{}, errors.New("employee with such id does not exist")
	}
	fmt.Printf("MemoryStorage: Get employee with id %d \n", id)
	return e, nil
}

func (m *MemoryStorage) Delete(id int) error {
	delete(m.data, id)
	fmt.Printf("MemoryStorage: Delete employee with id %d \n", id)
	return nil
}

type DumpStorage struct {
}

func NewDumpStorage() *DumpStorage {
	return &DumpStorage{}
}

func (m *DumpStorage) Insert(e Employee) error {
	fmt.Printf("DumpStorage: Insert employee with id %d \n", e.Id)
	return nil
}

func (m *DumpStorage) Get(id int) (Employee, error) {
	fmt.Printf("DumpStorage: Get employee with id %d \n", id)
	e := Employee{Id: id}
	return e, nil
}

func (m *DumpStorage) Delete(id int) error {
	fmt.Printf("DumpStorage: Delete employee with id %d \n", id)
	return nil
}

func spawnEmployees(s Storage) {
	for i := 0; i < 10; i++ {
		s.Insert(Employee{Id: i})
	}
}

func main() {
	memoryStorage := NewMemoryStorage()
	dumpStorage := NewDumpStorage()
	spawnEmployees(memoryStorage)
	spawnEmployees(dumpStorage)
}
