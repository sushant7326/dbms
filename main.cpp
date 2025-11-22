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

const uint32_t PAGE_SIZE = 4096;
const uint32_t ID_SIZE = sizeof(int32_t);
const uint32_t USERNAME_SIZE = 32;
const uint32_t EMAIL_SIZE = 92;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;

void serialize_row (const Row &source, void *destination) {
    char *dest = static_cast<char*>(destination);

    memcpy(dest, &source.id, ID_SIZE);
    dest += ID_SIZE;

    string u = source.username;
    if (u.size() > USERNAME_SIZE) u = u.substr(0, USERNAME_SIZE);
    memset(dest, 0, USERNAME_SIZE);
    memcpy(dest, u.data(), u.size());
    dest += USERNAME_SIZE;

    string e = source.email;
    if (e.size() > EMAIL_SIZE) e = e.substr(0, EMAIL_SIZE);
    memset(dest, 0, EMAIL_SIZE);
    memcpy(dest, e.data(), e.size());
}

void deserialize_row(const void *source, Row &dest_row) {
    const char *src = static_cast<const char*>(source);

    memcpy(&dest_row.id, src, ID_SIZE);
    src += ID_SIZE;

    dest_row.username.assign(src, USERNAME_SIZE);
    auto it_u = find(dest_row.username.begin(), dest_row.username.end(), '\0');
    if (it_u != dest_row.username.end()) {
        dest_row.username.erase(it_u, dest_row.username.end());
    }

    src += USERNAME_SIZE;
    dest_row.email.assign(src, EMAIL_SIZE);
    auto it_e = find(dest_row.email.begin(), dest_row.email.end(), '\0');
    if (it_e != dest_row.email.end()) {
        dest_row.email.erase(it_e, dest_row.email.end());
    }
}

class BTreeNode {
public:
    bool leaf;
    int n;
    vector<int> keys;
    vector<int> values;
    vector<BTreeNode*> children;

    BTreeNode(int t, bool leaf): 
        leaf(leaf), 
        n(0), 
        keys(2*t-1), 
        values(2*t-1), 
        children(2*t, nullptr) {
    }
};

class BTreeIndex {
private:
    BTreeNode* root;
    int t;

    static void free_node(BTreeNode* node) {
        if (!node) return;
        if (!node->leaf) {
            for (BTreeNode* child: node->children) {
                if (child) free_node(child);
            }
        }
        delete node;
    }

    bool search_internal(BTreeNode* node, int key, int &row_index_out) const {
        if (!node) return false;
        int i = 0;
        while (i < node->n && key > node->keys[i]) i++;
        if (i < node->n && key == node->keys[i]) {
            row_index_out = node->values[i];
            return true;
        }
        if (node->leaf) return false;
        return search_internal(node->children[i], key, row_index_out);
    }

    void split_child(BTreeNode* parent, int i, BTreeNode* y) {
        BTreeNode* z = new BTreeNode(t, y->leaf);
        z->n = t-1;

        for (int j = 0; j < t-1; j++) {
            z->keys[j] = y->keys[j+t];
            z->values[j] = y->values[j+t];
        }
        
        if (!y->leaf) {
            for (int j = 0; j < t; j++) {
                z->children[j] = y->children[j+t];
            }
        }

        y->n = t-1;

        for (int j = parent->n; j >= i+1; j--) {
            parent->children[j+1] = parent->children[j];
        }
        parent->children[i+1] = z;

        for (int j = parent->n - 1; j >= i; j--) {
            parent->keys[j+1] = parent->keys[j];
            parent->values[j+1] = parent->values[j];
        }

        parent->keys[i] = y->keys[t-1];
        parent->values[i] = y->values[t-1];
        parent->n += 1;
    }

    void insert_non_full(BTreeNode* node, int key, int row_index) {
        int i = node->n - 1;
        if (node->leaf) {
            while (i >= 0 && key < node->keys[i]) {
                node->keys[i+1] = node->keys[i];
                node->values[i+1] = node->values[i];
                i--;
            }
            node->keys[i+1] = key;
            node->values[i+1] = row_index;

            node->n += 1;
        } else {
            while (i >= 0 && key < node->keys[i]) i--;
            i++;

            if (node->children[i]->n == 2*t-1) {
                split_child(node, i, node->children[i]);
                if (key > node->keys[i]) i++;
            }
            insert_non_full(node->children[i], key, row_index);
        }
    }

public:
    BTreeIndex(int t_min = 3): root(nullptr), t(t_min) {}

    ~BTreeIndex() {
        clear();
    }

    void clear() {
        free_node(root);
        root = nullptr;
    }

