#include "ttopaz.h"

//Qt
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QXmlStreamWriter>
#include <QCoreApplication>
#include <QFile>
#include <QThread>
//My
#include "common.h"

using namespace Topaz;

using namespace Common;

TTopaz::TTopaz(TConfig* cnf, QObject* parent /* = nullptr*/)
    : QObject(parent)
    , _cnf(cnf)
{
    Q_ASSERT( _cnf != nullptr);

    //настраиваем подключениек БД
    _db = QSqlDatabase::addDatabase(_cnf->db_Driver(), "MainDB");
    _db.setDatabaseName(_cnf->db_DBName());
    _db.setUserName(_cnf->db_UserName());
    _db.setPassword(_cnf->db_Password());
    _db.setConnectOptions(_cnf->db_ConnectOptions());
    _db.setPort(_cnf->db_Port());
    _db.setHostName(_cnf->db_Host());

    //создаем поток обработки HTTP Запросов
    QString url = QString("http://%1:%2/CGI/LEVELGAUGE&%3&%4").arg(_cnf->srv_Host()).arg(_cnf->srv_Port()).
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
    Q_ASSERT(_sendHTTPTimer != nullptr);
    Q_ASSERT(_HTTPQuery != nullptr);

    sendLogMsg(MSG_CODE::CODE_OK, "Successfully finished");


    if (_sendHTTPTimer != nullptr) {
        _sendHTTPTimer->stop();
        _sendHTTPTimer->deleteLater();
    }


    if (_HTTPQuery != nullptr) {
        //ничего не делаем. остановится  по сигналу finished
    }

    if (_db.isOpen()) {
        _db.close();
    }
}

void TTopaz::start()
{
    Q_ASSERT(_HTTPQuery != nullptr);
    Q_ASSERT(_sendHTTPTimer != nullptr);

    //подключаемся к БД
    if (!_db.open()) {
        QString msg = QString("Cannot connect to database. Error: %1").arg(_db.lastError().text());
        qCritical() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
        saveLogToFile(msg);

        emit finished();

        return;
    };

    //запукаем таймер отправки HTTP запроов
    _sendHTTPTimer->start();

    sendLogMsg(MSG_CODE::CODE_OK, "Successfully started");
}

void TTopaz::sendLogMsg(uint16_t category, const QString& msg)
{
    if (_cnf->sys_DebugMode()) {
        qDebug() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
    }

    Common::writeDebugLogFile("LOG>", msg);

    QString queryText = "INSERT INTO LOG (CATEGORY, SENDER, MSG) VALUES ( "
                        + QString::number(category) + ", "
                        "\'LevelGauge\', "
                        "\'" + msg +"\'"
                        ")";

    DBQueryExecute(queryText);
}

void TTopaz::saveLogToFile(const QString& msg)
{
    Common::writeLogFile("LOG>", msg);
}

void TTopaz::DBQueryExecute(const QString &queryText)
{
   if (!_db.isOpen()) {
       QString msg = QString("Cannot query to DB execute because connetion is closed. Query: %1").arg(queryText);
       saveLogToFile(msg);
       qCritical() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
       return;
   }

    QSqlQuery query(_db);
    _db.transaction();

    if (!query.exec(queryText)) {
        errorDBQuery(query);
    }

    DBCommit();
}

void TTopaz::DBCommit()
{
  if (!_db.commit()) {
    QString msg = QString("Cannot commit trancsation. Error: %1").arg(_db.lastError().text());
    qCritical() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
    saveLogToFile(msg);

    _db.rollback();

    emit finished();
  }
}

void TTopaz::errorDBQuery(const QSqlQuery& query)
{
    QString msg = QString("Cannot execute query. Error: %1").arg(query.lastError().text());
    qCritical() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
    saveLogToFile(msg);

    _db.rollback();

    emit finished();
}

