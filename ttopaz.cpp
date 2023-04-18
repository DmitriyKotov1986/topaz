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
    Q_CHECK_PTR(_cnf);

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

    if (_sendHTTPTimer != nullptr)
    {
        delete _sendHTTPTimer;
    }

    if (_db.isOpen())
    {
        _db.close();
    }

    if (_getDocs != nullptr)
    {
        delete _getDocs;
    }

    _loger->sendLogMsg(TDBLoger::MSG_CODE::OK_CODE, "Successfully finished");

    if (_loger != nullptr)
    {
        TDBLoger::deleteDBLoger();
    }

    if (_HTTPQuery != nullptr)
    {
        THTTPQuery::deleteTHTTPQuery();
    }
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
        _loger->sendLogMsg(TDBLoger::CRITICAL_CODE, msg);

        emit finished();

        return;
    };


    //создаем классы обслуживающие разлиные документы из БД Топаза
    _getDocs = new TGetDocs(); // в конструкторе этого класса создаются классы отслеживающие различные документы в БД Топаза
    if (_getDocs->isError())
    {
        QString msg = QString("Cannot start to get new documents from Topaz-AZS database. Error: %1").arg(_getDocs->errorString());
        _loger->sendLogMsg(TDBLoger::CRITICAL_CODE, msg);

        emit finished();

        return;
    }

    _sendHTTPTimer->start();

    _loger->sendLogMsg(TDBLoger::MSG_CODE::OK_CODE, "Successfully started");
}



void TTopaz::resetSending()
{
    _sending = false;
    _sendingDocsID.clear();
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
            QString queryText = QString("DELETE FROM TOPAZDOCS "
                                        "WHERE ID IN (%1)")
                                    .arg(_sendingDocsID.join(","));

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


