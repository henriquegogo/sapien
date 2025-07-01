all:
	@gcc -Wno-unused-result -O2 -o sapien main.c -lfcgi -lsqlite3

run: all
	@lighttpd -D -f lighttpd.conf

database:
	@sqlite3 database.db < database.sql

clean:
	@rm -f database.db
	@rm -f sapien
