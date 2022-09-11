#include "ttopaz.h"

//Qt
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QXmlStreamWriter>
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QThread>

//My
#include "Common/common.h"

using namespace Topaz;

using namespace Common;

TTopaz::TTopaz(QObject* parent /* = nullptr*/)
    : QObject(parent)
    , _cnf(Topaz::TConfig::config())
{
    Q_CHECK_PTR( _cnf != nullptr);

    //настраиваем подключение к БД
    _db = QSqlDatabase::addDatabase(_cnf->db_Driver(), "MainDB");
    _db.setDatabaseName(_cnf->db_DBName());
    _db.setUserName(_cnf->db_UserName());
    _db.setPassword(_cnf->db_Password());
    _db.setConnectOptions(_cnf->db_ConnectOptions());
    _db.setPort(_cnf->db_Port());
    _db.setHostName(_cnf->db_Host());

    //настраиваем подключение БД логирования
    _logdb = QSqlDatabase::addDatabase(_cnf->db_Driver(), "LoglogDB");
    _logdb.setDatabaseName(_cnf->db_DBName());
    _logdb.setUserName(_cnf->db_UserName());
    _logdb.setPassword(_cnf->db_Password());
    _logdb.setConnectOptions(_cnf->db_ConnectOptions());
    _logdb.setPort(_cnf->db_Port());
    _logdb.setHostName(_cnf->db_Host());

    _loger = Common::TDBLoger::DBLoger(&_logdb, _cnf->sys_DebugMode());

    //создаем поток обработки HTTP Запросов
    QString url = QString("http://%1:%2/CGI/TOPAZ&%3&%4").arg(_cnf->srv_Host()).arg(_cnf->srv_Port()).
                    arg(_cnf->srv_UserName()).arg(_cnf->srv_Password());
    _HTTPQuery = THTTPQuery::HTTPQuery(url, nullptr);

    QThread* HTTPQueryThread = new QThread(this);
    _HTTPQuery->moveToThread(HTTPQueryThread);

    QObject::connect(this, SIGNAL(sendHTTP(const QByteArray&)), _HTTPQuery, SLOT(send(const QByteArray&)));
    QObject::connect(_HTTPQuery, SIGNAL(getAnswer(const QByteArray&)), SLOT(getAnswerHTTP(const QByteArray&)));
    QObject::connect(_HTTPQuery, SIGNAL(errorOccurred(const QString&)), SLOT(errorOccurredHTTP(const QString&)));

    QObject::connect(this, SIGNAL(finished()), HTTPQueryThread, SLOT(quit())); //сигнал на завершение
    QObject::connect(HTTPQueryThread, SIGNAL(finished()), HTTPQueryThread, SLOT(deleteLater())); //уничтожиться после остановки

    HTTPQueryThread->start();

    //таймер отправки HTTP запроса
    _sendHTTPTimer = new QTimer();
    _sendHTTPTimer->setInterval(_cnf->sys_Interval());

    QObject::connect(_sendHTTPTimer, SIGNAL(timeout()), SLOT(sendToHTTPServer()));

    _clearImportDocTimer = new QTimer();
    _clearImportDocTimer->setSingleShot(true);
    _clearImportDocTimer->setInterval(15 * 60000);

    QObject::connect(_clearImportDocTimer, SIGNAL(timeout()), SLOT(clearImportDoc_timeout()));

    //настраиваем подключение к БД Топаза
    _topazDB = QSqlDatabase::addDatabase(_cnf->topazdb_Driver(), "TopazDB");
    _topazDB.setDatabaseName(_cnf->topazdb_DBName());
    _topazDB.setUserName(_cnf->topazdb_UserName());
    _topazDB.setPassword(_cnf->topazdb_Password());
    _topazDB.setConnectOptions(_cnf->topazdb_ConnectOptions());
    _topazDB.setPort(_cnf->topazdb_Port());
    _topazDB.setHostName(_cnf->topazdb_Host());

    //таймер опроса БД Топаз-АЗС
    _workTimer = new QTimer();
    _workTimer->setInterval(_cnf->sys_Interval());

    QObject::connect(_workTimer, SIGNAL(timeout()), SLOT(work()));
}

