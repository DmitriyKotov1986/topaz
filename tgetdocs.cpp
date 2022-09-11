#include "tgetdocs.h"

//Qt
#include <QSqlQuery>
#include <QSqlError>

//My
#include "toffact.h"
#include "tinputact.h"
#include "Common/common.h"

using namespace Topaz;

using namespace Common;

Topaz::TGetDocs::TGetDocs(QSqlDatabase& db) :
    _cnf(Topaz::TConfig::config()),
    _db(db),
    _loger(TDBLoger::DBLoger())
{
    Q_CHECK_PTR(_cnf);
    Q_CHECK_PTR(_loger);
    Q_ASSERT(db.isValid());

    //фабрика загружаемых документов
    //добавляем документы которые необходимо выдергивать из Топаза
    if (_cnf->topaz_OffActEnabled())
    {
        _docs.insert(_cnf->topaz_OffActCode(), new TOffAct(db));
    }
    if (_cnf->topaz_InputActDeleteFile())
    {
        _docs.insert(_cnf->topaz_InputActCode(), new TInputAct(db));
    }
}

Topaz::TGetDocs::~TGetDocs()
{
    for (auto docItem = _docs.begin(); docItem != _docs.end(); ++docItem)
    {
        delete docItem.value();
    }
}

Topaz::TGetDocs::TDocsInfoList Topaz::TGetDocs::getDocs()
{
    _errorString.clear();

    //получаем список новых документов
    QSqlQuery query(_db);
    query.setForwardOnly(true);

    _db.transaction();
    QString queryText = QString("SELECT MAX(\"rgItemRestID\") as \"rgItemRestID\", \"DocTypeID\", \"DocID\" "
                                "FROM \"rgItemRests\" "
                                "WHERE \"rgItemRestID\" > %1 "
                                "GROUP BY \"DocTypeID\", \"DocID\" ")
                                .arg(_cnf->topaz_LastDocNumber());

    writeDebugLogFile(QString("QUERY TO %1>").arg(_db.connectionName()), queryText);

    if (!query.exec(queryText))
    {
        errorDBQuery(_db, query);
        return TDocsInfoList();
    }

    //создаем список новых документов

    typedef struct
    {
        int DocType;
        int DocID;
    } TNewDoc;

    QList<TNewDoc> newDocsList;

    int lastDocNumber = _cnf->topaz_LastDocNumber();
    while (query.next())
    {
        TNewDoc tmp;
        tmp.DocID = query.value("DocID").toInt();
        tmp.DocType = query.value("DocTypeID").toInt();
        newDocsList.push_back(tmp);

        lastDocNumber = std::max(lastDocNumber, query.value("rgItemRestID").toInt());
    }

    query.finish();

    DBCommit(_db);

    //обрабатываем новые документы по очереди
    TDocsInfoList res;
    for (auto newDocItem: newDocsList)
    {
        //если есть обработчик для данного типа документов - запускаем его
        if (_docs.contains(newDocItem.DocType))
        {
            auto newDocInfo = _docs[newDocItem.DocType]->getNewDoc(newDocItem.DocID);

            if (_docs[newDocItem.DocType]->isError())
            {
                _loger->sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, QString("Document from Topaz-AZS processing error. Type: %1. ID: %2. Msg: %3")
                                   .arg(newDocItem.DocType).arg(newDocItem.DocID).arg(_docs[newDocItem.DocType]->errorString()));
                continue;
            }
            else
            {
                _loger->sendLogMsg(TDBLoger::MSG_CODE::OK_CODE, QString("Document from Topaz-AZS processing success finished. Type: %1. ID: %2")
                                   .arg(newDocItem.DocType).arg(newDocItem.DocID));
                res.push_back(newDocInfo);
            }
        }
        else
        {
            _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("Unknow document type: %1. ID: %2")
                               .arg(newDocItem.DocType).arg(newDocItem.DocID));
        }

    }

    _cnf->set_topaz_LastDocNumber(lastDocNumber);
    _cnf->save();

    return res;
}
