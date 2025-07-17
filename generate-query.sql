BEGIN TRANSACTION;

SELECT
printf("CREATE VIEW IF NOT EXISTS %s_view AS SELECT id, json_object('id',id,%s) AS _json FROM %s;
", tbl.name, group_concat(quote(col.name) || ',' || col.name), tbl.name) ||

printf("
CREATE TRIGGER IF NOT EXISTS post_%s INSTEAD OF INSERT ON %s_view BEGIN INSERT INTO %s VALUES (NULL,%s); END;
", tbl.name, tbl.name, tbl.name, group_concat("json_extract(NEW._json,'$." || col.name || "')")) ||

printf("
CREATE TRIGGER IF NOT EXISTS put_%s INSTEAD OF UPDATE ON %s_view BEGIN UPDATE %s SET %s WHERE id=NEW.id; END;
", tbl.name, tbl.name, tbl.name,
group_concat(col.name || "=json_extract(NEW._json,'$." || col.name || "')")) ||

printf("
CREATE TRIGGER IF NOT EXISTS delete_%s INSTEAD OF DELETE ON %s_view BEGIN DELETE FROM %s WHERE id=OLD.id; END;
", tbl.name, tbl.name, tbl.name)

FROM sqlite_master AS tbl, pragma_table_info(tbl.name) AS col
WHERE tbl.type = 'table' AND tbl.name NOT LIKE 'sqlite_%' AND col.name != 'id'
GROUP BY tbl.name;

COMMIT;
