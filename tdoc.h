//Класс родитель для всех документов Топаз-АЗС
#ifndef TDOC_H
#define TDOC_H

//Qt
#include <QObject>
#include <QSqlDatabase>
#include <QDateTime>
#include <QTimer>
#include <QList>

//My
#include "Common/tdbloger.h"
#include "tconfig.h"

namespace Topaz
{

class TDoc: public QObject
{
    Q_OBJECT

protected:
    struct DocInfo
    {
        QString XMLText;
        int number = 0;
        int smena = 0;
        QString type;
        QDateTime dateTime;
        QString creater;
    };

    typedef QList<DocInfo> TDocsInfo;

    struct SmenaInfo
    {
        quint64 smenaID = 0;
        quint32 smenaNumber = 0;
        QString operatorName;
        QDateTime startDateTime;
        QDateTime endDateTime = QDateTime::fromString("2000-01-01 00:00:00.001", "yyyy-MM-dd hh:mm:ss:zzz");
        bool isOpen = false;
        bool isDeleted = false;
    };

public:
    explicit TDoc();
    virtual ~TDoc();

    QString errorString() const { return _errorString; } //возвращает текст ошибки
    bool isError() const { return !_errorString.isEmpty(); } //возвращает наличие ошибки

    int docTypeCode() const { return _docTypeCode; };

protected:
    SmenaInfo getCurrentSmena(); //возвращает номер текущей смены
    void addInfoMsg(const QString& msg);
    bool checkLastID(const QString& tableName, const QString& fieldName, const QString& idName, quint64 oldValue, quint64& newValue);

private slots:
    void workTimer_timeout();

private:
    virtual TDocsInfo getDoc() = 0;

    void saveDocToDB(const DocInfo& docInfo); //сохраняет документ в локальную БД

protected:
     QString _errorString;

private:
    TConfig* _cnf = nullptr;
    Common::TDBLoger* _loger = nullptr;
    QSqlDatabase _db;
    QSqlDatabase _topazDB;

    int _docTypeCode = -1;

    QTimer* _workTimer;
};

} //Topaz

#endif // TDOC_H
