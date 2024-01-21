#include "tdoc.h"

//Qt
#include <QSqlError>
#include <QSqlQuery>
#include <QRandomGenerator>

//My
#include <Common/common.h>

using namespace Topaz;

using namespace Common;

TDoc::TDoc()
    : _cnf(TConfig::config())
    , _loger(TDBLoger::DBLoger())
{
    Q_CHECK_PTR(_cnf);
    Q_CHECK_PTR(_loger);

    const auto id = QRandomGenerator64::global()->generate64();

    //настраиваем подключение к БД
    _db = QSqlDatabase::addDatabase(_cnf->db_Driver(), QString("DocMainDB%1").arg(id));
    _db.setDatabaseName(_cnf->db_DBName());
    _db.setUserName(_cnf->db_UserName());
    _db.setPassword(_cnf->db_Password());
    _db.setConnectOptions(_cnf->db_ConnectOptions());
    _db.setPort(_cnf->db_Port());
    _db.setHostName(_cnf->db_Host());

    //подключаемся к БД Системы Мониторинга
    if (!_db.open())
    {
        QString _errorString = QString("Cannot connect to database. Error: %1").arg(_db.lastError().text());

        return;
    };

    //настраиваем подключение к БД Топаза
    _topazDB = QSqlDatabase::addDatabase(_cnf->topazdb_Driver(), QString("DocTopazDB%1").arg(id));
    _topazDB.setDatabaseName(_cnf->topazdb_DBName());
    _topazDB.setUserName(_cnf->topazdb_UserName());
    _topazDB.setPassword(_cnf->topazdb_Password());
    _topazDB.setConnectOptions(_cnf->topazdb_ConnectOptions());
    _topazDB.setPort(_cnf->topazdb_Port());
    _topazDB.setHostName(_cnf->topazdb_Host());

    //подключаемся к БД Топаз-АЗС
    if (!_topazDB.open())
    {
        _errorString = QString("Cannot connect to Topaz_AZS database. Error: %1").arg(_topazDB.lastError().text());

        return;
    };

    //таймер опроса БД Топаз-АЗС
    _workTimer = new QTimer;

    QObject::connect(_workTimer, SIGNAL(timeout()), SLOT(workTimer_timeout()));

    _workTimer->start(_cnf->sys_Interval());

    //мы не можем вызвать workTimer_timeout() из кнструктора, т.к. это виртуальный метод
}

TDoc::~TDoc()
{
    if (_db.isOpen())
    {
        _db.close();
    }

    if (_topazDB.isOpen())
    {
        _topazDB.close();
    }
}

void TDoc::workTimer_timeout()
{
    const auto docsInfo = getDoc();

    if (docsInfo.empty())
    {
        _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, "-->No new documents found");

        return;
    }

    for (const auto& docInfoItem : docsInfo)
    {
            saveDocToDB(docInfoItem);       
    }

    _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("-->%1 documents saved in the database").arg(docsInfo.count()));
}

void TDoc::saveDocToDB(const DocInfo& docInfo)
{
    //сохраняем документы к БД Системного монитора
    const QString queryText =
            QString("INSERT INTO TOPAZDOCS (DATE_TIME, DOC_TYPE, DOC_NUMBER, SMENA, CREATER, BODY) "
                    "VALUES ('%1', '%2', %3, %4, '%5', ? )")
                .arg(docInfo.dateTime.toString("yyyy-MM-dd hh:mm:ss.zzz"))
                .arg(docInfo.type)
                .arg(docInfo.number)
                .arg(docInfo.smena)
                .arg(docInfo.creater.toUtf8().toBase64(QByteArray::Base64Encoding)); //для сохранения спецсимволов и кодировки сохряняем документ в base64

    _topazDB.transaction();
    QSqlQuery query(_db);

    query.prepare(queryText);
    query.bindValue(0, docInfo.XMLText.toUtf8().toBase64(QByteArray::Base64Encoding));  //для сохранения спецсимволов и кодировки сохряняем документ в base64

    writeDebugLogFile(QString("QUERY TO %1>").arg(_db.connectionName()), query.lastQuery());

    if (!query.exec())
    {
        errorDBQuery(_db, query);
    }

    DBCommit(_db);
}

