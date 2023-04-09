#ifndef TDBLOGER_H
#define TDBLOGER_H

//Qt
#include <QObject>
#include <QSqlDatabase>

namespace Common
{

class TDBLoger final: public QObject
{
    Q_OBJECT

public:
    static TDBLoger* DBLoger(QSqlDatabase* db = nullptr, bool debugMode = true, QObject* parent = nullptr);
    static void deleteDBLoger();

public:
    enum MSG_CODE : int
    {
        OK_CODE = 0,
        INFORMATION_CODE = 1,
        WARNING_CODE = 2,
        ERROR_CODE= 3,
        CRITICAL_CODE = 4
    };

private:
    explicit TDBLoger(QSqlDatabase& db, bool debugMode = true, QObject* parent = nullptr);
    ~TDBLoger();

public:
    bool isError() const { return !_errorString.isEmpty(); }
    QString errorString();

public slots:
    void sendLogMsg(const int category, const QString& msg); //Сохранения логов

private:
    QSqlDatabase _db;
    bool _debugMode = true;

    QString _errorString;

};

} //namespace Common

#endif // TDBLOGER_H
