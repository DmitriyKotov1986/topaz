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

class TCoupons : public QObject, public TDoc
{
    Q_OBJECT

public:
    explicit TCoupons(QSqlDatabase& db);
    ~TCoupons();

public:
    TDoc::TDocInfo getNewDoc(uint number) override; //возвращает текст XML документа

private:
    void getCoupons();

private:
    QTimer* _getCouponsTimer = nullptr;
    Common::TDBLoger* _loger = nullptr;

    QQueue<TDoc::TDocInfo> _docs;
};

} //namespace Topaz

#endif // TCOUPONS_H