TTopaz::~TTopaz()
{
    Q_CHECK_PTR(_sendHTTPTimer);
    Q_CHECK_PTR(_workTimer);
    Q_CHECK_PTR(_HTTPQuery);
    Q_CHECK_PTR(_clearImportDocTimer);

    _loger->sendLogMsg(TDBLoger::MSG_CODE::OK_CODE, "Successfully finished");

    if (_sendHTTPTimer != nullptr)
    {
        _sendHTTPTimer->stop();
        _sendHTTPTimer->deleteLater();
    }

    if (_workTimer != nullptr)
    {
        _workTimer->stop();
        _workTimer->deleteLater();
    }

    if (_clearImportDocTimer != nullptr)
    {
        _clearImportDocTimer->stop();
        _clearImportDocTimer->deleteLater();
    }

    if (_HTTPQuery != nullptr)
    {
        //ничего не делаем. остановится  по сигналу finished
    }

    if (_db.isOpen())
    {
        _db.close();
    }

    if (_topazDB.isOpen())
    {
        _topazDB.close();
    }
}

void TTopaz::start()
{
    Q_CHECK_PTR(_HTTPQuery);
    Q_CHECK_PTR(_sendHTTPTimer);
    Q_CHECK_PTR(_workTimer);
    Q_CHECK_PTR(_loger);
    Q_ASSERT(_getDocs == nullptr);

    //проверяем подключение к Логеру
    if (_loger->isError())
    {
        QString msg = QString("Error starting loger: %1").arg(_loger->errorString());
        qCritical() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
        saveLogToFile(msg);

        emit finished();

        return;
    }

    //подключаемся к БД Системы Мониторинга
    if (!_db.open())
    {
        QString msg = QString("Cannot connect to database. Error: %1").arg(_db.lastError().text());
        _loger->sendLogMsg(TDBLoger::CRITICAL_CODE, msg);

        emit finished();

        return;
    };

    //подключаемся к БД топаза
    if (!_topazDB.open())
    {
        QString msg = QString("Cannot connect to Topaz-AZS database. Error: %1").arg(_topazDB.lastError().text());
        _loger->sendLogMsg(TDBLoger::CRITICAL_CODE, msg);

        emit finished();

        return;
    };

    //запускаем поиск новых документов
    if (_getDocs == nullptr)
    {
        _getDocs = new TGetDocs(_topazDB);
    }

    //запукаем таймер отправки HTTP запроов
    if (_sendHTTPTimer != nullptr)
    {
        _sendHTTPTimer->start();
    }

    //запускаем таймер опроса БД Топаз-АЗС
    if (_workTimer != nullptr)
    {
        _workTimer->start();
    }

    _loger->sendLogMsg(TDBLoger::MSG_CODE::OK_CODE, "Successfully started");

    work(); //первый раз запускаем обработку события рабочер таймера сручную, чтобы не ждать минуту
}

void TTopaz::getDocs()
{
    TGetDocs::TDocsInfoList docsInfoList = _getDocs->getDocs();
    if (_getDocs->isError())
    {
        _loger->sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, QString("Cannot get new documments from Topaz-AZS database. Error: %1").arg(_getDocs->errorString()));
        return;
    }

    if (docsInfoList.size() == 0)
    {
        _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("Not new documents"));
        return;
    }

    //сохраняем документы к БД Системного монитора
    _db.transaction();
    for (const auto& docInfoItem: docsInfoList)
    {
        QSqlQuery query(_db);
        QString queryText =
                QString("INSERT INTO TOPAZDOCS (DATE_TIME, DOC_TYPE, DOC_NUMBER, SMENA, CREATER, BODY) "
                        "VALUES ('%1', '%2', %3, %4, '%5', ? )").arg(docInfoItem.dateTime.toString("yyyy-MM-dd hh:mm:ss.zzz"))
                        .arg(docInfoItem.type).arg(docInfoItem.number).arg(docInfoItem.smena).arg(docInfoItem.creater);



        query.prepare(queryText);
        query.bindValue(0, docInfoItem.XMLText.toUtf8());

        Common::writeDebugLogFile("QUERY>", query.lastQuery());

        if (!query.exec())
        {
            errorDBQuery(_db, query);
            break;
        }
    }

    DBCommit(_db);
}

