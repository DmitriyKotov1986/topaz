#include "tcoupons.h"

//QT
#include <QSqlQuery>
#include <QXmlStreamWriter>

//My
#include "Common/common.h"
#include "tconfig.h"

using namespace Topaz;

using namespace Common;

static const QString DOC_NAME = "Coupons";

Topaz::TCoupons::TCoupons()
  : TDoc()
  , _loger(TDBLoger::DBLoger())
  , _cnf(TConfig::config())
{
  Q_CHECK_PTR(_loger);

  //настраиваем подключение к БД Топаза
  _topazDB = QSqlDatabase::addDatabase(_cnf->topazdb_Driver(), QString("CouponsTopazDB%1").arg(_cnf->topaz_OffActCode()));
  _topazDB.setDatabaseName(_cnf->topazdb_DBName());
  _topazDB.setUserName(_cnf->topazdb_UserName());
  _topazDB.setPassword(_cnf->topazdb_Password());
  _topazDB.setConnectOptions(_cnf->topazdb_ConnectOptions());
  _topazDB.setPort(_cnf->topazdb_Port());
  _topazDB.setHostName(_cnf->topazdb_Host());

  //подключаемся к БД Топаз-АЗС
  if (!_topazDB.open())
  {
      QString _errorString = QString("Cannot connect to Topaz_AZS database. Error: %1").arg(_topazDB.lastError().text());

      return;
  }

  quint64 id = 0;
  if (!checkLastID("rgCoupons", "CouponID", "LastCouponsID", _cnf->topaz_LastCouponsID(), id))
  {
      _cnf->set_topaz_LastCouponsID(id);
  }
}

Topaz::TCoupons::~TCoupons()
{
    if (_topazDB.isOpen())
    {
        _topazDB.close();
    }
}


Topaz::TDoc::TDocsInfo TCoupons::getDoc()
{
    _loger->sendLogMsg(TDBLoger::INFORMATION_CODE, "Launching Coupons sales detection");

    auto lastID = _cnf->topaz_LastCouponsID();

    //получаем список новых документов
    QSqlQuery query(_topazDB);
    _topazDB.transaction();

    QString queryText = QString("SELECT C.\"CouponID\", \"UserName\", C.\"Date\", \"SessionNum\", C.\"DocID\", C.\"Volume\", "
                                "       C.\"CouponCode\", C.\"CouponFuelName\", C.\"CouponVolume\", C.\"VolumeFact\" "
                                "FROM \"sysSessions\" AS A "
                                "INNER JOIN ( "
                                "    SELECT \"CouponID\", \"Date\", \"SessionID\", \"DocID\", \"Volume\", \"CouponCode\", \"CouponFuelName\", \"CouponVolume\", \"VolumeFact\" "
                                "    FROM \"rgCoupons\" AS B "
                                "    WHERE \"CouponID\" > %1 "
                                "     ) AS C "
                                "ON C.\"SessionID\" = A.\"SessionID\" "
                                "ORDER BY \"CouponID\" ")
                            .arg(lastID);

    writeDebugLogFile(QString("QUERY TO %1>").arg(_topazDB.connectionName()), queryText);

    if (!query.exec(queryText))
    {
        errorDBQuery(_topazDB, query);
    }

    TDocsInfo res;
    bool isFound = false;
    while (query.next())
    {
        TDoc::DocInfo docInfo;
        docInfo.number = query.value("DocID").toInt();
        docInfo.dateTime = query.value("Date").toDateTime();
        docInfo.smena = query.value("SessionNum").toUInt();
        docInfo.creater = query.value("UserName").toString();
        docInfo.type = DOC_NAME;

        _loger->sendLogMsg(TDBLoger::INFORMATION_CODE,
                           QString("-->Detect coupons sales.Number: %1, Smena: %2, Operator: %3, Date: %4, Volume: %5, Product: %6")
                                .arg(docInfo.number)
                                .arg(docInfo.smena)
                                .arg(docInfo.creater)
                                .arg(docInfo.dateTime.toString("yyyy-MM-dd hh:mm:ss.zzz"))
                                .arg(query.value("Volume").toString())
                                .arg(query.value("CouponFuelName").toString()));

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

        XMLWriter.writeStartElement("Coupons");

        do
        {
            XMLWriter.writeStartElement("Coupon");
            XMLWriter.writeTextElement("Code", query.value("CouponCode").toString());
            XMLWriter.writeTextElement("CouponVolume", query.value("CouponVolume").toString());
            XMLWriter.writeTextElement("VolumeFact", query.value("VolumeFact").toString());
            XMLWriter.writeEndElement(); //Coupon

            lastID = std::max(query.value("CouponID").toULongLong(), lastID);
        }
        while (query.next() && (docInfo.number == query.value("DocID").toInt()));
        query.previous();

        XMLWriter.writeEndElement(); //Coupons

        XMLWriter.writeStartElement("Sales");
        XMLWriter.writeTextElement("VolumeFactTotal", query.value("Volume").toString());
        XMLWriter.writeTextElement("FuelName", query.value("CouponFuelName").toString());
        XMLWriter.writeEndElement(); //Sales

        _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE,
                           QString("Find sales of coupons. ID: %1. Document ID: %2. Total volume: %3")
                                .arg(query.value("CouponID").toString())
                                .arg(docInfo.number)
                                .arg(query.value("Volume").toString()));

        XMLWriter.writeEndElement(); //Root

        docInfo.XMLText = XMLStr;

        res.push_back(docInfo);

        isFound = true;
    }

    DBCommit(_topazDB);

    if (isFound)
    {
        _cnf->set_topaz_LastCouponsID(lastID);
    }

    return res;
}

