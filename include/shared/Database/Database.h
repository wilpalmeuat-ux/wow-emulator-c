#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

enum class DatabaseType { DATABASE_AUTH, DATABASE_WORLD, DATABASE_CHARACTER };

struct QueryResult;
struct PreparedStatement;

// Simple sync MySQL wrapper using libmysqlclient
class Database {
public:
    static Database* Create(const std::string& type, const std::string& host,
                            int port, const std::string& user,
                            const std::string& password,
                            const std::string& database);

    virtual ~Database();

    virtual bool Initialize() = 0;
    virtual void Close() = 0;

    virtual bool Execute(const char* sql) = 0;
    virtual bool Query(const char* sql, QueryResult*& result) = 0;
    virtual bool QueryNoResult(const char* sql) = 0;

    virtual uint64_t GetLastInsertId() = 0;
    virtual int GetAffectedRows() = 0;

    virtual std::string EscapeString(const std::string& str) = 0;

    void SetTimeout(int secs) { _timeout = secs; }

protected:
    Database() : _timeout(10) {}

    int _timeout = 10;
    std::string _host, _user, _pass, _database;
    int _port = 3306;
};

// ─── QueryResult ──────────────────────────────────────────────────
struct Field {
    char* name = nullptr;
    char* data = nullptr;
    size_t dataLength = 0;
    size_t maxLength = 0;
};

struct QueryResult {
    QueryResult() = default;
    ~QueryResult();

    uint64_t rowCount = 0;
    uint32_t fieldCount = 0;
    uint32_t currentRow = 0;
    Field* fields = nullptr;
    char*** rows = nullptr;

    bool NextRow();
    void Reset();

    Field* Fetch(size_t col) const;
    uint8_t GetUInt8(size_t col) const;
    uint16_t GetUInt16(size_t col) const;
    uint32_t GetUInt32(size_t col) const;
    uint64_t GetUInt64(size_t col) const;
    int8_t GetInt8(size_t col) const;
    int16_t GetInt16(size_t col) const;
    int32_t GetInt32(size_t col) const;
    int64_t GetInt64(size_t col) const;
    float GetFloat(size_t col) const;
    double GetDouble(size_t col) const;
    const char* GetString(size_t col) const;
    const char* GetCString(size_t col) const;
};

// ─── PreparedStatement ─────────────────────────────────────────────
struct PreparedStatement {
    PreparedStatement(uint32_t index, const char* sql);
    ~PreparedStatement();

    void SetString(uint32_t col, const std::string& val);
    void SetUInt8(uint32_t col, uint8_t val);
    void SetUInt16(uint32_t col, uint16_t val);
    void SetUInt32(uint32_t col, uint32_t val);
    void SetUInt64(uint32_t col, uint64_t val);
    void SetInt8(uint32_t col, int8_t val);
    void SetInt16(uint32_t col, int16_t val);
    void SetInt32(uint32_t col, int32_t val);
    void SetInt64(uint32_t col, int64_t val);
    void SetFloat(uint32_t col, float val);
    void SetDouble(uint32_t col, double val);

    uint32_t _index;
    const char* _sql;
    std::vector<std::vector<char>> _data;
    std::vector<int> _types;
};
