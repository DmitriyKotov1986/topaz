#ifndef TCONFIG_H
#define TCONFIG_H

//QT
#include <QString>
#include <QFile>

namespace Topaz
{

class TConfig final
{
public:
    static TConfig* config(const QString& configFileName = "");
    static void deleteConfig();

private:
    explicit TConfig(const QString& configFileName);
    ~TConfig();

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

    //[TOPAZDATABASE]
    const QString& topazdb_Driver() const { return _topazdb_Driver; }
    const QString& topazdb_DBName() const { return _topazdb_DBName; }
    const QString& topazdb_UserName() const { return _topazdb_UserName; }
    const QString& topazdb_Password() const { return _topazdb_Password; }
    const QString& topazdb_ConnectOptions() const { return _topazdb_ConnectOptions; }
    const QString& topazdb_Host() const { return _topazdb_Host; }
    quint16 topazdb_Port() const { return _topazdb_Port; }

    //[TOPAZ]
    const QString& topaz_DirName() const { return _topaz_DirName; }

    quint64 topaz_LastOffActID() const { return _topaz_LastOffActID; }
    void set_topaz_LastOffActID(quint64 lastOffActID) { _topaz_LastOffActID = lastOffActID; save(); }
    quint16 topaz_OffActCode() const { return _topaz_OffActCode; }
    bool topaz_OffActEnabled() const { return _topaz_OffActEnabled; }

    bool topaz_InputActEnabled() const { return _topaz_InputActEnabled; }
    bool topaz_InputActDeleteFile() const { return _topaz_InputActDeleteFile; }
    quint16 topaz_InputActCode() const { return _topaz_InputActCode; }
    quint64 topaz_LastInputActID() const { return _topaz_LastInputActID; }
    void set_topaz_LastInputActID(quint64 lastInputActId) { _topaz_LastInputActID = lastInputActId; save(); }
    const QStringList& topaz_InputActFilesList() const { return  _topaz_InputActFilesList; };
    void set_topaz_InputActFilesList(const QStringList& inputActFilesList) { _topaz_InputActFilesList = inputActFilesList; save(); };

    bool topaz_CouponsEnable() const { return _topaz_CouponsEnable; }
    quint16 topaz_CouponsCode() const { return _topaz_CouponsCode; }
    quint64 topaz_LastCouponsID() const { return _topaz_LastCouponsID; }
    void set_topaz_LastCouponsID(quint64 lastCouponsId) { _topaz_LastCouponsID = lastCouponsId; save();  }

    bool topaz_NewSmenaEnabled() const { return _topaz_NewSmenaEnabled; }
    quint64 topaz_LastSmenaID() const { return _topaz_LastSmenaID; }
    void set_topaz_LastSmenaID(quint64 lastSmenaID) { _topaz_LastSmenaID = lastSmenaID; save();}
    bool topaz_PriceDeleteFile() const { return  _topaz_PriceDeleteFile; };
    const QStringList& topaz_PriceFilesList() const { return  _topaz_PriceFilesList; };
    void set_topaz_PriceFilesList(const QStringList& priceFilesList) { _topaz_PriceFilesList = priceFilesList; save(); };

    //[SYSTEM]
    int sys_Interval() const { return _sys_Interval; }
    bool sys_DebugMode() const { return _sys_DebugMode; }

    //[SERVER]
    const QString& srv_Host() const { return _srv_Host; }
    quint16 srv_Port() const { return _srv_Port; }
    const QString& srv_UserName() const { return _srv_UserName; }
    const QString& srv_Password() const { return _srv_Password; }
    int srv_MaxRecord() const { return _srv_MaxRecord; }

    QString errorString();
    bool isError() const { return !_errorString.isEmpty(); }

private:
    const QString _configFileName;

    QString _errorString;

    //[DATABASE]
    QString _db_Driver;
    QString _db_DBName;
    QString _db_UserName;
    QString _db_Password;
    QString _db_ConnectOptions;
    QString _db_Host;
    quint16 _db_Port = 0;

    //[TOPAZ_DATABASE]
    QString _topazdb_Driver;
    QString _topazdb_DBName;
    QString _topazdb_UserName;
    QString _topazdb_Password;
    QString _topazdb_ConnectOptions;
    QString _topazdb_Host;
    quint16 _topazdb_Port = 0;

    //[TOPAZ]
    QString _topaz_DirName;

    quint64 _topaz_LastOffActID = 0;
    bool _topaz_OffActEnabled = true;
    quint16 _topaz_OffActCode = 20;

    quint64 _topaz_LastInputActID = 0;
    bool _topaz_InputActEnabled = true;
    quint16 _topaz_InputActCode = 1;
    bool _topaz_InputActDeleteFile = true;
    QStringList _topaz_InputActFilesList;

    quint64 _topaz_LastCouponsID = 0;
    bool _topaz_CouponsEnable = true;
    quint16 _topaz_CouponsCode = 6;

    bool _topaz_NewSmenaEnabled = true;
    quint64 _topaz_LastSmenaID = 0;
    bool _topaz_PriceDeleteFile = true;
    QStringList _topaz_PriceFilesList;


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

} //namespace Topaz

#endif // TCONFIG_H
