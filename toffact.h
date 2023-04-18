#ifndef TOFFACT_H
#define TOFFACT_H

//Qt
#include <QSqlDatabase>

//My
#include "tdoc.h"
#include "Common/tdbloger.h"
#include "tconfig.h"

namespace Topaz
{

class TOffAct final: public TDoc
{
public:
    explicit TOffAct();
    ~TOffAct();

    TDocsInfo getDoc() override; //возвращает текст XML документа и прочие данные

private:
    Common::TDBLoger* _loger = nullptr;
    Topaz::TConfig* _cnf = nullptr;

    QTimer* _deleteFileTimer = nullptr;
    QSqlDatabase _topazDB;
};

} //namespace Topaz

#endif // TOFFACT_H
