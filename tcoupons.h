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

    TDoc::TDocInfo getNewDoc(uint number) override; //возвращает текст XML документа
    bool isEmpty() const { return _docs.isEmpty(); }

private slots:
    void getCoupons();
    int getLastDocNumber();

private:
    QTimer* _getCouponsTimer = nullptr;
    Common::TDBLoger* _loger = nullptr;
    Topaz::TConfig* _cnf = nullptr;

    QQueue<TDoc::TDocInfo> _docs;
};

} //namespace Topaz

#endif // TCOUPONS_H

