#include "toffact.h"

//QT
#include <QSqlQuery>
#include <QSqlError>
#include <QXmlStreamWriter>
#include <QDateTime>

//My
#include "Common/common.h"

using namespace Topaz;
using namespace Common;

static const QString DOC_NAME = "OffAct";

TOffAct::TOffAct()
    : TDoc()
    , _loger(TDBLoger::DBLoger())
    , _cnf(TConfig::config())
{
    Q_CHECK_PTR(_loger);
    Q_CHECK_PTR(_cnf);

    //настраиваем подключение к БД Топаза
    _topazDB = QSqlDatabase::addDatabase(_cnf->topazdb_Driver(), QString("OffActTopazDB%1").arg(_cnf->topaz_OffActCode()));
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

    quint64 id = 0;
    if (!checkLastID("rgItemRests", "rgItemRestID", "LastOffActID", _cnf->topaz_LastOffActID(), id))
    {
        _cnf->set_topaz_LastOffActID(id);
    }
}

TOffAct::~TOffAct()
{
    if (_topazDB.isOpen())
    {
        _topazDB.close();
    }
}

TDoc::TDocsInfo TOffAct::getDoc()
{
    _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, "Launching Off Act detection");

    _topazDB.transaction();
    QSqlQuery query(_topazDB);

    auto lastID = _cnf->topaz_LastOffActID();

    QString queryText =
        QString("SELECT J.\"rgItemRestID\", F.\"SessionNum\", F.\"UserName\", J.\"NDoc\", J.\"Date\", \"FullName\", \"ExtCode\", J.\"Quantity\" "
                "FROM \"sysSessions\" AS F "
                "INNER JOIN ( "
                "    SELECT E.\"rgItemRestID\", D.\"NDoc\", E.\"Date\", \"FullName\", \"ExtCode\", E.\"Quantity\" "
                "    FROM \"trOutActH\" AS D "
                "    INNER JOIN ( "
                "        SELECT C.\"rgItemRestID\", C.\"DocID\", C.\"Date\", \"FullName\", \"ExtCode\", C.\"Quantity\" "
                "        FROM \"dcItems\" AS A "
                "        INNER JOIN ( "
                "            SELECT \"rgItemRestID\", \"Date\", \"DocID\", \"ItemID\", \"Quantity\" "
                "            FROM \"rgItemRests\" AS B "
                "            WHERE (B.\"DocTypeID\" = %1) AND (B.\"rgItemRestID\" > %2) "
                "            ) AS C "
                "        ON (A.\"ItemID\" = C.\"ItemID\") "
                "    ) AS E "
                "    ON (D.\"OutActHID\" = E.\"DocID\") "
                ") AS J "
                "ON ((J.\"Date\" >= F.\"StartDateTime\") AND ((J.\"Date\" <= F.\"EndDateTime\") OR (F.\"EndDateTime\" is NULL))) "
                "ORDER BY J.\"NDoc\" ").arg(_cnf->topaz_OffActCode()).arg(lastID);

    writeDebugLogFile(QString("QUERY TO %1>").arg(_topazDB.connectionName()), queryText);

    if (!query.exec(queryText))
    {
        errorDBQuery(_topazDB, query);
    }

    TDoc::TDocsInfo res;
    bool isFound = false;
    while (query.next())
    {
        TDoc::DocInfo docInfo;
        docInfo.number = query.value("NDoc").toInt();
        docInfo.dateTime = query.value("Date").toDateTime();
        docInfo.smena = query.value("SessionNum").toInt();
        docInfo.creater = query.value("UserName").toString();
        docInfo.type = DOC_NAME;

        _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE,
                           QString("-->Detect new Off Act. Number: %1, Smena: %2, Operator: %3, Date: %4")
                                .arg(docInfo.number)
                                .arg(docInfo.smena)
                                .arg(docInfo.creater)
                                .arg(docInfo.dateTime.toString("yyyy-MM-dd hh:mm:ss.zzz")));

        //формируем xml
        QString XMLStr;
        QXmlStreamWriter XMLWriter(&XMLStr);
        XMLWriter.setAutoFormatting(true);
        XMLWriter.writeStartDocument("1.0");
        XMLWriter.writeStartElement("Root");
        XMLWriter.writeTextElement("AZSCode", _cnf->srv_UserName());
        XMLWriter.writeTextElement("DocumentType", docInfo.type);
        XMLWriter.writeTextElement("DocumentNumber", QString::number(docInfo.number));
        XMLWriter.writeTextElement("Session", QString::number(docInfo.smena));
        XMLWriter.writeTextElement("Creater", docInfo.creater);
        XMLWriter.writeTextElement("DocumentDateTime", docInfo.dateTime.toString("yyyy-MM-dd hh:mm:ss.zzz"));

        XMLWriter.writeStartElement("Items");

        do
        {
            XMLWriter.writeStartElement("Item");
            XMLWriter.writeTextElement("ExtCode", query.value("ExtCode").toString());
            XMLWriter.writeTextElement("FullName", query.value("FullName").toString().toUtf8());
            XMLWriter.writeTextElement("Quantity", query.value("Quantity").toString());
            XMLWriter.writeEndElement(); //Item

            lastID = std::max(query.value("rgItemRestID").toULongLong(), lastID);
        }
        while (query.next() && (query.value("NDoc").toInt() == docInfo.number));
        query.previous(); //возвращаемся на 1 назад, т.к. при выходе из цикла сместились в новый документ

        XMLWriter.writeEndElement(); //Items

        XMLWriter.writeEndElement(); //root
        XMLWriter.writeEndDocument();

        docInfo.XMLText = XMLStr; //для сохранения спецсимволов и кодировки сохряняем документ в base64

        res.push_back(docInfo);

        isFound = true;
    }

    DBCommit(_topazDB);

    if (isFound)
    {
        _cnf->set_topaz_LastOffActID(lastID);
    }

    return res;
}

