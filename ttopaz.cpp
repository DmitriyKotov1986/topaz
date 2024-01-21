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
    , _queryQueue(QueryQueue::createQueryQueue())
{
    Q_CHECK_PTR(_cnf);
    Q_CHECK_PTR(_queryQueue);

    //настраиваем подключение к БД
    _db = QSqlDatabase::addDatabase(_cnf->db_Driver(), "MainDB");
    _db.setDatabaseName(_cnf->db_DBName());
    _db.setUserName(_cnf->db_UserName());
    _db.setPassword(_cnf->db_Password());
    _db.setConnectOptions(_cnf->db_ConnectOptions());
    _db.setPort(_cnf->db_Port());
    _db.setHostName(_cnf->db_Host());

    //настраиваем подключение БД логирования
    auto logdb = QSqlDatabase::addDatabase(_cnf->db_Driver(), "LoglogDB");
    logdb.setDatabaseName(_cnf->db_DBName());
    logdb.setUserName(_cnf->db_UserName());
    logdb.setPassword(_cnf->db_Password());
    logdb.setConnectOptions(_cnf->db_ConnectOptions());
    logdb.setPort(_cnf->db_Port());
    logdb.setHostName(_cnf->db_Host());

    _loger = Common::TDBLoger::DBLoger(&logdb, _cnf->sys_DebugMode());

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
}

TTopaz::~TTopaz()
{
    Q_CHECK_PTR(_sendHTTPTimer);
    Q_CHECK_PTR(_HTTPQuery);
    Q_CHECK_PTR(_queryQueue);

    delete _sendHTTPTimer;

    if (_db.isOpen())
    {
        _db.close();
    }

    delete _getDocs;

    _loger->sendLogMsg(TDBLoger::MSG_CODE::OK_CODE, "Successfully finished");

    QueryQueue::deleteConfig();
    TDBLoger::deleteDBLoger();
    THTTPQuery::deleteTHTTPQuery();
}

void TTopaz::start()
{
    Q_CHECK_PTR(_HTTPQuery);
    Q_CHECK_PTR(_sendHTTPTimer);
    Q_CHECK_PTR(_loger);

    //подключаемся к БД Системы Мониторинга
    if (!_db.open())
    {
        QString msg = QString("Cannot connect to database. Error: %1").arg(_db.lastError().text());
        _loger->sendLogMsg(TDBLoger::MSG_CODE::CRITICAL_CODE, msg);

        emit finished();

        return;
    };

    //создаем классы обслуживающие разлиные документы из БД Топаза
    _getDocs = new TGetDocs(); // в конструкторе этого класса создаются классы отслеживающие различные документы в БД Топаза
    if (_getDocs->isError())
    {
        QString msg = QString("Cannot start to get new documents from Topaz-AZS database. Error: %1").arg(_getDocs->errorString());
        _loger->sendLogMsg(TDBLoger::MSG_CODE::CRITICAL_CODE, msg);

        emit finished();

        return;
    }

    _lastQueryID = _cnf->srv_lastQueryID();

    _sendHTTPTimer->start();

    _loger->sendLogMsg(TDBLoger::MSG_CODE::OK_CODE, "Successfully started");
}



void TTopaz::resetSending()
{
    _sending = false;
    _sendingDocsID.clear();
}

void TTopaz::addQueryQueue(const QueryQueue::QueryData &data)
{
    Q_CHECK_PTR(_queryQueue);

    auto count = _queryQueue->enqueue(data);
    _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("Query with id %1 added to queue. Total query: %2").arg(data.id).arg(count));
}

