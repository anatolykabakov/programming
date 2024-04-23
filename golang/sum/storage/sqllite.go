
package sqllite

import (
	"fmt"
	"database/sql"
	"errors"
	"github.com/mattn/go-sqlite3"
)

type Storage struct {
	db *sql.DB
}

func New(storagePath string) (*Storage, error) {
	const op = "storage.sqlite.NewStorage" // Имя текущей функции для логов и ошибок

	db, err := sql.Open("sqlite3", storagePath) // Подключаемся к БД
	if err != nil {
			return nil, fmt.Errorf("%s: %w", op, err)
	}

	// Создаем таблицу, если ее еще нет
	stmt, err := db.Prepare(`
	CREATE TABLE IF NOT EXISTS storage(
			id INTEGER PRIMARY KEY,
			key TEXT NOT NULL,
			value TEXT NOT NULL);
	`)
	if err != nil {
			return nil, fmt.Errorf("%s: %w", op, err)
	}

	_, err = stmt.Exec()
	if err != nil {
			return nil, fmt.Errorf("%s: %w", op, err)
	}

	return &Storage{db: db}, nil
}

func (s *Storage) Save(key string, value string) (int64, error) {
	const op = "storage.sqlite.SaveURL"

	// Подготавливаем запрос
	stmt, err := s.db.Prepare("INSERT INTO storage(key, value) values(?, ?)")
	if err != nil {
			return 0, fmt.Errorf("%s: prepare statement: %w", op, err)
	}

	// Выполняем запрос
	res, err := stmt.Exec(key, value)
	if err != nil {
			if sqliteErr, ok := err.(sqlite3.Error); ok && sqliteErr.ExtendedCode == sqlite3.ErrConstraintUnique {
					return 0, fmt.Errorf("%s: %w", op, "storage.ErrURLExists")
			}

			return 0, fmt.Errorf("%s: execute statement: %w", op, err)
	}

	// Получаем ID созданной записи
	id, err := res.LastInsertId()
	if err != nil {
			return 0, fmt.Errorf("%s: failed to get last insert id: %w", op, err)
	}

	// Возвращаем ID
	return id, nil
}

func (s *Storage) Get(key string) (string, error) {
	const op = "storage.sqlite.GetURL"

	stmt, err := s.db.Prepare("SELECT data FROM storage WHERE key = ?")
	if err != nil {
			return "", fmt.Errorf("%s: prepare statement: %w", op, err)
	}

	var value string

	err = stmt.QueryRow(key).Scan(&value)
	if errors.Is(err, sql.ErrNoRows) {
			return "", nil
	}
	if err != nil {
			return "", fmt.Errorf("%s: execute statement: %w", op, err)
	}

	return value, nil
}