    void build_from_rows(const vector<Row> &rows) {
        clear();
        for (int i = 0; i < (int)rows.size(); i++) {
            insert(rows[i].id, i);
        }
    }

    bool search(int key, int &row_index_out) const {
        return search_internal(root, key, row_index_out);
    }

    void insert(int key, int row_index) {
        if (root == nullptr) {
            root = new BTreeNode(t, true);
            root->keys[0] = key;
            root->values[0] = row_index;
            root->n = 1;
            return;
        }

        if (root->n == 2*t-1) {
            BTreeNode* s = new BTreeNode(t, false);
            s->children[0] = root;
            split_child(s, 0, root);
            int i = 0;
            if (key > s->keys[0]) i++;
            insert_non_full(s->children[i], key, row_index);
            root = s;
        } else {
            insert_non_full(root, key, row_index);
        }
    }
};

class Pager {
private:
    string filename;
    fstream file;
    vector<unique_ptr<char[]>> pages;
    uint32_t num_pages_loaded = 0;
    size_t file_length = 0;
    bool write_mode;

public:
    Pager(const string &path, bool write): filename(path), write_mode(write) {
        if (write_mode) {
            file.open(filename, ios::binary | ios::out | ios::trunc);
            if (!file) {
                cerr << "Error: cannot open file for writing: " << filename << endl;
                return;
            }
            file_length = 0;
        } else {
            file.open(filename, ios::binary | ios::in);
            if (!file) {
                file_length = 0;
                return;
            }
            file.seekg(0, ios::end);
            file_length = static_cast<size_t>(file.tellg());
            file.seekg(0, ios::beg);
        }
        pages.resize(0);
    }

    ~Pager() {
        if (file.is_open()) {
            file.flush();
            file.close();
        }
    }

    size_t get_file_length() const {
        return file_length;
    }

    char* get_page(uint32_t page_num) {
        if (page_num >= pages.size()) {
            pages.resize(page_num+1);
        }
        if (!pages[page_num]) {
            pages[page_num] = unique_ptr<char[]>(new char[PAGE_SIZE]);
            memset(pages[page_num].get(), 0, PAGE_SIZE);

            if (!write_mode && file.is_open()) {
                size_t offset = static_cast<size_t>(page_num) * PAGE_SIZE;
                if (offset < file_length) {
                    file.seekg(offset, ios::beg);
                    file.read(pages[page_num].get(), PAGE_SIZE);
                }
            }
        }
        if (page_num+1 > num_pages_loaded) {
            num_pages_loaded = page_num+1;
        }
        return pages[page_num].get();
    }


    void flush_page(uint32_t page_num) {
        if (!write_mode) return;
        if (page_num >= pages.size()) return;
        if (!pages[page_num]) return;
        if (!file.is_open()) return;

        size_t offset = static_cast<size_t>(page_num) * PAGE_SIZE;
        file.seekp(offset, ios::beg);
        file.write(pages[page_num].get(), PAGE_SIZE);

        size_t end_pos = offset + PAGE_SIZE;
        if (end_pos > file_length) {
            file_length = end_pos;
        }
    }

    void flush_all() {
        if (!write_mode) return;
        for (uint32_t i = 0; i < num_pages_loaded; i++) {
            flush_page(i);
        }
        if (file.is_open()) file.flush();
    }
};

class Table {
private:
    vector<Row> rows;
    size_t max_rows;
    BTreeIndex index;
    int next_id;

public:
    Table(size_t maxRows = 1000) : max_rows(maxRows), index(3), next_id(1) {}

    void rebuild_index() {
        index.build_from_rows(rows);
    }

    bool insert_row(const Row &r) {
        if (rows.size() >= max_rows) return false;
        rows.push_back(r);
        index.insert(r.id, (int)rows.size()-1);
        return true;
    }
    
    bool insert_autoincrement(const string &username, const string &email) {
        Row r(next_id, username, email);
        bool ok = insert_row(r);
        if (!ok) {
            return false;
        }

        if (next_id == INT_MAX) {
            next_id = 1;
        } else {
            next_id++;
        }

        return true;
    }

    void select_all() const {
        for (const auto &r: rows) {
            cout << "(" << r.id << ", " << r.username << ", " << r.email << ")\n";
        }
        cout << "Rows: " << rows.size() << endl;
    }

    int select_where_id(int id) const {
        int row_idx;
        bool found = index.search(id, row_idx);
        int count = 0;

        if (found) {
            if (row_idx >= 0 && row_idx < (int)rows.size()) {
                const auto &r = rows[row_idx];
                cout << "(" << r.id << ", " << r.username << ", " << r.email << ")\n";
                count = 1;
            }
        }
        cout << "Rows: " << count << endl;
        return count;
    }

