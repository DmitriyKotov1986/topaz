#include "tinputact.h"

//QT
#include <QDir>
#include <QFileInfo>
#include <QSqlQuery>
#include <QXmlStreamWriter>

//My
#include "Common/common.h"

using namespace Topaz;

using namespace Common;

static const QString DOC_NAME = "InputAct";

Topaz::TInputAct::TInputAct()
    : TDoc()
    , _loger(TDBLoger::DBLoger())
    , _cnf(TConfig::config())
{
    Q_CHECK_PTR(_loger);
    Q_CHECK_PTR(_cnf);

    //настраиваем подключение к БД Топаза
    _topazDB = QSqlDatabase::addDatabase(_cnf->topazdb_Driver(), QString("InputActTopazDB%1").arg(_cnf->topaz_InputActCode()));
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

    //таймер удаления загруженных актов прихода
    _deleteFileTimer = new QTimer;

    QObject::connect(_deleteFileTimer, SIGNAL(timeout()), SLOT(deleteFiles()));

    _deleteFileTimer->start(_cnf->sys_Interval());

    quint64 id = 0;
    if (!checkLastID("trInBillH", "InBillHID", "LastInputActID", _cnf->topaz_LastInputActID(), id))
    {
        _cnf->set_topaz_LastInputActID(id);
    }

    for (const auto& fileNameItem : _cnf->topaz_InputActFilesList())
    {
        _filesForDelete.insert(fileNameItem, QDateTime::currentDateTime());
        _loger->sendLogMsg(TDBLoger::INFORMATION_CODE, QString("The file will be deleted: %1").arg(fileNameItem));
    }
}

Topaz::TInputAct::~TInputAct()
{
    Q_CHECK_PTR(_deleteFileTimer);

    if (_deleteFileTimer != nullptr)
    {
        delete _deleteFileTimer;
    }

    if (_topazDB.isOpen())
    {
        _topazDB.close();
    }
}

Topaz::TDoc::TDocsInfo Topaz::TInputAct::getDoc()
{
    _loger->sendLogMsg(TDBLoger::INFORMATION_CODE, "Launching Input Act detection");

    auto smenaInfo = getCurrentSmena();

    QSqlQuery query(_topazDB);
    _topazDB.transaction();

    QString queryText =
        QString("SELECT \"NDoc\", \"InBillHID\", \"DateDoc\" "
                "FROM \"trInBillH\" "
                " WHERE \"InBillHID\" > %1")
                .arg(_cnf->topaz_LastInputActID());

    writeDebugLogFile(QString("QUERY TO %1>").arg(_topazDB.connectionName()), query.lastQuery());

    if (!query.exec(queryText)) {
        errorDBQuery(_topazDB, query);
    }

    TDocsInfo res;
    quint64 lastId = _cnf->topaz_LastInputActID();
    bool isFound = false;
    while (query.next())
    {
        TDoc::DocInfo docInfo;
        docInfo.number = query.value("NDoc").toInt();
        docInfo.dateTime =  query.value("DateDoc").toDateTime();
        if (smenaInfo.isOpen)
        {
            docInfo.smena = smenaInfo.smenaNumber;
            docInfo.creater= smenaInfo.operatorName;
        }
        docInfo.type = DOC_NAME;
        docInfo.XMLText = makeXML(docInfo);

        _loger->sendLogMsg(TDBLoger::INFORMATION_CODE,
                           QString("-->Detect new Input Act. Number: %1, Date: %2")
                                .arg(docInfo.number)
                                .arg(docInfo.dateTime.toString("yyyy-MM-dd")));

        res.push_back(docInfo);

        if (_cnf->topaz_InputActDeleteFile())
        {
            addFileForDelete(docInfo.number);
        }

        lastId = std::max(query.value("InBillHID").toULongLong(), lastId);
        isFound = true;
    }

    if (isFound)
    {
        _cnf->set_topaz_LastInputActID(lastId);
        _cnf->set_topaz_InputActFilesList(QStringList(_filesForDelete.keys()));
    }

    return res;
}

void TInputAct::deleteFiles()
{
    bool isChange = false;
    for (auto fileForDelete_it = _filesForDelete.begin(); fileForDelete_it != _filesForDelete.end(); )
    {
        //если прошло меньше 1 минута - пропускаем файл
        if (fileForDelete_it.value().secsTo(QDateTime::currentDateTime()) < (60))
        {
            ++fileForDelete_it;

            continue;
        }

        if (!QFileInfo::exists(fileForDelete_it.key()))
        {
            _loger->sendLogMsg(TDBLoger::INFORMATION_CODE, QString("File %1 already deleted").arg(fileForDelete_it.key()));
        }
        else
        {
            const QString fileName(fileForDelete_it.key());
            QFile file(fileName);
            if (file.remove())
            {
                _loger->sendLogMsg(TDBLoger::OK_CODE, QString("File deleted. File name: %1").arg(fileName));
            }
            else
            {
                _loger->sendLogMsg(TDBLoger::INFORMATION_CODE,
                    QString("Cannot deleted file. Skip. File name: %1. Error: %2")
                        .arg(fileName).arg(file.errorString()));
            }

        }

        fileForDelete_it = _filesForDelete.erase(fileForDelete_it);

        isChange = true;
    }

    if (isChange)
    {
        _cnf->set_topaz_InputActFilesList(QStringList(_filesForDelete.keys()));
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
        if (fileName.indexOf(QString("00%1.xml").arg(number)) > 0)
        {
            _filesForDelete.insert(fileName, QDateTime::currentDateTime());
            _loger->sendLogMsg(TDBLoger::INFORMATION_CODE, QString("-->Find file Input Act with number: %1. File name: %2")
                .arg(number).arg(fileName));

            return;
        }
    }

    _loger->sendLogMsg(TDBLoger::OK_CODE, QString("-->Cannot find file for Input Act with number: %1. Skip.").arg(number));
}

QString TInputAct::makeXML(const DocInfo &docInfo)
{
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
    XMLWriter.writeTextElement("DetectDateTime", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"));
    XMLWriter.writeEndElement(); //Root
    XMLWriter.writeEndDocument();

    return XMLStr;
}
