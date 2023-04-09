#include "tdbloger.h"

//Qt
#include <QSqlError>
#include <QSqlQuery>
#include <QCoreApplication>

//My
#include "common.h"

using namespace Common;

//static
static TDBLoger* DBLogerPtr = nullptr;

TDBLoger* TDBLoger::DBLoger(QSqlDatabase* db, bool debugMode, QObject* parent)
{
    if (DBLogerPtr == nullptr)
    {
        DBLogerPtr = new TDBLoger(*db, debugMode, parent);
    }

    return DBLogerPtr;
}

void TDBLoger::deleteDBLoger()
{
    Q_CHECK_PTR(DBLogerPtr);

    if (DBLogerPtr != nullptr)
    {
        delete DBLogerPtr;

        DBLogerPtr = nullptr;
    }
}

//class
TDBLoger::TDBLoger(QSqlDatabase& db, bool debugMode /* = true */, QObject* parent /* = nullptr */) :
    QObject(parent),
    _db(db),
    _debugMode(debugMode)
{
    Q_ASSERT(_db.isValid());

    //подключаемся к БД
    if ((_db.isOpen()) || (!_db.open()))
    {
        _errorString = QString("Cannot connect to log database. Error: %1").arg(_db.lastError().text());

        return;
    };
}

TDBLoger::~TDBLoger()
{
    if (_db.isOpen())
    {
        _db.close();
    }
}

QString TDBLoger::errorString()
{
    auto res = _errorString;
    _errorString.clear();

    return res;
}

void TDBLoger::sendLogMsg(int category, const QString& msg)
{
    if (category == MSG_CODE::CRITICAL_CODE)
    {
        qCritical() <<  QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
        writeLogFile("CRITICAL>", msg);
    }
    else if (_debugMode)
    {
        qDebug() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
    }

    writeDebugLogFile("LOG>", msg);

    QString queryText = "INSERT INTO LOG (CATEGORY, SENDER, MSG) VALUES ( "
                        + QString::number(category) + ", "
                        "\'" + QCoreApplication::applicationName()+ "\', "
                        "\'" + msg +"\'"
                        ")";

    Common::DBQueryExecute(_db, queryText);
}

