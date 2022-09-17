#include "tcoupons.h"

//QT
#include <QSqlQuery>
#include <QXmlStreamWriter>

//My
#include "Common/common.h"
#include "tconfig.h"

using namespace Topaz;

using namespace Common;

Topaz::TCoupons::TCoupons(QSqlDatabase& db) :
    TDoc(db, TConfig::config()->topaz_CouponsCode()),
    _loger(Common::TDBLoger::DBLoger()),
    _cnf(Topaz::TConfig::config())
{
    Q_CHECK_PTR(_loger);
    Q_CHECK_PTR(_cnf);
    Q_ASSERT(db.isOpen());

    _getCouponsTimer = new QTimer();

    QObject::connect(_getCouponsTimer, SIGNAL(timeout()), SLOT(getCoupons()));

    _getCouponsTimer->start(60000);
}

Topaz::TCoupons::~TCoupons()
{
    if (_getCouponsTimer != nullptr)
    {
        delete _getCouponsTimer;
    }
}

TDoc::TDocInfo Topaz::TCoupons::getNewDoc(uint number)
{
    if (!_docs.isEmpty())
    {
        return _docs.dequeue();
    }
}

void TCoupons::getCoupons()
{
    int lastDocNumber = _cnf->topaz_LastCouponsID();

    //получаем список новых документов
    QSqlQuery query(_db);
    query.setForwardOnly(true);

    _db.transaction();
    QString queryText = QString("SELECT \"CouponID\", \"Date\", \"SessionID\", \"DocID\", \"Volume\", \"CouponCode\", \"CouponFuelName\", \"CouponVolume\", \"VolumeFact\" "
                                "FROM \"rgCoupons\" "
                                "WHERE \"CouponID\" > %1 "
                                "ORDER BY \"CouponID\" ")
                            .arg(lastDocNumber);

    writeDebugLogFile(QString("QUERY TO %1>").arg(_db.connectionName()), queryText);

    if (!query.exec(queryText))
    {
        errorDBQuery(_db, query);
        return;
    }

    int currentDocId = -1;
    TDoc::TDocInfo res;
    QString XMLStr;
    QXmlStreamWriter* XMLWriter = nullptr;

    while (query.next())
    {
        if (query.value("DocID").toInt() != currentDocId)
        {
            if (XMLWriter != nullptr)
            {
                XMLWriter->writeEndElement(); //Root
                XMLStr.toUtf8().toBase64(QByteArray::Base64Encoding); //для сохранения спецсимволов и кодировки сохряняем документ в base64
                _docs.enqueue(res);

                delete XMLWriter;
            }

            res.number = query.value("DocID").toInt();
            res.dateTime = QDateTime::currentDateTime();
            res.smena = query.value("SessionID").toInt();
            res.creater = QString();
            res.type = QString("Coupons").toUtf8();

            //формируем xml
            XMLStr.clear();
            XMLWriter = new QXmlStreamWriter(&XMLStr);

            XMLWriter->setAutoFormatting(true);
            XMLWriter->writeStartDocument("1.0");
            XMLWriter->writeStartElement("Root");
            XMLWriter->writeTextElement("AZSCode", _cnf->srv_UserName());
            XMLWriter->writeTextElement("DocumentType", res.type);
            XMLWriter->writeTextElement("DocumentNumber", QString::number(res.number));
            XMLWriter->writeTextElement("Session", QString::number(res.smena));
            XMLWriter->writeTextElement("Creater", res.creater);
            XMLWriter->writeTextElement("DocumentDateTime", res.dateTime.toString("yyyy-MM-dd hh:mm:ss.zzz"));

            XMLWriter->writeStartElement("Sales");
            XMLWriter->writeTextElement("VolumeFactTotal", query.value("Volume").toString());
            XMLWriter->writeTextElement("FuelName", query.value("CouponFuelName").toString().toUtf8());
            XMLWriter->writeEndElement(); //Sales

            XMLWriter->writeStartElement("Coupon");
            XMLWriter->writeTextElement("Code", query.value("Code").toString());
            XMLWriter->writeTextElement("CouponVolume", query.value("CouponVolume").toString());
            XMLWriter->writeTextElement("VolumeFact", query.value("VolumeFact").toString());
            XMLWriter->writeEndElement(); //Coupon

            _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("Find sales of coupons. Document ID: %1. Total volume: %2")
                .arg(res.number).arg(query.value("Volume").toString()));
        }
        else
        {
            Q_CHECK_PTR(XMLWriter);

            XMLWriter->writeStartElement("Coupon");
            XMLWriter->writeTextElement("Code", query.value("Code").toString());
            XMLWriter->writeTextElement("CouponVolume", query.value("CouponVolume").toString());
            XMLWriter->writeTextElement("VolumeFact", query.value("VolumeFact").toString());
            XMLWriter->writeEndElement(); //Coupon
        }

        lastDocNumber = std::max(lastDocNumber, query.value("CouponsID").toInt());
    }

    query.finish();
    DBCommit(_db);

    if (_cnf->topaz_LastCouponsID() != lastDocNumber)
    {
        _cnf->set_topaz_LastCouponsID(lastDocNumber);
        _cnf->save();
    }
}
