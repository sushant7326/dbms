## DBMS

### INSERT
```bash
db > INSERT sushant sushant@123.com
Row insertion successful.

db > INSERT singh singh@123.com
Row insertion successful.
```

### SELECT
```bash
db > SELECT
(1, sushant, sushant@123.com)
(2, singh, singh@123.com)
Rows: 2

db > SELECT WHERE id = 2
(2, singh, singh@123.com)
Rows: 1
```

### UPDATE
```bash
db > UPDATE WHERE id = 2 singh_updated singh2@123.com
1 row(s) updated successfully.
```

### DELETE
```bash
db > DELETE WHERE id = 1
1 row(s) deleted successfully.

db > SELECT
(2, singh_updated, singh2@123.com)
Rows: 1
```

### EXIT
```bash
db > .exit
Exiting Program.
```