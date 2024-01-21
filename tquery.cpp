#include "tquery.h"

//Qt
#include <QSqlError>
#include <QSqlRecord>
#include <QXmlStreamWriter>

//My
#include "Common/common.h"

using namespace Topaz;
using namespace Common;

static const QString DOC_NAME = "Query";

TQuery::TQuery()
    : TDoc()
    , _loger(TDBLoger::DBLoger())
    , _cnf(TConfig::config())
    , _queryQueue(QueryQueue::createQueryQueue())
{
    Q_CHECK_PTR(_loger);
    Q_CHECK_PTR(_cnf);
    Q_CHECK_PTR(_queryQueue);

    //настраиваем подключение к БД Топаза
    _topazDB = QSqlDatabase::addDatabase(_cnf->topazdb_Driver(), QString("QueryTopazDB%1").arg(_cnf->topaz_OffActCode()));
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
    }
}

TQuery::~TQuery()
{
    if (_topazDB.isOpen())
    {
        _topazDB.close();
    }
}

TDoc::TDocsInfo TQuery::getDoc()
{
    Q_ASSERT(_topazDB.isOpen());

    TDocsInfo res;
    QString errorString;

    _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, "Launching SQL query detection");

    if (_queryQueue->isEmpty())
    {
        return res;
    }

    _topazDB.transaction();
    QSqlQuery query(_topazDB);

    const auto queryData = _queryQueue->dequeue();

    DocInfo docInfo;
    docInfo.type = DOC_NAME;
    docInfo.dateTime = QDateTime::currentDateTime();
    docInfo.number = queryData.id;

    bool ok = false;
    if (!queryData.queryText.isEmpty())
    {
        if (query.exec(queryData.queryText))
        {
            ok = true;
        }
        else
        {
            const QString errorMsg =  QString("Cannot execute query. Query ID: %1. Error: %2. Query: %3")
                    .arg(queryData.id)
                    .arg(query.lastError().text())
                    .arg(query.lastQuery().replace("'", """"));

            errorString = QString("ERROR: %1").arg(errorMsg);

            _loger->sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, QString("-->%1").arg(errorMsg));
        }
    }
    else
    {
        const QString errorMsg =  QString("Query is empty or cannot be decoded. Query ID: %1.").arg(queryData.id);

        errorString = QString("ERROR: %1").arg(errorMsg);

        res.push_back(docInfo) ;
        _loger->sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, QString("-->%1").arg(errorMsg));
    }

    //формируем xml
    QString XMLStr;
    QXmlStreamWriter XMLWriter(&XMLStr);

    XMLWriter.setAutoFormatting(true);
    XMLWriter.writeStartDocument("1.0");
    XMLWriter.writeStartElement("Root");

    XMLWriter.writeTextElement("AZSCode", _cnf->srv_UserName());
    XMLWriter.writeTextElement("DocumentType", docInfo.type);
    XMLWriter.writeTextElement("DocumentNumber", QString::number(queryData.queryId));
    XMLWriter.writeTextElement("Session", QString::number(docInfo.smena));
    XMLWriter.writeTextElement("Creater", docInfo.creater);
    XMLWriter.writeTextElement("DocumentDateTime", docInfo.dateTime.toString("yyyy-MM-dd hh:mm:ss.zzz"));

    if (ok)
    {
        XMLWriter.writeStartElement("Items");

        while (query.next())
        {
            XMLWriter.writeStartElement("Item");
            const auto record = query.record();
            for (int column = 0; column < record.count(); ++column)
            {
                const auto columnName = record.fieldName(column);
                XMLWriter.writeTextElement(columnName, query.value(columnName).toString());
            }
            XMLWriter.writeEndElement(); //Item
        }

        XMLWriter.writeEndElement(); //Items
    }
    else
    {
        XMLWriter.writeTextElement("Error", errorString);
    }

    XMLWriter.writeEndElement(); //root
    XMLWriter.writeEndDocument();

    docInfo.XMLText = XMLStr;

    res.push_back(docInfo);

    DBCommit(_topazDB);

    _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("-->Request with id %1 executed successfully").arg(queryData.id));
    _cnf->set_srv_lastQueryID(queryData.id);

    return res;
}
