#include "tnewsmena.h"

//Qt
#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QXmlStreamWriter>

//My
#include "Common/common.h"

using namespace Topaz;
using namespace Common;

static const QString DOC_NAME = "NewSmena";

TNewSmena::TNewSmena()
    : TDoc()
    , _loger(TDBLoger::DBLoger())
    , _cnf(TConfig::config())
{
    Q_CHECK_PTR(_loger);
    Q_CHECK_PTR(_cnf);

    //настраиваем подключение к БД Топаза
    _topazDB = QSqlDatabase::addDatabase(_cnf->topazdb_Driver(), QString("SmenaTopazDB"));
    _topazDB.setDatabaseName(_cnf->topazdb_DBName());
    _topazDB.setUserName(_cnf->topazdb_UserName());
    _topazDB.setPassword(_cnf->topazdb_Password());
    _topazDB.setConnectOptions(_cnf->topazdb_ConnectOptions());
    _topazDB.setPort(_cnf->topazdb_Port());
    _topazDB.setHostName(_cnf->topazdb_Host());

    //подключаемся к БД Топаз-АЗС
    if (!_topazDB.open())
    {
        _errorString = QString("Cannot connect to Topaz_AZS database. Error: %1").arg(_topazDB.lastError().text());

        return;
    }

    _filesForDelete = _cnf->topaz_PriceFilesList();
    for (const auto& fileNameItem: _filesForDelete)
    {
        _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("The file will be deleted: %1").arg(fileNameItem));
    }

    if (!_filesForDelete.isEmpty())
    {
        QTimer::singleShot(60000, this, SLOT(deleteFiles()));
    }

    quint64 id = 0;
    if (!checkLastID("sysSessions", "SessionID", "LastSmenaID", _cnf->topaz_LastSmenaID(), id))
    {
        _cnf->set_topaz_LastSmenaID(id);
    }
}

TNewSmena::~TNewSmena()
{
    if ( _topazDB.isOpen())
    {
        _topazDB.close();
    }
}

