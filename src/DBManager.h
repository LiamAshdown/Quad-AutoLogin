#include "mysql_connection.h"
#include "mysql_driver.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

#include <iostream>
#include <string>
#include <vector>

#pragma once
class DBManager
{
public:
    DBManager(const char* DbUser, const char* DbPass, const unsigned int port, const char* DbHost, const char* Dbase) :
        Username(DbUser), Password(DbPass), DbPort(port), DBHost(DbHost), DbName(Dbase) {}
    ~DBManager();

    bool ConnectToDB();

    unsigned long int const GetRowCount();
    std::vector<std::pair<std::string, std::string>> GetUserContainer() { return rawContainer; }
    void InsertAccount(const char* username, const char* password, const char* serverName);
    bool CheckAccountExist(const char* username, const char* password, const char* serverName);

private:
    sql::mysql::MySQL_Driver* driver;
    sql::Connection *Con;
    sql::PreparedStatement *pstmt;
    sql::ResultSet *res;
    sql::Statement *stmt;

protected:
    const unsigned int DbPort;
    const char* Username;
    const char* Password;
    const char* DBHost;
    const char* DbName;
    unsigned long TotalRowCount;
    std::vector<std::pair<std::string, std::string>> rawContainer;
 
protected:
    void ManageException(sql::SQLException& e);
    void LoadRowCount();
    void LoadIntoContainer();
    std::string GetStringField(const std::string& Field);
};

