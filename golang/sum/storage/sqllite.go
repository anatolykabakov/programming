
package sqllite

import (
	"fmt"
	"database/sql"
	"github.com/mattn/go-sqlite3"
)

type Storage struct {
	db *sql.DB
}

func New(storagePath string) (*Storage, error) {
	const op = "storage.sqlite.NewStorage"

	db, err := sql.Open("sqlite3", storagePath)
	if err != nil {
			return nil, fmt.Errorf("%s: %w", op, err)
	}

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
	const op = "storage.sqlite.Save"

	stmt, err := s.db.Prepare("INSERT INTO storage(key, value) values(?, ?)")
	if err != nil {
			return 0, fmt.Errorf("%s: prepare statement: %w", op, err)
	}

	res, err := stmt.Exec(key, value)
	if err != nil {
			if sqliteErr, ok := err.(sqlite3.Error); ok && sqliteErr.ExtendedCode == sqlite3.ErrConstraintUnique {
					return 0, fmt.Errorf("%s: %w", op, "storage.ErrURLExists")
			}
			return 0, fmt.Errorf("%s: execute statement: %w", op, err)
	}

	id, err := res.LastInsertId()
	if err != nil {
			return 0, fmt.Errorf("%s: failed to get last insert id: %w", op, err)
	}

	return id, nil
}

func (s *Storage) Get(depth int) ([]string, error) {
	rows, err := s.db.Query("SELECT value FROM storage")
	if err != nil {
		panic(err)
	}
	defer rows.Close()
	var answer []string
	for rows.Next() {
		var value string
		err := rows.Scan(&value)
		if err != nil {
			panic(err)
		}
		answer = append(answer, value)
		fmt.Println(value)
	}

	return answer, nil
}
