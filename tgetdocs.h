#ifndef TGETDOCS_H
#define TGETDOCS_H

//QT
#include <QSqlDatabase>
#include <QMap>
#include <QDateTime>

//My
#include "tconfig.h"
#include "tdoc.h"

namespace Topaz
{

class TGetDocs final
{
public:
    explicit TGetDocs();
    ~TGetDocs();

    QString errorString(); //возвращает текст ошибки
    bool isError() const { return !_errorString.isEmpty(); } //возвращает наличие ошибки

private:
    TConfig* _cnf = nullptr;

    QList<TDoc*> _docs;
    QString _errorString;
};

} // namespace Topaz

#endif // TGETDOCS_H
