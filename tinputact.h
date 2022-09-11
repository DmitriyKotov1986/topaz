#ifndef TINPUTACT_H
#define TINPUTACT_H

//QT
#include <QObject>
#include <QTimer>
#include <QMap>
#include <QDateTime>

//My
#include "tdoc.h"
#include "Common/tdbloger.h"

namespace Topaz
{

class TInputAct final: public QObject, public TDoc
{
    Q_OBJECT
public:
    explicit TInputAct(QSqlDatabase& db);
    ~TInputAct();

    TDoc::TDocInfo getNewDoc(uint number) override; //возвращает текст XML документа

private slots:
    void deleteFiles();
    void addFileForDelete(int number);

private:
    QTimer* _deleteFileTimer = nullptr;
    Common::TDBLoger* _loger = nullptr;

    QMap<QString, QDateTime> _filesForDelete; //список файлов на удаление. ключ - имя файла, значение - время загрузки документа в Топаз-АЗС
};

} //napespace Topaz

#endif // TINPUTACT_H
