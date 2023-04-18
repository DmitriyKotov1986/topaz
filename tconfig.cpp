#include "tconfig.h"

//Qt
#include <QSettings>
#include <QFileInfo>
#include <QDebug>
#include <QDir>

using namespace Topaz;

//static
static TConfig* configPtr = nullptr;

TConfig* TConfig::config(const QString& configFileName)
{
    if (configPtr == nullptr)
    {
        configPtr = new TConfig(configFileName);
    }

    return configPtr;
};

void TConfig::deleteConfig()
{
    Q_CHECK_PTR(configPtr);

    if (configPtr != nullptr)
    {
        delete configPtr;
        configPtr = nullptr;
    }
}

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
    if (_topaz_DirName.isEmpty() || !QDir(_topaz_DirName).exists())
    {
        _errorString = QString("Topaz-AZS ImportDoc directory not exist: %1").arg(_topaz_DirName);

        return;
    }

    bool ok = false;
    _topaz_LastOffActID = ini.value("LastOffActID", 0).toULongLong(&ok);
    if (!ok)
    {
        _errorString = QString("Key value [TOPAZ]/LastOffActID must be unsigned number");

        return;
    }

    _topaz_OffActEnabled = ini.value("OffActEnabled", true).toBool();
    _topaz_OffActCode = ini.value("OffActCode", 20).toUInt(&ok);
    if (!ok)
    {
        _errorString = QString("Key value [TOPAZ]/OffActCode must be unsigned number");

        return;
    }

    _topaz_InputActDeleteFile = ini.value("InputActDeleteFile", true).toBool();
    _topaz_InputActCode = ini.value("InputActCode", 1).toUInt(&ok);
    if (!ok)
    {
        _errorString = QString("Key value [TOPAZ]/InputActCode must be unsigned number");

        return;
    }

    _topaz_LastInputActID = ini.value("LastInputActID", 0).toULongLong(&ok);
    if (!ok)
    {
        _errorString = QString("Key value [TOPAZ]/LastInputActID must be unsigned number");

        return;
    }
    _topaz_InputActFilesList = ini.value("InputActFilesList", QStringList()).toStringList();

    _topaz_CouponsEnable = ini.value("CouponsEnable", true).toBool();
    _topaz_CouponsCode = ini.value("CouponsCode", 6).toUInt(&ok);
    if (!ok)
    {
        _errorString = QString("Key value [TOPAZ]/CouponsCode must be unsigned number");

        return;
    }

    _topaz_LastCouponsID = ini.value("LastCouponsID", 0).toULongLong(&ok);
    if (!ok)
    {
        _errorString = QString("Key value [TOPAZ]/LastCouponsID must be unsigned number");

        return;
    }

    _topaz_NewSmenaEnabled = ini.value("NewSmenaEnabled", true).toBool();
    _topaz_LastSmenaID = ini.value("LastSmenaID", 0).toULongLong(&ok);
    if (!ok)
    {
        _errorString = QString("Key value [TOPAZ]/LastSmenaID must be unsigned number");

        return;
    }
    _topaz_PriceDeleteFile = ini.value("PriceDeleteFile", true).toBool();
    _topaz_PriceFilesList = ini.value("PriceFilesList", QStringList()).toStringList();

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

    ini.clear();

    //Database
    ini.beginGroup("DATABASE");

    ini.remove("");

    ini.setValue("Driver", _db_Driver);
    ini.setValue("DataBase", _db_DBName);
    ini.setValue("UID", _db_UserName);
    ini.setValue("PWD", _db_Password);
    ini.setValue("ConnectionOptions", _db_ConnectOptions);
    ini.setValue("Port", _db_Port);
    ini.setValue("Host", _db_Host);

    ini.endGroup();

    //Topaz Database
    ini.beginGroup("TOPAZDATABASE");

    ini.remove("");

    ini.setValue("Driver", _topazdb_Driver);
    ini.setValue("DataBase", _topazdb_DBName);
    ini.setValue("UID", _topazdb_UserName);
    ini.setValue("PWD", _topazdb_Password);
    ini.setValue("ConnectionOptions", _topazdb_ConnectOptions);
    ini.setValue("Port", _topazdb_Port);
    ini.setValue("Host", _topazdb_Host);

    ini.endGroup();

    //System
    ini.beginGroup("SYSTEM");

    ini.remove("");

    ini.setValue("Interval", _sys_Interval);
    ini.setValue("DebugMode", _sys_DebugMode);

    ini.endGroup();

    //Server
    ini.beginGroup("SERVER");

    ini.remove("");

    ini.setValue("UID", _srv_UserName);
    ini.setValue("PWD", _srv_Password);
    ini.setValue("Host", _srv_Host);
    ini.setValue("Port", _srv_Port);
    ini.setValue("MaxRecord", _srv_MaxRecord);

    ini.endGroup();

    //Topaz
    ini.beginGroup("TOPAZ");

    ini.remove("");

    ini.setValue("Dir", _topaz_DirName);

    ini.setValue("LastOffActID", _topaz_LastOffActID);
    ini.setValue("OffActEnabled", _topaz_OffActEnabled);
    ini.setValue("OffActCode", _topaz_OffActCode);

    ini.setValue("InputActEnabled", _topaz_InputActEnabled);
    ini.setValue("LastInputActID", _topaz_LastInputActID);
    ini.setValue("InputActCode", _topaz_InputActCode);
    ini.setValue("InputActDeleteFile", _topaz_InputActDeleteFile);
    ini.setValue("InputActFilesList", _topaz_InputActFilesList);

    ini.setValue("CouponsEnable", _topaz_CouponsEnable);
    ini.setValue("LastCouponsID", _topaz_LastCouponsID);
    ini.setValue("CouponsCode", _topaz_CouponsCode);

    ini.setValue("NewSmenaEnabled", _topaz_NewSmenaEnabled);
    ini.setValue("LastSmenaID", _topaz_LastSmenaID);
    ini.setValue("PriceDeleteFile", _topaz_PriceDeleteFile);
    ini.setValue("PriceFilesList", _topaz_PriceFilesList);

    ini.endGroup();

    ini.sync();

    if (_sys_DebugMode)
    {
        qDebug() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg("Save configuration to " +  _configFileName);
    }

    return true;
}

QString TConfig::errorString()
{
    auto res = _errorString;
    _errorString.clear();

    return res;
}