bool TTopaz::updateCurrentSmenaNumber()
{
    QSqlQuery query(_topazDB);
    query.setForwardOnly(true);
    _topazDB.transaction();

    QString queryText =
            "SELECT FIRST 1 MAX(\"SessionNum\") AS \"SessionNum\", \"UserName\", \"StartDateTime\" "
            "FROM \"sysSessions\" "
            "WHERE \"Deleted\" = 0 "
            "GROUP BY \"UserName\", \"StartDateTime\" "
            "ORDER BY \"SessionNum\" DESC ";

    writeDebugLogFile(QString("QUERY TO %1>").arg(_topazDB.connectionName()), queryText);

    if (!query.exec(queryText)) {
        errorDBQuery(_topazDB, query);
        return false;
    }

    bool res = false;
    if (query.first()) {
        if (_currentSmenaNumber != query.value("SessionNum").toInt())
        {
            if (_currentSmenaNumber == -1)
            {
                _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("Current smena number is %1. Start: %2. User: %3")
                    .arg(query.value("SessionNum").toInt()).arg(query.value("StartDateTime").toDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
                    .arg(query.value("UserName").toString()));
            }
            else
            {
                _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("Find start of new smena. Number is %1. Start: %2. User: %3")
                    .arg(query.value("SessionNum").toInt()).arg(query.value("StartDateTime").toDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
                    .arg(query.value("UserName").toString()));
                clearImportDoc();
            }

            _currentSmenaNumber = query.value("SessionNum").toInt();
        }

        res = true;
    }

    query.finish();
    Common::DBCommit(_topazDB);

    return res;
}

void TTopaz::resetSending()
{
    _sending = false;
    _sendingDocsID.clear();
}

void TTopaz::clearImportDoc()
{
    //получаем список файлов в ImportDoc
    QDir topazImportDir(_cnf->topaz_DirName() + "/ImportDoc");
    topazImportDir.setFilter(QDir::NoDotAndDotDot | QDir::Files | QDir::NoSymLinks | QDir::Hidden);
    auto fileList = topazImportDir.entryInfoList();
    //ищем файлы
    bool find = false;
    for (const auto& fileInfoItem: fileList)
    {
        QString fileName = fileInfoItem.absoluteFilePath();
        _filesForDelete.push_back(fileName);
        _loger->sendLogMsg(TDBLoger::INFORMATION_CODE, QString("Find file after new smena start. File name: %1").arg(fileName));
        find = true;
    }
    if (!find)
    {
        _loger->sendLogMsg(TDBLoger::INFORMATION_CODE, QString("Cannot find file for document after new smena start."));
    }

    _clearImportDocTimer->start();
}

void TTopaz::work()
{
    //обновляем данные о текущей смене
    updateCurrentSmenaNumber();
    //получаем новые документы из Топаз-АЗС
    getDocs();
}

