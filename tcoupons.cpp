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
    _loger(Common::TDBLoger::DBLoger())
{
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

}

void TCoupons::getCoupons()
{
    //получаем список новых документов
    QSqlQuery query(_db);
    query.setForwardOnly(true);

    _db.transaction();
    QString queryText = QString("SELECT \"CouponID\", \"Date\", "SessionID\", "DocID\", \"Volume"\, \"CouponCode\", \"CouponFuelName\", \"CouponVolume\", \"VolumeFact\" "
                                "FROM \"rgCoupons\" "
                                "WHERE \"CouponID\" > %1")
                            .arg(_cnf->topaz_LastCouponsID());

    writeDebugLogFile(QString("QUERY TO %1>").arg(_db.connectionName()), queryText);

    if (!query.exec(queryText))
    {
        errorDBQuery(_db, query);
        return;
    }

    //создаем список новых документов










    typedef struct
    {
        int DocType;
        int DocID;
    } TNewDoc;

    QList<TNewDoc> newDocsList;

    int lastDocNumber = _cnf->topaz_LastDocNumber();
    while (query.next())
    {
        TNewDoc tmp;
        tmp.DocID = query.value("DocID").toInt();
        tmp.DocType = query.value("DocTypeID").toInt();
        newDocsList.push_back(tmp);

        lastDocNumber = std::max(lastDocNumber, query.value("rgItemRestID").toInt());
    }

    query.finish();

    DBCommit(_db);

}
