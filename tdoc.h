//Класс родитель для всех документов Топаз-АЗС
#ifndef TDOC_H
#define TDOC_H

//Qt
#include <QSqlDatabase>
#include <QDateTime>

//My
#include "tconfig.h"

namespace Topaz {

class TDoc
{
public:
    typedef struct
    {
        QString XMLText;
        int number;
        int smena;
        QString type;
        QDateTime dateTime;
        QString creater;
    } TDocInfo;

    typedef struct
    {
        int snmena;
        QString operatorName;
    } TSmenaInfo;

public:
    explicit TDoc(QSqlDatabase& db, int docTypeCode) : //в конструкторе наследника необходимо определить значение _docTypeCode
        _cnf(TConfig::config()),
        _db(db),
        _docTypeCode(docTypeCode)
    {
        Q_CHECK_PTR(_cnf);
        Q_ASSERT(db.isValid());
        Q_ASSERT(docTypeCode >= 0);
    }

    virtual ~TDoc() {};

    virtual TDocInfo getNewDoc(uint number) = 0; //возвращает текст XML документа

    int docTypeCode() const { //возвращает код документа
        Q_ASSERT(_docTypeCode != -1);
        return _docTypeCode;
    }

    QString errorString() const { return _errorString; } //возвращает текст ошибки
    bool isError() const { return !_errorString.isEmpty(); } //возвращает наличие ошибки

protected:
    QString _errorString;
    TConfig* _cnf = nullptr;
    QSqlDatabase& _db;
    int _docTypeCode = -1;
};

} //Topaz

#endif // TDOC_H
