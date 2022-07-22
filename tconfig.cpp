#include <QSettings>
#include <QFileInfo>
#include <QDebug>
#include "tconfig.h"

using namespace Topaz;

TConfig::TConfig(const QString& configFileName) :
    _configFileName(configFileName)
{
    if (_configFileName.isEmpty()) {
        _isError = true;
        _errorString = "Configuration file name cannot be empty";
        return;
    }
    if (!QFileInfo(_configFileName).exists()) {
        _isError = true;
        _errorString = "Configuration file not exist. File name: " + _configFileName;
        return;
    }

    qDebug() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg("Reading configuration from " +  _configFileName);

    QSettings ini(_configFileName, QSettings::IniFormat);

    //Database
    ini.beginGroup("DATABASE");
    _db_Driver = ini.value("Driver", "QODBC").toString();
    _db_DBName = ini.value("DataBase", "HTTPServerDB").toString();
    _db_UserName = ini.value("UID", "").toString();
    _db_Password = ini.value("PWD", "").toString();
    _db_ConnectOptions = ini.value("ConnectionOprions", "").toString();
    _db_Port = ini.value("Port", "").toUInt();
    _db_Host = ini.value("Host", "localhost").toString();
    ini.endGroup();

    ini.beginGroup("SYSTEM");
    _sys_Interval = ini.value("Interval", "60000").toInt();
    _sys_DebugMode = ini.value("DebugMode", "0").toBool();
    ini.endGroup();

    ini.beginGroup("SERVER");
    _srv_UserName = ini.value("UID", "000").toString();
    _srv_Password = ini.value("PWD", "").toString();
    _srv_Host = ini.value("Host", "localhost").toString();
    _srv_Port = ini.value("Port", "").toUInt();
    _srv_MaxRecord = ini.value("MaxRecord", "100").toUInt();
    ini.endGroup();
}

bool TConfig::save()
{
    QSettings ini(_configFileName, QSettings::IniFormat);

    if (!ini.isWritable()) {
        _isError = true;
        _errorString = "Can not write configuration file " +  _configFileName;
        return false;
    }

    ini.sync();

    if (_sys_DebugMode) {
        qDebug() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg("Save configuration to " +  _configFileName);
    }

    return true;
}

