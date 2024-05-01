
package sqllite

import (
	"fmt"
	"database/sql"

	_ "github.com/mattn/go-sqlite3"
)

type Storage struct {
	db *sql.DB
}

func NewStorage(storagePath string) *Storage {
	db, err := sql.Open("sqlite3", storagePath)
	if err != nil {
		panic(err)
	}

	stmt, err := db.Prepare(`
	CREATE TABLE IF NOT EXISTS storage(
			id INTEGER PRIMARY KEY,
			key TEXT NOT NULL,
			value TEXT NOT NULL);
	`)
	if err != nil {
		panic(err)
	}

	_, err = stmt.Exec()
	if err != nil {
		panic(err)
	}

	return &Storage{db: db}
}

func (s *Storage) Save(key string, value string) (int64, error) {
	const op = "storage.sqlite.Save"
	fmt.Println(key, value)

	stmt, err := s.db.Prepare("INSERT INTO storage(key, value) values(?, ?)")
	if err != nil {
			return 0, fmt.Errorf("%s: prepare statement: %w", op, err)
	}

	res, _ := stmt.Exec(key, value)
	if err != nil {
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
