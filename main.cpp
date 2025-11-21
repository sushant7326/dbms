#include <bits/stdc++.h>
using namespace std;

string to_lower_copy(const string &s) {
    string t = s;
    for (char &c : t) c = tolower(static_cast<unsigned char>(c));
    return t;
}

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

class Parser {
private:
    const vector<Token> &tokens;
    size_t pos;

    const Token &current() const {
        return tokens[pos];
    }

    void advance() {
        if (pos < tokens.size()) pos++;
    }

    bool match_identifier_ci(const string &expected) {
        if (current().type != TokenType::IDENTIFIER) return false;
        string got = to_lower_copy(current().text);
        if (got == expected) {
            advance();
            return true;
        }
        return false;
    }

    bool expect_number(int &value_out) {
        if (current().type != TokenType::NUMBER) return false;
        value_out = stoi(current().text);
        advance();
        return true;
    }

    bool expect_identifier(string &text_out) {
        if (current().type != TokenType::IDENTIFIER) return false;
        text_out = current().text;
        advance();
        return true;
    }

    void parse_insert(InsertStatement &ins) {
        if (!expect_number(ins.id)) {
            ins.id = -1;
            return;
        }
        if (!expect_identifier(ins.username)) {
            return;
        }
        if (!expect_identifier(ins.email)) {
            return;
        }
    }

    void parse_select(SelectStatement &sel) {
        sel.has_where = false;

        if (match_identifier_ci("where")) {
            if (!match_identifier_ci("id")) return;
            if (!match_identifier_ci("=")) return;

            int value;
            if (!expect_number(value)) return;
            sel.has_where = true;
            sel.where_id = value;
        }
    }

    void parse_delete(DeleteStatement &del) {
        del.has_where = false;

        if (match_identifier_ci("where")) {
            if (!match_identifier_ci("id")) return;
            if (!match_identifier_ci("=")) return;

            int value;
            if (!expect_number(value)) return;
            del.has_where = true;
            del.where_id = value;
        }
    }

    void parse_update(UpdateStatement &upd) {
        upd.has_where = false;

        if (match_identifier_ci("where")) {
            if (!match_identifier_ci("id")) return;
            if (!match_identifier_ci("=")) return;

            int value;
            if (!expect_number(value)) return;
            upd.has_where = true;
            upd.where_id = value;
        } else {
            return;
        }

        if (!expect_identifier(upd.new_username)) return;
        if (!expect_identifier(upd.new_email)) return;
    }

public:
    Parser(const vector<Token> &toks): tokens(toks), pos(0) {}

    Statement parse_statement() {
        Statement stmt;

        if (current().type == TokenType::END_OF_INPUT) {
            stmt.type = StatementType::INVALID;
            return stmt;
        }

        if (current().type != TokenType::IDENTIFIER) {
            stmt.type = StatementType::INVALID;
            return stmt;
        }

        string first = to_lower_copy(current().text);

        if (first == "insert") {
            advance();
            stmt.type = StatementType::INSERT;
            parse_insert(stmt.insertStmt);
        } else if (first == "select") {
            advance();
            stmt.type = StatementType::SELECT;
            parse_select(stmt.selectStmt);
        } else if (first == "delete") {
            advance();
            stmt.type = StatementType::DELETE_STMT;
            parse_delete(stmt.deleteStmt);
        } else if (first == "update") {
            advance();
            stmt.type = StatementType::UPDATE;
            parse_update(stmt.updateStmt);
        } else {
            stmt.type = StatementType::INVALID;
        }

        return stmt;
    }
};

enum class OpCode {
    OP_INSERT,
    OP_SELECT_ALL,
    OP_SELECT_WHERE_ID,
    OP_DELETE_WHERE_ID,
    OP_UPDATE_WHERE_ID,
    OP_HALT
};

struct Instruction {
    OpCode op;
    int id_operand{0};
    string username_operand;
    string email_operand;
};

class Program {
public:
    vector<Instruction> code;

    void add(const Instruction &ins) {
        code.push_back(ins);
    }
};

