#include <bits/stdc++.h>
using namespace std;

class Row {
public:
    int id{};
    string username;
    string email;

    Row() = default;

    Row(int _id, const string &_u, const string &_e): id(_id), username(_u), email(_e) {}
};

class Table {
private:
    vector<Row> rows;
    size_t max_rows;

public:
    Table(size_t maxRows = 1000) : max_rows(maxRows) {}

    bool insert_row(const Row &r) {
        if (rows.size() >= max_rows) return false;
        rows.push_back(r);
        return true;
    }

    void select_all() const {
        for (const auto &r: rows) {
            cout << "(" << r.id << ", " << r.username << ", " << r.email << ")\n";
        }
        cout << "Rows: " << rows.size() << endl;
    }

    int select_where_id(int id) const {
        int count = 0;
        for (const auto &r: rows) {
            if (r.id == id) {
                cout << "(" << r.id << ", " << r.username << ", " << r.email << ")\n";
                count++;
            }
        }
        cout << "Rows: " << count << endl;
        return count;
    }

    int delete_where_id (int id) {
        int deleted = 0;
        for (size_t i = 0; i < rows.size(); ) {
            if (rows[i].id == id) {
                rows.erase(rows.begin() + i);
                deleted++;
            } else {
                i++;
            }
        }
        return deleted; 
    }

    int update_where_id (int id, const string &new_username, const string &new_email) {
        int updated = 0;
        for (auto &r: rows) {
            if (r.id == id) {
                r.username = new_username;
                r.email = new_email;
                updated++;
            }
        }
        return updated;
    }
};