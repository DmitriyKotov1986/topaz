#ifndef TCONFIG_H
#define TCONFIG_H

#include <QString>
#include <QFile>

namespace Topaz {

class TConfig
{
public:
    static TConfig* config(const QString& configFileName = "")
    {
        static TConfig* _config = nullptr;

        if (_config == nullptr){
            _config = new TConfig(configFileName);
        }

        return _config;
    };

private:
    explicit TConfig(const QString& configFileName);

public:
    bool save();

    //[DATABASE]
    const QString& db_Driver() const { return _db_Driver; }
    const QString& db_DBName()  const { return _db_DBName; }
    const QString& db_UserName() const { return _db_UserName; }
    const QString& db_Password() const { return _db_Password; }
    const QString& db_ConnectOptions() const { return _db_ConnectOptions; }
    const QString& db_Host() const { return _db_Host; }
    quint16 db_Port() const { return _db_Port; }

    //[SYSTEM]
    int sys_Interval() const { return _sys_Interval; }
    bool sys_DebugMode() const { return _sys_DebugMode; }

    //[SERVER]
    const QString& srv_Host() const { return _srv_Host; }
    quint16 srv_Port() const { return _srv_Port; }
    const QString& srv_UserName() const { return _srv_UserName; }
    const QString& srv_Password() const { return _srv_Password; }
    int srv_MaxRecord() const { return _srv_MaxRecord; }

    const QString& errorString() const { return _errorString; }
    bool isError() const {return _isError; }

private:
    const QString _configFileName;

    bool _isError = false;
    QString _errorString;

    //[DATABASE]
    QString _db_Driver;
    QString _db_DBName;
    QString _db_UserName;
    QString _db_Password;
    QString _db_ConnectOptions;
    QString _db_Host;
    quint16 _db_Port = 0;

    //[SYSTEM]
    int _sys_Interval = 60 * 1000;
    bool _sys_DebugMode = false;

    //[SERVER]
    QString _srv_Host;
    quint16 _srv_Port = 0;
    QString _srv_UserName;
    QString _srv_Password;
    int _srv_MaxRecord = 100;
};

} //namespace LevelGaugeStatus

#endif // TCONFIG_H
