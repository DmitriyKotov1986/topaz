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

TDoc::TDocInfo Topaz::TOffAct::getNewDoc(uint number)
{
    _errorString.clear();

    QSqlQuery query(_db);
    query.setForwardOnly(true);
    _db.transaction();

    QString queryText =
        QString("SELECT F.\"SessionNum\", F.\"UserName\", J.\"NDoc\", J.\"Date\", \"FullName\", \"ExtCode\", J.\"Quantity\" "
                "FROM \"sysSessions\" AS F "
                "INNER JOIN ( "
                "    SELECT D.\"NDoc\", E.\"Date\", \"FullName\", \"ExtCode\", E.\"Quantity\"  "
                "    FROM \"trOutActH\" AS D "
                "    INNER JOIN ( "
                "        SELECT C.\"DocID\", C.\"Date\", \"FullName\", \"ExtCode\", C.\"Quantity\" "
                "        FROM \"dcItems\" AS A "
                "        INNER JOIN ( "
                "            SELECT \"Date\", \"DocID\", \"ItemID\", \"Quantity\" "
                "            FROM \"rgItemRests\" AS B "
                "            WHERE (B.\"DocTypeID\" = %1) AND (B.\"DocID\" = %2) "
                "            ) AS C "
                "        ON (A.\"ItemID\" = C.\"ItemID\") "
                "    ) AS E "
                "    ON (D.\"OutActHID\" = E.\"DocID\") "
                ") AS J "
                "ON ((J.\"Date\" > F.\"StartDateTime\") AND (J.\"Date\" < F.\"EndDateTime\") OR (F.\"EndDateTime\" is NULL)) "
                "ORDER BY J.\"NDoc\"").arg(_docTypeCode).arg(number);

    writeDebugLogFile(QString("QUERY TO %1>").arg(_db.connectionName()), queryText);

    if (!query.exec(queryText))
    {
        errorDBQuery(_db, query);
        return TDoc::TDocInfo();
    }

    TDoc::TDocInfo res;

    if (query.first())
    {
        res.number = query.value("NDoc").toInt();
        res.dateTime = query.value("Date").toDateTime();
        res.smena = query.value("SessionNum").toInt();
        res.creater = query.value("UserName").toString().toUtf8().toBase64(QByteArray::Base64Encoding);
        res.type = QString("OffAct").toUtf8();
    }
    else
    {
        _errorString = QString("Document is empty. No find documents of number: %1").arg(number);
        return TDoc::TDocInfo();
    }

    //формируем xml
    QString XMLStr;
    QXmlStreamWriter XMLWriter(&XMLStr);
    XMLWriter.setAutoFormatting(true);
    XMLWriter.writeStartDocument("1.0");
    XMLWriter.writeStartElement("Root");
    XMLWriter.writeTextElement("AZSCode", _cnf->srv_UserName());
    XMLWriter.writeTextElement("DocumentType", res.type);
    XMLWriter.writeTextElement("DocumentNumber", QString::number(res.number));
    XMLWriter.writeTextElement("Session", QString::number(res.smena));
    XMLWriter.writeTextElement("Creater", res.creater);
    XMLWriter.writeTextElement("DocumentDateTime", res.dateTime.toString("yyyy-MM-dd hh:mm:ss.zzz"));

    do
    {
        XMLWriter.writeStartElement("Item");
        XMLWriter.writeTextElement("ExtCode", query.value("ExtCode").toString());
        XMLWriter.writeTextElement("FullName", query.value("FullName").toString().toUtf8());
        XMLWriter.writeTextElement("Quantity", query.value("Quantity").toString());
        XMLWriter.writeEndElement();
    }
    while (query.next());

    XMLWriter.writeEndElement(); //root
    XMLWriter.writeEndDocument();

    query.finish();
    DBCommit(_db);

    res.XMLText = XMLStr.toUtf8().toBase64(QByteArray::Base64Encoding); //для сохранения спецсимволов и кодировки сохряняем документ в base64

    return res;
}