TDoc::SmenaInfo TDoc::getCurrentSmena()
{
    QString queryText =
            "SELECT FIRST 1 MAX(\"SessionNum\") AS \"SessionNum\", \"SessionID\", \"UserName\", \"StartDateTime\", \"EndDateTime\" "
            "FROM \"sysSessions\" "
            "WHERE \"Deleted\" = 0 "
            "GROUP BY \"SessionID\", \"UserName\", \"StartDateTime\", \"EndDateTime\" "
            "ORDER BY \"SessionNum\" DESC ";

    _topazDB.transaction();
    QSqlQuery query(_topazDB);
    query.setForwardOnly(true);

    writeDebugLogFile(QString("QUERY TO %1>").arg(_topazDB.connectionName()), queryText);

    if (!query.exec(queryText))
    {
        errorDBQuery(_topazDB, query);
    }

    SmenaInfo res;
    if (query.first())
    {
        res.smenaID = query.value("SessionID").toULongLong();
        res.smenaNumber = query.value("SessionNum").toInt();
        res.operatorName = query.value("UserName").toString();
        res.startDateTime = query.value("StartDateTime").toDateTime();
        res.isOpen = !query.value("EndDateTime").isNull();
        if (!res.isOpen)
        {
            res.endDateTime = query.value("EndDateTime").toDateTime();
        }
        res.isDeleted = false;
    }

    DBCommit(_topazDB);

    return res;
}

void TDoc::addInfoMsg(const QString& msg)
{
    DocInfo docInfo;

    docInfo.creater = "TOPAZ";
    docInfo.dateTime = QDateTime::currentDateTime();
    docInfo.type = "SYS_MSG";
    docInfo.XMLText = msg;

    saveDocToDB(docInfo);
}

bool Topaz::TDoc::checkLastID(const QString& tableName, const QString& fieldName, const QString& idName, quint64 oldValue, quint64& newValue)
{
    QSqlQuery query(_topazDB);
    query.setForwardOnly(true);
    _topazDB.transaction();

    QString queryText =
        QString("SELECT MAX(\"%1\") AS \"%1\" "
                "FROM \"%2\"")
            .arg(fieldName)
            .arg(tableName);

    writeDebugLogFile(QString("QUERY TO %1>").arg(_topazDB.connectionName()), query.lastQuery());

    if (!query.exec(queryText))
    {
        errorDBQuery(_topazDB, query);
    }

    bool res = true;
    QString msg;

    if (query.first())
    {
        quint64 id = query.value(fieldName).toULongLong();

        if (oldValue == 0) //первый запуск
        {
            msg = QString("First request %1. New value: %2").arg(idName).arg(id);
            newValue = id;
            res = false;
        }
        else if (oldValue > id) //сохранянный ИД больше полученного. Вероятно подменили БД Топаз-АЗС
        {
            msg = QString("Incorrect value \"%1\" on table \"%2\" Topaz-AZS database. New value %3: %4")
                    .arg(fieldName).arg(tableName).arg(idName).arg(id);
            newValue = id;
            res = false;
        }
    }
    else //запрос ничего не вернул. Возможно у нас совсем новая БД у Топаз-АЗС
    {
        msg = QString("Undefine '%1' on table \"%2\" Topaz-AZS database. New value %3: 0")
                .arg(fieldName).arg(tableName).arg(idName);
        newValue = 0;
        res = false;
    }

    DBCommit(_topazDB);

    if (res)
    {
        _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("Check %1 passed successfully").arg(idName));
    }
    else
    {
        addInfoMsg(msg);
        _loger->sendLogMsg(TDBLoger::MSG_CODE::WARNING_CODE, msg);
    }

    return res;
}


