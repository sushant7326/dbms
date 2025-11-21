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

enum class TokenType {
    IDENTIFIER,
    NUMBER,
    END_OF_INPUT
};

struct Token {
    TokenType type;
    string text;
};

class Tokenizer {
private:
    string input;
    vector<Token> tokens;

    void add_token(const string &s) {
        bool is_number = !s.empty() && all_of(s.begin(), s.end(), [](char c) { return isdigit( static_cast<unsigned char>(c)); });

        if (is_number) {
            tokens.push_back({TokenType::NUMBER, s});
        } else {
            tokens.push_back({TokenType::IDENTIFIER, s});
        }
    }

public:
    Tokenizer(const string &inp): input(inp) {}

    const vector<Token> &tokensize() {
        tokens.clear();
        string current;

        for (size_t i = 0; i < input.size(); i++) {
            char c = input[i];
            if (isspace(static_cast<unsigned char>(c))) {
                if (!current.empty()) {
                    add_token(current);
                    current.clear();
                }
            } else {
                current.push_back(c);
            }
        }
        
        if (!current.empty()) {
            add_token(current);
        }
        
        tokens.push_back({TokenType::END_OF_INPUT, ""});
        return tokens;
    } 
};

enum class StatementType {
    INSERT,
    SELECT,
    DELETE_STMT,
    UPDATE,
    INVALID
};

struct InsertStatement {
    int id{};
    string username;
    string email;
};

struct SelectStatement {
    bool has_where{false};
    int where_id{};
};

struct DeleteStatement {
    bool has_where{false};
    int where_id{};
};

struct UpdateStatement {
    bool has_where{false};
    int where_id{};
    string new_username;
    string new_email;
};

struct Statement {
    StatementType type{StatementType::INVALID};
    InsertStatement insertStmt;
    SelectStatement selectStmt;
    DeleteStatement deleteStmt;
    UpdateStatement updateStmt;
};