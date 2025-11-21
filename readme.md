## DBMS

### INSERT
```bash
db > INSERT 1 abhinav abhinav@example.com
Row insertion successful.

db > INSERT 2 raj raj@example.com
Row insertion successful.
```

### SELECT
```bash
db > SELECT
(1, abhinav, abhinav@example.com)
(2, raj, raj@example.com)
Rows: 2

db > SELECT WHERE id = 2
(2, raj, raj@example.com)
Rows: 1
```

### UPDATE
```bash
db > UPDATE WHERE id = 2 raj_updated raj2@example.com
1 row(s) updated successfully.
```

### DELETE
```bash
db > DELETE WHERE id = 1
1 row(s) deleted successfully.

db > SELECT
(2, raj_updated, raj2@example.com)
Rows: 1
```

### EXIT
```bash
db > .exit
Exiting Program.
```