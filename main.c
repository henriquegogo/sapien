#include <fcgiapp.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void setup_database(sqlite3 *db) {
  sqlite3_stmt *stmt = NULL;

  char *sql = strdup("SELECT name FROM sqlite_schema WHERE type = 'table'");
  sqlite3_prepare_v2(db, sql, -1, &stmt, 0), free(sql);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *table = (const char *)sqlite3_column_text(stmt, 0);
    if (!strncmp(table, "sqlite_", 7) || !strncmp(table, "_", 1)) continue;

    sqlite3_stmt *cols_stmt = NULL;
    char *select_attrs = strdup("");
    char *insert_attrs = strdup("");
    char *update_attrs = strdup("");

    asprintf(&sql, "PRAGMA table_info(%s)", table);
    sqlite3_prepare_v2(db, sql, -1, &cols_stmt, 0), free(sql);

    while (sqlite3_step(cols_stmt) == SQLITE_ROW) {
      const char *col = (const char *)sqlite3_column_text(cols_stmt, 1);
      char *tmp = NULL;

      if (select_attrs[0]) {
        asprintf(&select_attrs, "%s%s", tmp = select_attrs, ","), free(tmp);
      }
      asprintf(&select_attrs, "%s\n  '%s', %s", tmp = select_attrs, col, col);
      free(tmp);

      if (!strcmp(col, "id")) continue;

      if (insert_attrs[0]) {
        asprintf(&insert_attrs, "%s%s", tmp = insert_attrs, ","), free(tmp);
      }
      asprintf(&insert_attrs, "%s\n    json_extract(NEW._json_, '$.%s')", 
          tmp = insert_attrs, col), free(tmp);

      if (update_attrs[0]) {
        asprintf(&update_attrs, "%s%s", tmp = update_attrs, ","), free(tmp);
      }
      asprintf(&update_attrs, "%s\n    %s = json_extract(NEW._json_, '$.%s')",
          tmp = update_attrs, col, col), free(tmp);
    }

    sqlite3_finalize(cols_stmt);

    asprintf(&sql, "CREATE VIEW IF NOT EXISTS %s_view AS\n\
SELECT *, json_object(%s\n) AS _json_ FROM %s", table, select_attrs, table);
    sqlite3_exec(db, sql, 0, 0, 0), free(sql), free(select_attrs);

    asprintf(&sql, "CREATE TRIGGER IF NOT EXISTS post_%s INSTEAD OF\n\
INSERT ON %s_view BEGIN\n\
  INSERT INTO %s\n\
  SELECT NULL, %s;\n\
END", table, table, table, insert_attrs), free(insert_attrs);
    sqlite3_exec(db, sql, 0, 0, 0), free(sql);

    asprintf(&sql, "CREATE TRIGGER IF NOT EXISTS put_%s INSTEAD OF\n\
UPDATE ON %s_view BEGIN\n\
  UPDATE %s SET %s\n\
  WHERE id = NEW.id;\n\
END", table, table, table, update_attrs), free(update_attrs);
    sqlite3_exec(db, sql, 0, 0, 0), free(sql);

    asprintf(&sql, "CREATE TRIGGER IF NOT EXISTS delete_%s INSTEAD OF\n\
DELETE ON %s_view BEGIN\n\
  DELETE FROM %s WHERE id = OLD.id;\n\
END", table, table, table);
    sqlite3_exec(db, sql, 0, 0, 0), free(sql);
  }

  sqlite3_finalize(stmt);
}

int handle_request(sqlite3 *db, sqlite3_stmt *stmt, FCGX_Request *request) {
  if (FCGX_Accept_r(request) != 0) return 0;

  int length = atoi(FCGX_GetParam("CONTENT_LENGTH", request->envp) ?: "0");
  const char *content_type = "Content-Type: application/json\r\n\r\n";
  const char *method = FCGX_GetParam("REQUEST_METHOD", request->envp);
  const char *path   = FCGX_GetParam("REQUEST_URI", request->envp);
  const char *query  = FCGX_GetParam("QUERY_STRING", request->envp);
  const char *select = strncmp(query, "id=", 3) ?
    "json_group_array(json(_json_))" : "_json_";
  const char *where = strdup(query[0] ? query: "TRUE");
  char *sql = strdup(""), *view = strdup(""), *body = calloc(length + 1, 1);

  FCGX_GetStr(body, length, request->in);

  if (path && strcmp(path, "/") != 0) {
    const char *start = path + 1, *end = strchr(start, '?');
    size_t len = end ? (size_t)(end - start) : strlen(start);
    asprintf(&view, "%.*s_view", (int)len, start);
  }

  if (!view[0]) {
    asprintf(&sql, "SELECT json_group_array(REPLACE(name, '_view', '')) \
        AS _json_ FROM sqlite_schema WHERE type = 'view'");
  } else if (!strcmp(method, "POST")) {
    asprintf(&sql, "INSERT INTO %s(_json_) VALUES ('%s')", view, body);
  } else if (!strcmp(method, "PUT")) {
    asprintf(&sql, "UPDATE %s SET _json_ = '%s' WHERE %s", view, body, where);
  } else if (!strcmp(method, "DELETE")) {
    asprintf(&sql, "DELETE FROM %s WHERE %s", view, where);
  } else {
    asprintf(&sql, "SELECT %s FROM %s WHERE %s", select, view, where);
  }

  int status = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
  free(view), free(body), free(sql); 

  if (status != SQLITE_OK) {
    FCGX_FPrintF(request->out, "Status: 400 Bad Request\r\n%s", content_type);
    return FCGX_FPrintF(request->out, "{\"error\":\"%s\"}", sqlite3_errmsg(db));
  } else if (!strcmp(method, "POST") && sqlite3_step(stmt)) {
    FCGX_FPrintF(request->out, "Status: 201 Created\r\n%s", content_type);
    FCGX_FPrintF(request->out, "{\"status\":\"inserted\"}\n");
  } else if (!strcmp(method, "PUT") && sqlite3_step(stmt)) {
    FCGX_FPrintF(request->out, "Status: 200 OK\r\n%s", content_type);
    FCGX_FPrintF(request->out, "{\"status\":\"updated\"}\n");
  } else if (!strcmp(method, "DELETE") && sqlite3_step(stmt)) {
    FCGX_FPrintF(request->out, "Status: 200 OK\r\n%s", content_type);
    FCGX_FPrintF(request->out, "{\"status\":\"deleted\"}\n");
  } else if (sqlite3_step(stmt)) {
    FCGX_FPrintF(request->out, "Status: 200 OK\r\n%s", content_type);
    FCGX_FPrintF(request->out, "%s\n", sqlite3_column_text(stmt, 0));
  }

  return 1;
}

int main() {
  sqlite3 *db;
  sqlite3_stmt *stmt;
  sqlite3_open("database.db", &db);

  FCGX_Init();
  FCGX_Request request;
  FCGX_InitRequest(&request, 0, 0);

  setup_database(db);
  while (handle_request(db, stmt, &request));

  FCGX_Finish_r(&request);

  sqlite3_finalize(stmt);
  sqlite3_close(db);

  return 0;
}
