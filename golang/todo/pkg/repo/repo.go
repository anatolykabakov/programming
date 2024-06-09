package repo

import (
	"github.com/jmoiron/sqlx"
	"todo"
)

type Authorization interface {
	CreateUser(user todo.User) (int, error)
	GetUser(usename string, password string) (todo.User, error)
}

type TodoList interface {
	Create(userId int, list todo.TodoList) (int, error)
	GetAllLists(userId int) ([]todo.TodoList, error)
	GetListById(userId int, listId int) (todo.TodoList, error)
}

type TodoItem interface {

}

type Repository struct {
	Authorization
	TodoList
	TodoItem
}

func NewRepository(db *sqlx.DB) *Repository {
	return &Repository{
		Authorization: NewAuthRepository(db),
		TodoList: NewTodoListPostgres(db),
	}
}
