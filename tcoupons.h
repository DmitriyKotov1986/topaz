#ifndef TCOUPONS_H
#define TCOUPONS_H

//Qt
#include <QObject>
#include <QMap>
#include <QTimer>
#include <QQueue>

//My
#include "Common/tdbloger.h"
#include "tdoc.h"

namespace Topaz
{

class TCoupons: public TDoc
{
public:
    explicit TCoupons();
    ~TCoupons();

    TDocsInfo getDoc() override; //возвращает текст XML документа

private:
    Common::TDBLoger* _loger = nullptr;
    Topaz::TConfig* _cnf = nullptr;

    QSqlDatabase _topazDB;
};

} //namespace Topaz

#endif // TCOUPONS_H