class Compiler {
public:
    static Program compile(const Statement &stmt) {
        Program prog;

        switch (stmt.type) {
            case StatementType::INSERT: {
                Instruction ins;
                ins.op = OpCode::OP_INSERT;
                ins.id_operand = stmt.insertStmt.id;
                ins.username_operand = stmt.insertStmt.username;
                ins.email_operand = stmt.insertStmt.email;
                prog.add(ins);
                break;
            } case StatementType::SELECT: {
                if (stmt.selectStmt.has_where) {
                    Instruction ins;
                    ins.op = OpCode::OP_SELECT_WHERE_ID;
                    ins.id_operand = stmt.selectStmt.where_id;
                    prog.add(ins);
                } else {
                    Instruction ins;
                    ins.op = OpCode::OP_SELECT_ALL;
                    prog.add(ins);
                }
                break;
            } case StatementType::DELETE_STMT: {
                if (stmt.deleteStmt.has_where) {
                    Instruction ins;
                    ins.op = OpCode::OP_DELETE_WHERE_ID;
                    ins.id_operand = stmt.deleteStmt.where_id;
                    prog.add(ins);
                }
                break;
            } case StatementType::UPDATE: {
                if (stmt.updateStmt.has_where) {
                    Instruction ins;
                    ins.op = OpCode::OP_UPDATE_WHERE_ID;
                    ins.id_operand = stmt.updateStmt.where_id;
                    ins.username_operand = stmt.updateStmt.new_username;
                    ins.email_operand = stmt.updateStmt.new_email;
                    prog.add(ins);
                }
                break;
            } default: {
                break;
            }

            Instruction halt;
            halt.op = OpCode::OP_HALT;
            prog.add(halt);

            return prog;
        }
    }
};

class VirtualMachine {
private:
    Table &table;

public:
    VirtualMachine(Table &t): table(t) {}

    void execute(const Program &prog) {
        size_t pc = 0;
        while (pc < prog.code.size()) {
            const Instruction &ins = prog.code[pc];
            switch (ins.op) {
                case OpCode::OP_INSERT: {
                    Row r(ins.id_operand, ins.username_operand, ins.email_operand);
                    bool ok = table.insert_row(r);
                    if (ok) {
                        cout << "Row insertion successful." << endl;
                    } else {
                        cout << "Error: Table full, row insertion failed." << endl;
                    }
                    pc++;
                    break;
                } case OpCode::OP_SELECT_ALL: {
                    table.select_all();
                    pc++;
                    break;
                } case OpCode::OP_SELECT_WHERE_ID: {
                    table.select_where_id(ins.id_operand);
                    pc++;
                    break;
                } case OpCode::OP_UPDATE_WHERE_ID: {
                    int updated = table.update_where_id(
                        ins.id_operand,
                        ins.username_operand,
                        ins.email_operand
                    );
                    cout << updated << "row(s) updated successfully." << endl;
                    pc++;
                    break;
                } case OpCode::OP_DELETE_WHERE_ID: {
                    int deleted = table.delete_where_id(ins.id_operand);
                    cout << deleted << "row(s) deleted successfully." << endl;
                    pc++;
                    break;
                } case OpCode::OP_HALT: {
                    return;
                }
            }
        }
    }
};

class DatabaseEngine {
private:
    Table table;

    static void trim(string &s) {
        size_t start = 0;
        while(start < s.size() && isspace(static_cast<unsigned char>(s[start]))) start++;
        size_t end = s.size();
        while(end > start && isspace(static_cast<unsigned char>(s[end - 1]))) end--;
        s = s.substr(start, end-start);
    }

public:
    DatabaseEngine(): table(1000) {}

    bool handle_input(const string &line) {
        string trimmed = line;
        trim(trimmed);
        if(trimmed[0] == '.') {
            if (trimmed == ".exit") {
                cout << "Exiting Program.";
                return false;
            } else {
                cout << "Unrecognised meta-command: " << trimmed << endl;
                return true; 
            }
        }

        Tokenizer tokenizer(trimmed);
        const vector<Token> &tokens = tokenizer.tokensize();

        Parser parser(tokens);
        Statement stmt = parser.parse_statement();
        if (stmt.type == StatementType::INVALID) {
            cout << "Syntax Error!" << endl;
            return true;
        }

        Program prog = Compiler::compile(stmt);
        VirtualMachine vm(table);
        vm.execute(prog);

        return true;
    }
};

class REPL {
private:
    DatabaseEngine engine;

public:
    void run() {
        while (true) {
            cout << "db > ";
            string line;
            if (!getline(cin, line)) break;
            if (!engine.handle_input(line)) break;
        }
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    REPL repl;
    repl.run();

    return 0;
}