# Sapien

A simple, reflection-based REST API server for SQLite.

This application  automatically generates RESTful endpoints from a database
schema. For each table in your SQLite database, it creates a corresponding API
endpoint.

## Dependencies

To build and run this project, you will need:
- `gcc`
- `make`
- `lighttpd`
- `libfcgi-dev`
- `libsqlite3-dev`

## Setup

1.  **Initialize the database:**
    The project comes with a `database.sql` file to set up the example schema
    and data. To create the `database.db` file, run:
    ```bash
make database
    ```
    This will create a table named `persons`.

## Running the Application

To compile the application and start the web server, simply run:
```bash
make run
```
This will start a Lighttpd server on `http://localhost:8000`.

## Usage

The application automatically creates endpoints based on the tables in your
database. Based on the initial schema, the following endpoints are available.

### List Endpoints

To see a list of all available table endpoints:

```bash
curl http://localhost:8000/
```

**Response:**
```json
["persons"]
```

### Get All Records

To retrieve all records from the `persons` table:

```bash
curl http://localhost:8000/persons
```

**Response:**
```json
[{"id":1,"name":"John Doe","age":40,"_json_":"{\"id\":1,\"name\":\"John Doe\",\"age\":40}"},{"id":3,"name":"Baby Yoda","age":1211,"_json_":"{\"id\":3,\"name\":\"Baby Yoda\",\"age\":1211}"}]
```

### Get a Single Record

To retrieve a single record by its ID, use a query parameter.

```bash
curl "http://localhost:8000/persons?id=1"
```

**Response:**
```json
{"id":1,"name":"John Doe","age":40}
```

### Create a Record

To create a new record, send a `POST` request with the JSON payload in the body.

```bash
curl -X POST -d '{"name": "Luke Skywalker", "age": 23}' http://localhost:8000/persons
```

**Response:**
```json
{"status":"inserted"}
```

### Update a Record

To update an existing record, send a `PUT` request with the JSON payload and
specify the record's ID in the query string.

```bash
curl -X PUT -d '{"name": "Luke Skywalker", "age": 24}' "http://localhost:8000/persons?id=4"
```

**Response:**
```json
{"status":"updated"}
```

### Delete a Record

To delete a record, send a `DELETE` request and specify the record's ID in the
query string.

```bash
curl -X DELETE "http://localhost:8000/persons?id=4"
```

**Response:**
```json
{"status":"deleted"}
```

## Development

-   **Compile:** To compile the `sapien` executable without running the server,
use `make all` or simply `make`.
-   **Clean:** To remove the compiled executable and the database file, run
`make clean`.

## License

MIT