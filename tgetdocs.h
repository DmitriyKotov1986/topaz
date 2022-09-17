#ifndef TGETDOCS_H
#define TGETDOCS_H

//QT
#include <QSqlDatabase>
#include <QMap>
#include <QDateTime>

//My
#include "tconfig.h"
#include "tdoc.h"
#include "tcoupons.h"
#include "Common/tdbloger.h"

namespace Topaz {

class TGetDocs final
{
public:
    typedef QList<Topaz::TDoc::TDocInfo> TDocsInfoList;

public:
    explicit TGetDocs(QSqlDatabase& db);
    ~TGetDocs();

    TDocsInfoList getDocs(); //возвращает список документов у которых номер больше чем _cnf.topaz_LastDocNumber()

    QString errorString() const { return _errorString; } //возвращает текст ошибки
    bool isError() const { return !_errorString.isEmpty(); } //возвращает наличие ошибки

private:
    TConfig* _cnf = nullptr;
    QSqlDatabase _db;
    Common::TDBLoger* _loger = nullptr;

    QMap<int, TDoc*> _docs; //карта новых документов, ключ - код документа в БД топаза, значение - класс обработки документа

    QString _errorString;

    TCoupons* _coupons = nullptr; //Класс получения документов продажи талонов
};

} // namespace Topaz

#endif // TGETDOCS_H