TDoc::TDocsInfo TNewSmena::getDoc()
{
    _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, "Launching new smens detection");

    _topazDB.transaction();
    QSqlQuery query(_topazDB);

    QString queryText =
            QString("SELECT \"SessionID\", \"SessionNum\", \"UserName\", \"StartDateTime\", \"EndDateTime\", \"Deleted\" "
                    "FROM \"sysSessions\" "
                    "WHERE \"SessionID\" > %1"
                    "ORDER BY \"SessionID\" ")
                .arg(_cnf->topaz_LastSmenaID());

    writeDebugLogFile(QString("QUERY TO %1>").arg(_topazDB.connectionName()), queryText);

    if (!query.exec(queryText))
    {
        errorDBQuery(_topazDB, query);
    }

    TDocsInfo res;
    auto currentSmenaID = _cnf->topaz_LastSmenaID();
    while (query.next())
    {
        SmenaInfo tmp;
        tmp.smenaID = query.value("SessionID").toULongLong();
        tmp.smenaNumber = query.value("SessionNum").toInt();
        tmp.operatorName = query.value("UserName").toString();
        tmp.startDateTime = query.value("StartDateTime").toDateTime();
        tmp.isOpen = !query.value("EndDateTime").isNull();
        if (!tmp.isOpen)
        {
            tmp.endDateTime = query.value("EndDateTime").toDateTime();
        }
        tmp.isDeleted = query.value("Deleted").toBool();

        _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE,
                           QString("-->Detect start new smen. ID: %1, Number: %2, Operator: %3, Start: %4, End: %5, Deleted: %6")
                                .arg(tmp.smenaID)
                                .arg(tmp.smenaNumber)
                                .arg(tmp.operatorName)
                                .arg(tmp.startDateTime.toString("yyyy-MM-dd hh:mm:ss.zzz"))
                                .arg((tmp.isOpen ? "---" : tmp.endDateTime.toString("yyyy-MM-dd hh:mm:ss.zzz")))
                                .arg(tmp.isDeleted));

        TDoc::DocInfo docInfo;
        docInfo.number = query.value("SessionID").toInt();
        docInfo.dateTime = QDateTime::currentDateTime();
        docInfo.smena = query.value("SessionNum").toInt();
        docInfo.creater = query.value("UserName").toString();
        docInfo.type = DOC_NAME;

        //формируем xml
        QString XMLStr;
        QXmlStreamWriter XMLWriter(&XMLStr);
        XMLWriter.setAutoFormatting(true);
        XMLWriter.writeStartDocument("1.0");
        XMLWriter.writeStartElement("Root");

        XMLWriter.writeTextElement("AZSCode", _cnf->srv_UserName());
        XMLWriter.writeTextElement("DocumentType", docInfo.type);
        XMLWriter.writeTextElement("DocumentNumber", QString::number(docInfo.number));
        XMLWriter.writeTextElement("Session", QString::number(docInfo.smena));
        XMLWriter.writeTextElement("Creater", docInfo.creater);
        XMLWriter.writeTextElement("DocumentDateTime", docInfo.dateTime.toString("yyyy-MM-dd hh:mm:ss.zzz"));

        XMLWriter.writeTextElement("StartDateTime", tmp.startDateTime.toString("yyyy-MM-dd hh:mm:ss.zzz"));
        if (!tmp.isOpen)
        {
            XMLWriter.writeTextElement("EndDateTime", tmp.endDateTime.toString("yyyy-MM-dd hh:mm:ss.zzz"));
        }
        XMLWriter.writeTextElement("IsDeleted", tmp.isDeleted ? "true" : "false");

        XMLWriter.writeEndElement(); //root
        XMLWriter.writeEndDocument();

        docInfo.XMLText = XMLStr; //для сохранения спецсимволов и кодировки сохряняем документ в base64

        res.push_back(docInfo);

        currentSmenaID = std::max(tmp.smenaID, currentSmenaID);  
    }

    DBCommit(_topazDB);

    if (currentSmenaID > _cnf->topaz_LastSmenaID())
    {
        if (_cnf->topaz_PriceDeleteFile())
        {
            if (addFileForDelete())
            {
                QTimer::singleShot(60000, this, SLOT(deleteFiles()));
            }
        }

        _cnf->set_topaz_LastSmenaID(currentSmenaID);
    }

    return res;
}


void Topaz::TNewSmena::deleteFiles()
{
    for (auto fileName : _filesForDelete)
    {
        if (!QFileInfo::exists(fileName))
        {
            _loger->sendLogMsg(TDBLoger::MSG_CODE::OK_CODE, QString("File %1 already deleted").arg(fileName));
        }
        else
        {
            QFile file(fileName);
            if (file.remove())
            {
                _loger->sendLogMsg(TDBLoger::MSG_CODE::OK_CODE, QString("File deleted. File name: %1").arg(fileName));
            }
            else
            {
                _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE,
                    QString("Cannot deleted file. Skip. File name: %1. Error: %2")
                        .arg(fileName).arg(file.errorString()));
            }
        }
    }

    _filesForDelete.clear();

    _cnf->set_topaz_PriceFilesList(_filesForDelete);
}

bool Topaz::TNewSmena::addFileForDelete()
{
    //получаем список файлов в ImportDoc
    QDir topazImportDir(_cnf->topaz_DirName() + "/ImportDoc");
    topazImportDir.setFilter(QDir::NoDotAndDotDot | QDir::Files | QDir::NoSymLinks | QDir::Hidden);
    auto fileList = topazImportDir.entryInfoList();

    bool isFound = false;
    //ищем файл с нужным именем
    for (const auto& fileInfoItem: fileList)
    {
        QString fileName = fileInfoItem.absoluteFilePath();
        if (fileName.indexOf("price") > 0)
        {
            _filesForDelete.push_back(fileName);
            _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("-->Find file Price File name: %1").arg(fileName));

            isFound = true;
        }
    }

    if (isFound)
    {
        _cnf->set_topaz_PriceFilesList(_filesForDelete);
    }
    else
    {
        _loger->sendLogMsg(TDBLoger::MSG_CODE::OK_CODE, QString("-->Cannot find file for Price."));
    }

    return isFound;
}
