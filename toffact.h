#ifndef TOFFACT_H
#define TOFFACT_H

//Qt
#include <QSqlDatabase>

//My
#include "tdoc.h"

namespace Topaz {

class TOffAct final: public TDoc
{
public:
    explicit TOffAct(QSqlDatabase& db);

    TDoc::TDocInfo getNewDoc(uint number) override; //возвращает текст XML документа
};

} //namespace Topaz

#endif // TOFFACT_H
