#ifndef TINPUTACT_H
#define TINPUTACT_H

//QT
#include <QObject>
#include <QTimer>
#include <QMap>
#include <QDateTime>

//My
#include "tdoc.h"
#include "tconfig.h"
#include "Common/tdbloger.h"

namespace Topaz
{

class TInputAct final: public TDoc
{
    Q_OBJECT

public:
    explicit TInputAct();
    ~TInputAct();

    TDocsInfo getDoc() override; //возвращает текст XML документа и прочие данные

private slots:
    void deleteFiles();
    void addFileForDelete(int number);

private:
    QString makeXML(const TDoc::DocInfo& docInfo);

private:
    Common::TDBLoger* _loger = nullptr;
    Topaz::TConfig* _cnf = nullptr;

    QTimer* _deleteFileTimer = nullptr;
    QSqlDatabase _topazDB;

    QMap<QString, QDateTime> _filesForDelete; //список файлов на удаление. ключ - имя файла, значение - время загрузки документа в Топаз-АЗС
};

} //napespace Topaz

#endif // TINPUTACT_H