void TTopaz::clearImportDoc_timeout()
{
    for (auto fileForDeleteItem = _filesForDelete.begin(); fileForDeleteItem != _filesForDelete.end(); ++fileForDeleteItem)
    {
        if (!QFileInfo::exists(*fileForDeleteItem))
        {
            _loger->sendLogMsg(TDBLoger::INFORMATION_CODE, QString("File %1 already deleted after new smena started.").arg(*fileForDeleteItem));
            continue;
        }
        else
        {
            QDir topazImportDir;
            if (topazImportDir.remove(*fileForDeleteItem))
            {
                _loger->sendLogMsg(TDBLoger::OK_CODE, QString("File deleted after new smena started. File name: %1").arg(*fileForDeleteItem));
            }
            else
            {
                _loger->sendLogMsg(TDBLoger::INFORMATION_CODE, QString("Cannot deleted file after new smena started. Skip. File name: %1").arg(*fileForDeleteItem));
            }
        }
    }
    _filesForDelete.clear();
}

void TTopaz::sendToHTTPServer()
{
    //если ответ на предыдущий запрос еще не получен - то пропускаем этот такт таймера
    if (_sending)
    {
        return;
    }

    QSqlQuery query(_db);
    _db.transaction();

    QString queryText = "SELECT FIRST " + QString::number(_cnf->srv_MaxRecord()) + " ID, DATE_TIME, DOC_TYPE, DOC_NUMBER, SMENA, CREATER, BODY "
                        "FROM TOPAZDOCS "
                        "ORDER BY ID ";

    writeDebugLogFile(QString("QUERY TO %1>").arg(_db.connectionName()), queryText);

    if (!query.exec(queryText))
    {
        errorDBQuery(_db, query);
    }

    //форматируем XML докумен
    QString XMLStr;
    QXmlStreamWriter XMLWriter(&XMLStr);
    XMLWriter.setAutoFormatting(true);
    XMLWriter.writeStartDocument("1.0");
    XMLWriter.writeStartElement("Root");
    XMLWriter.writeTextElement("AZSCode", _cnf->srv_UserName());
    XMLWriter.writeTextElement("ClientVersion", QCoreApplication::applicationVersion());

    while (query.next())
    {
        XMLWriter.writeStartElement("Document");
        XMLWriter.writeTextElement("DateTime", query.value("DATE_TIME").toDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"));
        XMLWriter.writeTextElement("DocumentType", query.value("DOC_TYPE").toString());
        XMLWriter.writeTextElement("DocumentNumber", query.value("DOC_Number").toString());
        XMLWriter.writeTextElement("Smena", query.value("SMENA").toString());
        XMLWriter.writeTextElement("Creater", query.value("CREATER").toString());
        XMLWriter.writeTextElement("Body", query.value("BODY").toString());
        XMLWriter.writeEndElement();

        _sendingDocsID.push_back(query.value("ID").toString());
    }

    XMLWriter.writeEndElement(); //root
    XMLWriter.writeEndDocument();

    query.finish();
    DBCommit(_db);

    //отправляем запрос
    _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("Sending a request to HTTP server. Size: %1 byte").arg(XMLStr.toUtf8().size()));
    _sending = true; //устанавливаем флаг

    emit sendHTTP(XMLStr.toUtf8());

    //далее ждем прихода сигнала getAnswerHTTP - если сервер ответил успешно или errorOccurred если произошла ошибка
}

void TTopaz::errorOccurredHTTP(const QString& msg)
{
    resetSending();

    _loger->sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, "Error sending data to HTTP server. Msg: " + msg);
}

void TTopaz::getAnswerHTTP(const QByteArray &answer)
{
    if (answer.left(2) == "OK")
    {
        if (!_sendingDocsID.isEmpty())
        {
            QString queryText = "DELETE FROM TOPAZDOCS "
                                "WHERE ID IN (" + _sendingDocsID.join(",") + ")";

            DBQueryExecute(_db, queryText);
        }

        bool neetSending = (_sendingDocsID.size() == _cnf->srv_MaxRecord());

        _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, "Data has been successfully sent to the server.");

        resetSending();

        if (neetSending)
        {
            sendToHTTPServer(); //если есть еще данные для отправки - повторяем отправку данных
        }
    }
    else
    {
        _loger->sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, "Failed to send data to the server. Server answer: " + answer);

        resetSending();
    }
}