void TTopaz::sendToHTTPServer()
{
    //если ответ на предыдущий запрос еще не получен - то пропускаем этот такт таймера
    if (_sending)
    {
        return;
    }

    _db.transaction();
    QSqlQuery query(_db);

    QString queryText = QString("SELECT FIRST %1 ID, DATE_TIME, DOC_TYPE, DOC_NUMBER, SMENA, CREATER, BODY "
                                "FROM TOPAZDOCS "
                                "ORDER BY ID")
                            .arg(_cnf->srv_MaxRecord());

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
    XMLWriter.writeTextElement("ProtocolVersion","0.1");
    XMLWriter.writeTextElement("ClientVersion", QCoreApplication::applicationVersion());
    XMLWriter.writeTextElement("LastQueryID", _firstTime ? "0" : QString::number(_lastQueryID));

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
    QXmlStreamReader XMLReader(answer);

    bool loadStatus = false;

    while ((!XMLReader.atEnd()) && (!XMLReader.hasError()))
    {
        QXmlStreamReader::TokenType Token = XMLReader.readNext();
        if (Token == QXmlStreamReader::StartDocument)
        {
            continue;
        }
        else if (Token == QXmlStreamReader::EndDocument)
        {
            break;
        }
        else if (Token == QXmlStreamReader::StartElement)
        {
            if (XMLReader.name().toString()  == "Root")
            {
                while ((XMLReader.readNext() != QXmlStreamReader::EndElement) && !XMLReader.atEnd() && !XMLReader.hasError())
                {
                    if (XMLReader.name().toString().isEmpty())
                    {
                        continue;
                    }
                    else if (XMLReader.name().toString()  == "ProtocolVersion") //пока версию протокола не проверяем
                    {
                        XMLReader.readElementText();
                    }
                    else if (XMLReader.name().toString()  == "LoadStatus")
                    {
                        loadStatus = (XMLReader.readElementText() == "1");
                    }
                    else if (XMLReader.name().toString()  == "Queries")
                    {
                        while ((XMLReader.readNext() != QXmlStreamReader::EndElement) && !XMLReader.atEnd() && !XMLReader.hasError())
                        {
                            if (XMLReader.name().toString().isEmpty())
                            {
                                continue;
                            }
                            else if (XMLReader.name().toString()  == "Query")
                            {
                                QueryQueue::QueryData data;

                                while ((XMLReader.readNext() != QXmlStreamReader::EndElement) && !XMLReader.atEnd() && !XMLReader.hasError())
                                {
                                    if (XMLReader.name().toString().isEmpty())
                                    {
                                        continue;
                                    }
                                    else if (XMLReader.name().toString()  == "QueryID")
                                    {
                                        bool ok = false;
                                        data.queryId = XMLReader.readElementText().toULongLong(&ok);
                                        if (!ok)
                                        {
                                            _loger->sendLogMsg(TDBLoger::MSG_CODE::WARNING_CODE, QString("Incorrect value tag (Root/Queries/Query/%1). Value: %2. Value must be number.").arg(XMLReader.name().toString()));
                                        }
                                    }
                                    else if (XMLReader.name().toString()  == "ID")
                                    {
                                        bool ok = false;
                                        data.id = XMLReader.readElementText().toULong(&ok);
                                        if (!ok)
                                        {
                                            _loger->sendLogMsg(TDBLoger::MSG_CODE::WARNING_CODE, QString("Incorrect value tag (Root/Queries/Query/%1). Value: %2. Value must be number.").arg(XMLReader.name().toString()));
                                        }
                                    }
                                    else if (XMLReader.name().toString()  == "QueryText")
                                    {
                                        data.queryText = XMLReader.readElementText();
                                        if (data.queryText.isEmpty())
                                        {
                                            _loger->sendLogMsg(TDBLoger::MSG_CODE::WARNING_CODE, QString("Incorrect value tag (Root/Queries/Query/%1). Value: %2. Value must not be emply.").arg(XMLReader.name().toString()));
                                        }
                                    }
                                    else
                                    {
                                        _loger->sendLogMsg(TDBLoger::MSG_CODE::WARNING_CODE, QString("Undefine tag in XML (Root/Queries/Query/%1)").arg(XMLReader.name().toString()));

                                        XMLReader.readElementText();
                                    }
                                } 

                                if (!data.queryText.isEmpty())
                                {
                                    addQueryQueue(data);
                                    if (_lastQueryID < data.id)
                                    {
                                        _lastQueryID = data.id;
                                    }
                                }
                            }
                            else
                            {
                                _loger->sendLogMsg(TDBLoger::MSG_CODE::WARNING_CODE, QString("Undefine tag in XML (Root/Queries/%1)").arg(XMLReader.name().toString()));

                                XMLReader.readElementText();
                            }
                        }
                    }
                    else
                    {
                        _loger->sendLogMsg(TDBLoger::MSG_CODE::WARNING_CODE, QString("Undefine tag in XML (Root/%1)").arg(XMLReader.name().toString()));

                        XMLReader.readElementText();
                    }
                }
            }
            else
            {
                _loger->sendLogMsg(TDBLoger::MSG_CODE::WARNING_CODE, QString("Undefine tag in XML (%1)").arg(XMLReader.name().toString()));

                XMLReader.readElementText();
            }
        }
    }

    if (XMLReader.hasError()) { //неудалось распарсить пришедшую XML
        _loger->sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, QString("Incorrect answer from server. Parser msg: %1 Answer from server: %2").arg(XMLReader.errorString()).arg(answer.left(200)));

        resetSending();

        return;
    }

    if (loadStatus)
    {
        if (!_sendingDocsID.isEmpty())
        {
            QString queryText = QString("DELETE FROM TOPAZDOCS "
                                        "WHERE ID IN (%1)")
                                    .arg(_sendingDocsID.join(","));

            DBQueryExecute(_db, queryText);
        }

        bool neetSending = (_sendingDocsID.size() == _cnf->srv_MaxRecord());

        _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, "Data has been successfully sent to the server.");

        resetSending();

        _firstTime = false;

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


