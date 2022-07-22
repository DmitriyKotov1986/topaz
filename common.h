#ifndef COMMON_H
#define COMMON_H

//Qt
#include <QString>
#include <QFile>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>

namespace Common {

enum EXIT_CODE: int {
    //SQL
    OK = 0,
    LOAD_CONFIG_ERR = -1,
    SQL_EXECUTE_QUERY_ERR = -2,
    SQL_COMMIT_ERR = -3,
    //LevelGauge
    LEVELGAUGE_UNDEFINE_TYPE = -100
};

void writeLogFile(const QString& prefix, const QString& msg);

void writeDebugLogFile(const QString& prefix, const QString& msg);

} //Common

#endif // COMMON_H
