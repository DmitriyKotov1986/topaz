#ifndef TNEWSMENA_H
#define TNEWSMENA_H

//QT
#include <QObject>

//My
#include "tdoc.h"
#include "Common/tdbloger.h"
#include "tconfig.h"

namespace Topaz
{

class TNewSmena : public TDoc
{
    Q_OBJECT

public:
    explicit TNewSmena();
    ~TNewSmena();

private:
    TDocsInfo getDoc() override;

    bool addFileForDelete();

private slots:
    void deleteFiles();

private:
    Common::TDBLoger* _loger = nullptr;
    Topaz::TConfig* _cnf = nullptr;

    QSqlDatabase _topazDB;

    QStringList _filesForDelete; //список файлов на удаление.

};  //class TNewSmena

} //namespace Topaz

#endif // TNEWSMENA_H