void TTopaz::sendToHTTPServer()
{
    //если ответ на предыдущий запрос еще не получен - то пропускаем этот такт таймера
    if (_sending) {
        return;
    }

    QSqlQuery query(_db);
    _db.transaction();

    QString queryText = "SELECT FIRST " + QString::number(_cnf->srv_MaxRecord()) + " " +
                       "ID, DATE_TIME, TANK_NUMBER, ENABLED, DIAMETR, VOLUME, TILT, TCCOEF, OFFSET, PRODUCT "
                       "FROM TANKSCONFIG "
                       "WHERE I";


    if (!query.exec(queryText)) {
       errorDBQuery(query);
    }

    //форматируем XML докумен
    QString XMLStr;
    QXmlStreamWriter XMLWriter(&XMLStr);
    XMLWriter.setAutoFormatting(true);
    XMLWriter.writeStartDocument("1.0");
    XMLWriter.writeStartElement("Root");
    XMLWriter.writeTextElement("AZSCode", _cnf->srv_UserName());
    XMLWriter.writeTextElement("ClientVersion", QCoreApplication::applicationVersion());

    while (query.next()) {
       // qDebug() << Query.value("TANK_NUMBER").toString();
        XMLWriter.writeStartElement("LevelGaugeConfig");
        XMLWriter.writeTextElement("DateTime", query.value("DATE_TIME").toDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"));
        XMLWriter.writeTextElement("TankNumber", query.value("TANK_NUMBER").toString());
        XMLWriter.writeTextElement("Enabled", query.value("ENABLED").toString());
        XMLWriter.writeTextElement("Diametr", query.value("DIAMETR").toString());
        XMLWriter.writeTextElement("Volume", query.value("VOLUME").toString());
        XMLWriter.writeTextElement("Tilt", query.value("TILT").toString());
        XMLWriter.writeTextElement("TCCoef", query.value("TCCOEF").toString());
        XMLWriter.writeTextElement("Offset", query.value("OFFSET").toString());
        XMLWriter.writeTextElement("Product", query.value("PRODUCT").toString());
        XMLWriter.writeEndElement();


    }

    queryText = "SELECT FIRST " + QString::number(_cnf->srv_MaxRecord()) + " " +
                "ID, TANK_NUMBER, DATE_TIME, VOLUME, MASS, DENSITY, TCCORRECT, HEIGHT, WATER, TEMP "
                "FROM TANKSMEASUMENTS "
                "WHERE ID >"
                "ORDER BY ID";

    if (!query.exec(queryText)) {
        errorDBQuery(query);
    }

    while (query.next()) {
        XMLWriter.writeStartElement("LevelGaugeMeasument");
        XMLWriter.writeTextElement("DateTime", query.value("DATE_TIME").toDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"));
        XMLWriter.writeTextElement("TankNumber", query.value("TANK_NUMBER").toString());
        XMLWriter.writeTextElement("Volume", query.value("VOLUME").toString());
        XMLWriter.writeTextElement("Mass", query.value("MASS").toString());
        XMLWriter.writeTextElement("Density", query.value("DENSITY").toString());
        XMLWriter.writeTextElement("TCCorrect", query.value("TCCORRECT").toString());
        XMLWriter.writeTextElement("Height", query.value("HEIGHT").toString());
        XMLWriter.writeTextElement("Water", query.value("WATER").toString());
        XMLWriter.writeTextElement("Temp", query.value("TEMP").toString());
        XMLWriter.writeEndElement();


    }

    XMLWriter.writeEndElement(); //root
    XMLWriter.writeEndDocument();

    DBCommit();

    //отправляем запрос
    sendLogMsg(MSG_CODE::CODE_INFORMATION, QString("Sending a request. Size: %1 byte").arg(XMLStr.toUtf8().size()));

    _sending = true; //устанавливаем флаг

    emit sendHTTP(XMLStr.toUtf8());
}

void TTopaz::errorOccurredHTTP(const QString& msg)
{
    _sending = false;
    //очищаем очереди

    sendLogMsg(MSG_CODE::CODE_ERROR, "Error sending data to HTTP server. Msg: " + msg);
}

void TTopaz::getAnswerHTTP(const QByteArray &answer)
{
    _sending = false;

    if (answer.left(2) == "OK") {

   //     bool neetSending = (_sendingTanksMasumentsID.size() == _cnf->srv_MaxRecord()) || (_sendingTanksConfigsID.size() == _cnf->srv_MaxRecord());
        //очищаем очереди
        _cnf->save();

        sendLogMsg(MSG_CODE::CODE_INFORMATION, "Data has been successfully sent to the server.");

  //      if (neetSending) sendToHTTPServer(); //если есть еще данные для отправки - повторяем отправку данных
   }
   else {
       sendLogMsg(MSG_CODE::CODE_ERROR, "Failed to send data to the server. Server answer: " + answer);
   }
}
