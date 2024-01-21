#ifndef TQUERY_H
#define TQUERY_H

#include "tdoc.h"

//Qt
#include <QSqlDatabase>
#include <QHash>

//My
#include "tdoc.h"
#include "Common/tdbloger.h"
#include "tconfig.h"
#include "queryqueue.h"

namespace Topaz
{

class TQuery final: public TDoc
{

public:
    explicit TQuery();
    ~TQuery();

    TDocsInfo getDoc() override; //возвращает текст XML документа и прочие данные

private:
    Common::TDBLoger* _loger = nullptr;
    Topaz::TConfig* _cnf = nullptr;
    QueryQueue* _queryQueue = nullptr;

    QSqlDatabase _topazDB;//список запросов к БД Топаз АЗС
};

} //namespace Topaz

#endif // TQUERY_H
