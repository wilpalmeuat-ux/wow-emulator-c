#include "Database/Database.h"
#include "Logging/Log.h"
#include <mysql/mysql.h>
#include <cstring>
#include <cerrno>

struct MySQLDatabase : Database {
    MySQLDatabase(const std::string& host, int port, const std::string& user,
                  const std::string& pass, const std::string& db)
    {
        _host = host; _port = port; _user = user; _pass = pass; _database = db;
        _conn = nullptr;
    }

    bool Initialize() override {
        _conn = mysql_init(nullptr);
        if (!_conn) return false;
        if (!mysql_real_connect(_conn, _host.c_str(), _user.c_str(), _pass.c_str(),
                                _database.c_str(), _port, nullptr, 0)) {
            LOG_ERROR("MySQL connect failed: %s", mysql_error(_conn));
            return false;
        }
        mysql_set_character_set(_conn, "utf8");
        return true;
    }

    void Close() override {
        if (_conn) { mysql_close(_conn); _conn = nullptr; }
    }

    bool Execute(const char* sql) override {
        return mysql_query(_conn, sql) == 0;
    }

    bool Query(const char* sql, QueryResult*& result) override {
        if (mysql_query(_conn, sql) != 0) {
            LOG_ERROR("Query failed: %s", mysql_error(_conn));
            result = nullptr;
            return false;
        }
        result = new QueryResult();
        MYSQL_RES* res = mysql_store_result(_conn);
        if (!res) {
            result->rowCount = 0;
            result->fieldCount = 0;
            result->rows = nullptr;
            result->fields = nullptr;
            return true;
        }
        result->fieldCount = mysql_num_fields(res);
        result->rowCount = mysql_num_rows(res);

        // fetch field names
        result->fields = new Field[result->fieldCount];
        MYSQL_FIELD* fields = mysql_fetch_fields(res);
        for (uint32_t i = 0; i < result->fieldCount; ++i) {
            result->fields[i].name = strdup(fields[i].name);
        }

        // fetch rows
        if (result->rowCount > 0) {
            result->rows = (char***)malloc(sizeof(char**) * result->rowCount);
            MYSQL_ROW row;
            uint32_t r = 0;
            while ((row = mysql_fetch_row(res))) {
                result->rows[r] = (char**)malloc(sizeof(char*) * result->fieldCount);
                for (uint32_t c = 0; c < result->fieldCount; ++c)
                    result->rows[r][c] = row[c] ? strdup(row[c]) : nullptr;
                ++r;
            }
        }
        mysql_free_result(res);
        return true;
    }

    bool QueryNoResult(const char* sql) override {
        return mysql_query(_conn, sql) == 0;
    }

    uint64_t GetLastInsertId() override { return (uint64_t)mysql_insert_id(_conn); }
    int GetAffectedRows() override { return (int)mysql_affected_rows(_conn); }

    std::string EscapeString(const std::string& str) override {
        char buf[1024 * 4];
        mysql_real_escape_string(_conn, buf, str.c_str(), (unsigned long)str.size());
        return std::string(buf);
    }

    MYSQL* _conn = nullptr;
};

Database* Database::Create(const std::string& type, const std::string& host,
                           int port, const std::string& user,
                           const std::string& password,
                           const std::string& database) {
    (void)type;
    return new MySQLDatabase(host, port, user, password, database);
}

Database::~Database() = default;

// ─── QueryResult ──────────────────────────────────────────────────
QueryResult::~QueryResult() {
    Reset();
}

void QueryResult::Reset() {
    if (fields) {
        for (uint32_t i = 0; i < fieldCount; ++i) free(fields[i].name);
        delete[] fields;
        fields = nullptr;
    }
    if (rows) {
        for (uint64_t r = 0; r < rowCount; ++r) {
            if (rows[r]) {
                for (uint32_t c = 0; c < fieldCount; ++c) free(rows[r][c]);
                free(rows[r]);
            }
        }
        free(rows);
        rows = nullptr;
    }
    rowCount = 0; fieldCount = 0; currentRow = 0;
}

bool QueryResult::NextRow() {
    if (currentRow >= rowCount) return false;
    ++currentRow;
    return true;
}

Field* QueryResult::Fetch(size_t col) const {
    if (currentRow == 0 || currentRow > rowCount || col >= fieldCount) return nullptr;
    size_t idx = currentRow - 1;
    fields[col].data = rows[idx][col];
    return &fields[col];
}

uint8_t QueryResult::GetUInt8(size_t col) const {
    auto f = Fetch(col);
    return f && f->data ? (uint8_t)std::atoi(f->data) : 0;
}
uint16_t QueryResult::GetUInt16(size_t col) const {
    auto f = Fetch(col);
    return f && f->data ? (uint16_t)std::atoi(f->data) : 0;
}
uint32_t QueryResult::GetUInt32(size_t col) const {
    auto f = Fetch(col);
    return f && f->data ? (uint32_t)std::atoll(f->data) : 0;
}
uint64_t QueryResult::GetUInt64(size_t col) const {
    auto f = Fetch(col);
    return f && f->data ? (uint64_t)std::atoll(f->data) : 0;
}
int8_t QueryResult::GetInt8(size_t col) const { return (int8_t)GetUInt8(col); }
int16_t QueryResult::GetInt16(size_t col) const { return (int16_t)GetUInt16(col); }
int32_t QueryResult::GetInt32(size_t col) const { return (int32_t)GetUInt32(col); }
int64_t QueryResult::GetInt64(size_t col) const { return (int64_t)GetUInt64(col); }
float QueryResult::GetFloat(size_t col) const {
    auto f = Fetch(col);
    return f && f->data ? (float)std::atof(f->data) : 0.0f;
}
double QueryResult::GetDouble(size_t col) const {
    auto f = Fetch(col);
    return f && f->data ? std::atof(f->data) : 0.0;
}
const char* QueryResult::GetString(size_t col) const {
    auto f = Fetch(col);
    return f ? f->data : "";
}

// ─── PreparedStatement ─────────────────────────────────────────────
PreparedStatement::PreparedStatement(uint32_t index, const char* sql)
    : _index(index), _sql(sql) {}

PreparedStatement::~PreparedStatement() = default;

void PreparedStatement::SetString(uint32_t col, const std::string& val) {
    if (col >= _data.size()) _data.resize(col + 1);
    _data[col] = std::vector<char>(val.begin(), val.end());
    _data[col].push_back('\0');
    _types[col] = 1;
}
void PreparedStatement::SetUInt8(uint32_t col, uint8_t val) {
    if (col >= _data.size()) _data.resize(col + 1);
    _data[col].assign((char*)&val, (char*)&val + 1); _types[col] = 2;
}
void PreparedStatement::SetUInt32(uint32_t col, uint32_t val) {
    if (col >= _data.size()) _data.resize(col + 1);
    _data[col].assign((char*)&val, (char*)&val + 4); _types[col] = 4;
}
void PreparedStatement::SetUInt64(uint32_t col, uint64_t val) {
    if (col >= _data.size()) _data.resize(col + 1);
    _data[col].assign((char*)&val, (char*)&val + 8); _types[col] = 5;
}
void PreparedStatement::SetInt32(uint32_t col, int32_t val) {
    if (col >= _data.size()) _data.resize(col + 1);
    _data[col].assign((char*)&val, (char*)&val + 4); _types[col] = 6;
}
