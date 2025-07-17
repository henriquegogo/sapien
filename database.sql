PRAGMA foreign_keys=ON;
BEGIN TRANSACTION;

CREATE TABLE IF NOT EXISTS persons (
  id   INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  name TEXT    NOT NULL,
  age  INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS children (
  id   INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  name TEXT    NOT NULL,
  age  INTEGER NOT NULL,
  parent_id INTEGER NOT NULL,
  FOREIGN KEY(parent_id) REFERENCES persons(id)
);

INSERT INTO persons(name, age) VALUES ("Johnny Cage", 40);
INSERT INTO persons(name, age) VALUES ("Johnny Cash", 60);
INSERT INTO persons(name, age) VALUES ("Baby Yoda", 1211);
UPDATE persons SET name = "John Doe" WHERE id = 1;
DELETE FROM persons WHERE id = 2;

COMMIT;
