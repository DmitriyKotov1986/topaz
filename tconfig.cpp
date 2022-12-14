#include "tconfig.h"

//Qt
#include <QSettings>
#include <QFileInfo>
#include <QDebug>
#include <QDir>

using namespace Topaz;

TConfig::TConfig(const QString& configFileName) :
    _configFileName(configFileName)
{
    if (_configFileName.isEmpty())
    {
        _errorString = "Configuration file name cannot be empty";
        return;
    }
    if (!QFileInfo::exists(_configFileName))
    {
        _errorString = "Configuration file not exist. File name: " + _configFileName;
        return;
    }

    qDebug() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg("Reading configuration from " +  _configFileName);

    QSettings ini(_configFileName, QSettings::IniFormat);

    QStringList groups = ini.childGroups();
    if (!groups.contains("DATABASE"))
    {
        _errorString = "Configuration file not contains [DATABASE] group";
        return;
    }
    if (!groups.contains("TOPAZDATABASE"))
    {
        _errorString = "Configuration file not contains [TOPAZDATABASE] group";
        return;
    }
    if (!groups.contains("TOPAZ"))
    {
        _errorString = "Configuration file not contains [TOPAZ] group";
        return;
    }

    //Database
    ini.beginGroup("DATABASE");
    _db_Driver = ini.value("Driver", "QODBC").toString();
    _db_DBName = ini.value("DataBase", "SystemMonitorDB").toString();
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

    //Topaz Database
    ini.beginGroup("TOPAZDATABASE");
    _topazdb_Driver = ini.value("Driver", "QODBC").toString();
    _topazdb_DBName = ini.value("DataBase", "TopazDB").toString();
    _topazdb_UserName = ini.value("UID", "").toString();
    _topazdb_Password = ini.value("PWD", "").toString();
    _topazdb_ConnectOptions = ini.value("ConnectionOprions", "").toString();
    _topazdb_Port = ini.value("Port", "").toUInt();
    _topazdb_Host = ini.value("Host", "localhost").toString();
    ini.endGroup();

    //topaz
    ini.beginGroup("TOPAZ");
    _topaz_DirName = ini.value("Dir", "").toString();
    if (!QDir(_topaz_DirName).exists())
    {
        _errorString = QString("Topaz AZS ImportDoc directory not exist: %1").arg(_topaz_DirName);
        return;
    }
    _topaz_LastDocNumber = ini.value("LastDocNumber", 0).toInt();
    _topaz_OffActEnabled = ini.value("OffActEnabled", true).toBool();
    _topaz_OffActCode = ini.value("OffActCode", 20).toInt();
    _topaz_InputActDeleteFile = ini.value("InputActDeleteFile", true).toBool();
    _topaz_InputActCode = ini.value("InputActCode", 1).toInt();
    _topaz_CouponsEnable = ini.value("CouponsEnable", true).toBool();
    _topaz_CouponsCode = ini.value("CouponsCode", 6).toInt();
    _topaz_LastCouponsID = ini.value("LastCouponsID", 0).toInt();

    ini.endGroup();

    ini.beginGroup("SERVER");
    _srv_UserName = ini.value("UID", "000").toString();
    _srv_Password = ini.value("PWD", "").toString();
    _srv_Host = ini.value("Host", "localhost").toString();
    _srv_Port = ini.value("Port", "").toUInt();
    _srv_MaxRecord = ini.value("MaxRecord", "100").toUInt();
    ini.endGroup();
}

TConfig::~TConfig()
{
    save();
}

bool TConfig::save()
{
    QSettings ini(_configFileName, QSettings::IniFormat);

    if (!ini.isWritable())
    {
        _errorString = "Can not write configuration file " +  _configFileName;
        return false;
    }

    ini.beginGroup("TOPAZ");
    ini.setValue("LastDocNumber", _topaz_LastDocNumber);
    ini.setValue("LastCouponsID", _topaz_LastCouponsID);
    ini.endGroup();

    ini.sync();

    if (_sys_DebugMode)
    {
        qDebug() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg("Save configuration to " +  _configFileName);
    }

    return true;
}

