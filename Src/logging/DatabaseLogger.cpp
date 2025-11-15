#include "logging/DatabaseLogger.hpp"
#include <iostream> // For std::cerr

DatabaseLogger::DatabaseLogger(const std::string &db_path)
    : m_db_path(db_path), m_db(nullptr) {}//Initlizing DatabaseLogger with db path

DatabaseLogger::~DatabaseLogger()
{
  if (m_db)
  {
    sqlite3_close(m_db);
  }
}

bool DatabaseLogger::init()
{
  char *errMsg = 0;

  int rc = sqlite3_open(m_db_path.c_str(), &m_db);
  if (rc)
  {
    std::cerr << "Can't open database: " << sqlite3_errmsg(m_db) << std::endl;
    return false;
  }

  const char *sql =
      "CREATE TABLE IF NOT EXISTS file_logs ("
      "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "  timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
      "  filename TEXT NOT NULL,"
      "  operation TEXT NOT NULL," // e.g., "ENCRYPT", "DECRYPT"
      "  status TEXT NOT NULL,"    // e.g., "SUCCESS", "FAILED"
      "  processing_time_ms REAL"
      ");";

  rc = sqlite3_exec(m_db, sql, 0, 0, &errMsg); // Use m_db
  if (rc != SQLITE_OK)
  {
    std::cerr << "SQL error: " << errMsg << std::endl;
    sqlite3_free(errMsg);
    return false;
  }
  return true;
}
  void DatabaseLogger::log(const std::string &filename, const std::string &operation,
                           const std::string &status, double time_ms)
  {
    std::lock_guard<std::mutex> lock(m_log_mutex);
    char *errMsg = 0;
    if (!m_db)
    {
      std::cerr << "Database not initialized, cannot log." << std::endl;
      return;
    }

    char *sql = sqlite3_mprintf(
        "INSERT INTO file_logs (filename, operation, status, processing_time_ms) "
        "VALUES (%Q, %Q, %Q, %f);",
        filename.c_str(),
        operation.c_str(),
        status.c_str(),
        time_ms);

    int rc = sqlite3_exec(m_db, sql, 0, 0, &errMsg); // Use m_db
    if (rc != SQLITE_OK)
    {
      std::cerr << "SQL log error: " << errMsg << std::endl;
      sqlite3_free(errMsg);
    }

    sqlite3_free(sql);
  }