    int delete_where_id (int id) {
        int row_idx;
        bool found = index.search(id, row_idx);

        if (!found) {
            cout << "Rows: 0" << endl;
            return 0;
        }
        if (row_idx < 0 || row_idx >= (int)rows.size()) {
            cout << "Rows: 0" << endl;
            return 0;
        }

        rows.erase(rows.begin()+row_idx);
        rebuild_index();
        cout << "Rows: 1" << endl;
        return 1;
    }

    int update_where_id (int id, const string &new_username, const string &new_email) {
        int row_idx;
        bool found = index.search(id, row_idx);
        if (!found) {
            cout << "0 row(s) updated successfully." << endl;
            return 0;
        }
        if (row_idx < 0 || row_idx >= (int)rows.size()) {
            cout << "0 row(s) updated successfully." << endl;
            return 0;
        }

        rows[row_idx].username = new_username;
        rows[row_idx].email = new_email;

        cout << "1 row(s) updated successfully." << endl;
        return 1;
    }

    bool save_to_file(const string &path) const {
        Pager pager(path, true);

        char *header = pager.get_page(0);
        uint64_t row_count = static_cast<uint64_t>(rows.size());
        memset(header, 0, PAGE_SIZE);
        memcpy(header, &row_count, sizeof(row_count));

        for (size_t i = 0; i < rows.size(); i++) {
            size_t rows_byte_offset = i * static_cast<size_t>(ROW_SIZE);
            size_t global_offset    = PAGE_SIZE + rows_byte_offset;

            uint32_t page_num = static_cast<uint32_t>(global_offset / PAGE_SIZE);
            uint32_t page_off = static_cast<uint32_t>(global_offset % PAGE_SIZE);

            char *page = pager.get_page(page_num);
            serialize_row(rows[i], page + page_off);
        }

        pager.flush_all();
        return true;
    }

    bool load_from_file(const string &path) {
        Pager pager(path, false);
        size_t file_len = pager.get_file_length();

        if (file_len < PAGE_SIZE) {
            rows.clear();
            index.clear();
            next_id = 1;
            return false;
        }

        char *header = pager.get_page(0);
        uint64_t row_count = 0;
        memcpy(&row_count, header, sizeof(row_count));

        if (row_count == 0) {
            rows.clear();
            next_id = 1;
            index.clear();
            return true;
        }

        size_t total_rows = static_cast<size_t>(row_count);
        if (total_rows > max_rows) {
            total_rows = max_rows;
        }

        rows.clear();
        rows.reserve(total_rows);

        for (size_t i = 0; i < total_rows; i++) {
            size_t rows_byte_offset = i * static_cast<size_t>(ROW_SIZE);
            size_t global_offset    = PAGE_SIZE + rows_byte_offset;

            uint32_t page_num = static_cast<uint32_t>(global_offset / PAGE_SIZE);
            uint32_t page_off = static_cast<uint32_t>(global_offset % PAGE_SIZE);

            char *page = pager.get_page(page_num);
            Row r;
            deserialize_row(page + page_off, r);
            rows.push_back(r);
        }

        rebuild_index();
        if (rows.empty()) {
            next_id = 1;
        } else {
            int max_id = rows[0].id;
            for (const auto &r: rows) {
                if (r.id > max_id) {
                    max_id = r.id;
                }
            }
            if (max_id < 0) {
                max_id = 0;
            }
            next_id = max_id + 1;
        }
        return true;
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
                ins.id_operand = 0;
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
            
        }
        Instruction halt;
        halt.op = OpCode::OP_HALT;
        prog.add(halt);

        return prog;
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
                    
                    bool ok = table.insert_autoincrement(ins.username_operand, ins.email_operand);;
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
                    cout << updated << " row(s) updated successfully." << endl;
                    pc++;
                    break;
                } case OpCode::OP_DELETE_WHERE_ID: {
                    int deleted = table.delete_where_id(ins.id_operand);
                    cout << deleted << " row(s) deleted successfully." << endl;
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
    string filename;

    static void trim(string &s) {
        size_t start = 0;
        while(start < s.size() && isspace(static_cast<unsigned char>(s[start]))) start++;
        size_t end = s.size();
        while(end > start && isspace(static_cast<unsigned char>(s[end - 1]))) end--;
        s = s.substr(start, end-start);
    }

public:
    DatabaseEngine(const string &file = "db.data"): table(1000) , filename(file) {
        table.load_from_file(filename);
    }

    bool handle_input(const string &line) {
        string trimmed = line;
        trim(trimmed);
        if (trimmed.empty()) return true;
        
        if(trimmed[0] == '.') {
            if (trimmed == ".exit") {
                table.save_to_file(filename);
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