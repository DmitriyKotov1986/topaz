#include "tinputact.h"

//QT
#include <QDir>
#include <QFileInfo>
#include <QSqlQuery>
#include <QXmlStreamWriter>

//My
#include "Common/common.h"
#include "tconfig.h"

using namespace Topaz;

using namespace Common;

Topaz::TInputAct::TInputAct(QSqlDatabase &db) :
    TDoc(db, TConfig::config()->topaz_InputActCode()),
    _loger(Common::TDBLoger::DBLoger())
{
    _deleteFileTimer = new QTimer;

    QObject::connect(_deleteFileTimer, SIGNAL(timeout()), SLOT(deleteFiles()));

    _deleteFileTimer->start(60000);
}

Topaz::TInputAct::~TInputAct()
{
    Q_CHECK_PTR(_deleteFileTimer);

    if (_deleteFileTimer != nullptr)
    {
        _deleteFileTimer->deleteLater();
    }
}

Topaz::TDoc::TDocInfo Topaz::TInputAct::getNewDoc(uint number)
{
    _errorString.clear();

    QSqlQuery query(_db);
    query.setForwardOnly(true);
    _db.transaction();

    QString queryText =
        QString("SELECT \"NDoc\", \"DateDoc\" "
                "FROM \"trInBillH\" AS A "
                "INNER JOIN (SELECT \"DocID\" "
                "            FROM \"rgItemRests\" "
                "            WHERE \"DocID\" = (%1)) AS B "
                "ON (A.\"InBillHID\" = B.\"DocID\") ")
                .arg(number);

    writeDebugLogFile(QString("QUERY TO %1>").arg(_db.connectionName()), queryText);

    if (!query.exec(queryText)) {
        errorDBQuery(_db, query);
        return TDoc::TDocInfo();
    }

    TDoc::TDocInfo res;

    if (query.first()) {
        res.number = query.value("NDoc").toInt();
        res.dateTime = QDateTime::currentDateTime();
        res.smena = 0;
        res.creater = QString();
        res.type = QString("InputAct").toUtf8();
    }
    else {
        _errorString = QString("Document is empty. No find documents of number: %1").arg(number);
        return TDoc::TDocInfo();
    }

    //формируем xml
    QString XMLStr;
    QXmlStreamWriter XMLWriter(&XMLStr);
    XMLWriter.setAutoFormatting(true);
    XMLWriter.writeStartDocument("1.0");
    XMLWriter.writeStartElement("Root");
    XMLWriter.writeTextElement("AZSCode", _cnf->srv_UserName());
    XMLWriter.writeTextElement("DocumentType", res.type);
    XMLWriter.writeTextElement("DocumentNumber", QString::number(res.number));
    XMLWriter.writeTextElement("Session", QString::number(res.smena));
    XMLWriter.writeTextElement("Creater", res.creater);
    XMLWriter.writeTextElement("DocumentDateTime", res.dateTime.toString("yyyy-MM-dd hh:mm:ss.zzz"));
    query.finish();
    DBCommit(_db);

    res.XMLText = XMLStr.toUtf8().toBase64(QByteArray::Base64Encoding); //для сохранения спецсимволов и кодировки сохряняем документ в base64


    addFileForDelete(res.number);

    return res;
}

void TInputAct::deleteFiles()
{
    for (auto fileForDeleteItem = _filesForDelete.begin(); fileForDeleteItem != _filesForDelete.end(); ++fileForDeleteItem)
    {
        //если прошло меньше 1 минута - пропускаем файл
        if (fileForDeleteItem.value().secsTo(QDateTime::currentDateTime()) < (60))
        {
            continue;
        }
        if (!QFileInfo::exists(fileForDeleteItem.key())) {
            _loger->sendLogMsg(TDBLoger::INFORMATION_CODE, QString("File %1 already deleted.").arg(fileForDeleteItem.key()));
        }
        else
        {
            QDir topazImportDir;
            if (topazImportDir.remove(fileForDeleteItem.key()))
            {

                _loger->sendLogMsg(TDBLoger::OK_CODE, QString("File deleted. File name: %1").arg(fileForDeleteItem.key()));
            }
            else {
                _loger->sendLogMsg(TDBLoger::INFORMATION_CODE, QString("Cannot deleted file. Skip. File name: %1").arg(fileForDeleteItem.key()));
            }
            fileForDeleteItem = _filesForDelete.erase(fileForDeleteItem);
        }
    }
}

void TInputAct::addFileForDelete(int number)
{
    //получаем список файлов в ImportDoc
    QDir topazImportDir(_cnf->topaz_DirName() + "/ImportDoc");
    topazImportDir.setFilter(QDir::NoDotAndDotDot | QDir::Files | QDir::NoSymLinks | QDir::Hidden);
    auto fileList = topazImportDir.entryInfoList();
    //ищем файл с нужным именем
    for (const auto& fileInfoItem: fileList)
    {
        QString fileName = fileInfoItem.absoluteFilePath();
        if (fileName.indexOf(QString("00%1.xml").arg(number)) != -1 )
        {
            _filesForDelete.insert(fileName, QDateTime::currentDateTime());
            _loger->sendLogMsg(TDBLoger::INFORMATION_CODE, QString("Find file for document ID: %1. File name: %2")
                .arg(number).arg(fileName));

            return;
        }
    }
    _loger->sendLogMsg(TDBLoger::INFORMATION_CODE, QString("Cannot find file for document ID: %1").arg(number));
}
