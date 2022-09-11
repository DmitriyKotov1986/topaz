#ifndef COMMON_H
#define COMMON_H

//Qt
#include <QString>
#include <QFile>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

namespace Common {

typedef enum : int {
    //SQL
    OK = 0,                         //успешное завершение
    LOAD_CONFIG_ERR = -1,           //ошибка чтения файла конфигурации
    ALREADY_RUNNIG = -2,            //попытка повторного запуска процесса
    //SQL
    SQL_EXECUTE_QUERY_ERR = -10,   //ошибка выполнения SQL запроса
    SQL_COMMIT_ERR = -11,
    SQL_NOT_OPEN_DB = -12,
    //LevelGauge
    LEVELGAUGE_UNDEFINE_TYPE = -100,
    //SystemMonitor
    SERVICE_INIT_ERR = -200,
    SERVICE_START_ERR = -201,
    SERVICE_RESUME_ERR = -202,
    SERVICE_STOP_ERR = -203,
    SERVISE_PAUSE_ERR = -204,
    //XML Parser
    XML_EMPTY = -500,
    XML_PARSE_ERR = 501,
    XML_UNDEFINE_TOCKEN = 502
} EXIT_CODE;

bool checkAlreadyRun();

void exitIfAlreadyRun();

void writeLogFile(const QString& prefix, const QString& msg);

void writeDebugLogFile(const QString& prefix, const QString& msg);

void saveLogToFile(const QString& msg);

void DBQueryExecute(QSqlDatabase& db, const QString &queryText); //Выполнят запрос к БД типа INSERT и UPDATE

void DBCommit(QSqlDatabase& db); //выполнят Commit

void errorDBQuery(QSqlDatabase& db, const QSqlQuery& query);

} //Common

#endif // COMMON_H
