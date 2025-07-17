all:
	@$(CC) -Wno-unused-result -O2 -o sapien main.c -lfcgi -lsqlite3

run: all
	@lighttpd -D -f lighttpd.conf

database:
	@rm -f database.db
	@sqlite3 database.db < database.sql
	@sqlite3 database.db < generate-query.sql | sqlite3 database.db

clean:
	@rm -f database.db
	@rm -f sapien
