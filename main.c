#include <fcgiapp.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int handle_request(sqlite3 *db, sqlite3_stmt *stmt, FCGX_Request *request) {
  if (FCGX_Accept_r(request) != 0) return 0;

  int length = atoi(FCGX_GetParam("CONTENT_LENGTH", request->envp) ?: "0");
  const char *content_type = "Content-Type: application/json\r\n\r\n";
  const char *method = FCGX_GetParam("REQUEST_METHOD", request->envp);
  const char *path   = FCGX_GetParam("REQUEST_URI", request->envp);
  const char *query  = FCGX_GetParam("QUERY_STRING", request->envp);
  const char *select = strncmp(query, "id=", 3) ?
    "json_group_array(json(_json))" : "_json";
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
        AS _json FROM sqlite_schema WHERE type = 'view'");
  } else if (!strcmp(method, "POST")) {
    asprintf(&sql, "INSERT INTO %s(_json) VALUES ('%s')", view, body);
  } else if (!strcmp(method, "PUT")) {
    asprintf(&sql, "UPDATE %s SET _json = '%s' WHERE %s", view, body, where);
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

  while (handle_request(db, stmt, &request));

  FCGX_Finish_r(&request);

  sqlite3_finalize(stmt);
  sqlite3_close(db);

  return 0;
}
