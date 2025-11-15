#pragma once

#include <string>
#include <mutex>
#include <sqlite3.h>

class DatabaseLogger {
public:
    DatabaseLogger(const std::string& db_path);
    ~DatabaseLogger();

    // Prevent copying
    DatabaseLogger(const DatabaseLogger&) = delete;
    DatabaseLogger& operator=(const DatabaseLogger&) = delete;

    bool init();
    void log(const std::string& filename, const std::string& operation,
             const std::string& status, double time_ms);

private:
    std::string m_db_path;
    sqlite3* m_db; // We'll manage the db connection here
    std::mutex m_log_mutex;
};