package service

import (
	"todo"
	"todo/pkg/repo"
)

type TodoListService struct {
	repo repo.TodoList
}

func NewTodoListService(repo repo.TodoList) *TodoListService {
	return &TodoListService{repo: repo}
}

func (s *TodoListService) Create(userId int, list todo.TodoList) (int, error) {
	return s.repo.Create(userId, list)
}

func (s *TodoListService) GetAllLists(userId int) ([]todo.TodoList, error) {
	return s.repo.GetAllLists(userId)
}

func (s *TodoListService) GetListById(userId int, listId int) (todo.TodoList, error) {
	return s.repo.GetListById(userId, listId)
}
