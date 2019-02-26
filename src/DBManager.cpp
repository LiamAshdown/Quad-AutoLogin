#include "DBManager.h"


DBManager::~DBManager()
{
}

bool DBManager::ConnectToDB()
{
    try
    {
        driver = sql::mysql::get_mysql_driver_instance();
        std::string connectDB = "tcp://" + std::string(DBHost) + ":" + std::to_string(DbPort);
        Con = driver->connect(connectDB, sql::SQLString(Username), sql::SQLString(Password));
        Con->setSchema(DbName);
        LoadIntoContainer();
        std::cout << "Connection to the database successfull!" << std::endl;

    }
    catch (sql::SQLException &e) {
        ManageException(e);
        return false;
    }

}

void DBManager::LoadIntoContainer()
{
    try
    {
        system("cls");
        std::cout << "Loading user containers..." << std::endl;
        pstmt = Con->prepareStatement(sql::SQLString("SELECT username, password FROM raw"));
        res = pstmt->executeQuery();
        res->afterLast();
        while (res->previous())
        {
            rawContainer.push_back(std::make_pair(GetStringField("username"), GetStringField("password")));
        }
    }
    catch (sql::SQLException &e) {
        ManageException(e);
    }
}

unsigned long int const DBManager::GetRowCount()
{
    return TotalRowCount;
}

void DBManager::InsertAccount(const char * username, const char * password, const char * serverName)
{
    if (CheckAccountExist(username, password, serverName))
    {
        std::cout << "[AUTOLOGIN]: Account inserted" << std::endl;
        pstmt = Con->prepareStatement(sql::SQLString("INSERT INTO accounts(server, username, password, data) VALUES (?, ?, ?, ?)"));
        pstmt->setString(1, serverName);
        pstmt->setString(2, username);
        pstmt->setString(3, password);
        pstmt->setInt(4, 0);
        pstmt->executeUpdate();
    }
    
    return;
}

bool DBManager::CheckAccountExist(const char * username, const char * password, const char * serverName)
{
    pstmt = Con->prepareStatement("SELECT server, username, password FROM accounts");
    res = pstmt->executeQuery();
    res->afterLast();

    while (res->previous())
    {
        if (GetStringField("username") == username && GetStringField("server") == serverName)
        {
            std::cout << "[AUTOLOGIN]: Account already exists! Skipping!" << std::endl;
            return true;
        }
    }
    return false;
}

void DBManager::LoadRowCount()
{
    try
    {
        pstmt = Con->prepareStatement(sql::SQLString("SELECT username raw"));
        res = pstmt->executeQuery();
        TotalRowCount = res->rowsCount();
    }
    catch (sql::SQLException &e) {
        ManageException(e);
    }
}

void DBManager::ManageException(sql::SQLException& e)
{
    if (e.getErrorCode() != 0) {
        std::cout << "# ERR: SQLException in " << __FILE__;
        std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
        std::cout << "# ERR: " << e.what();
        std::cout << " (MySQL error code: " << e.getErrorCode();
        std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    }
}

std::string DBManager::GetStringField(const std::string& Field)
{
    std::string temp;
    try
    {
        temp = res->getString(Field);
    }
    catch (sql::SQLException &e) {
        ManageException(e);
        temp = "NULL";
    }
    return temp;
}