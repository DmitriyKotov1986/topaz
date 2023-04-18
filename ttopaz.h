#ifndef TTOPAZ_H
#define TTOPAZ_H
#ifndef TLEVELGAUGEMONITORING_H
#define TLEVELGAUGEMONITORING_H

//Qt
#include <QObject>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QTimer>
#include <QStringList>

//My
#include "Common/thttpquery.h"
#include "Common/tdbloger.h"
#include "tconfig.h"
#include "tgetdocs.h"

namespace Topaz {

class TTopaz final : public QObject
{
    Q_OBJECT

public:
    explicit TTopaz(QObject *parent = nullptr);
    ~TTopaz();

public slots:
    void start();    //Событике запуска. Приходит сразу после запуска loop events

signals:
    void sendHTTP(const QByteArray& data); //отправка сообщения по HTTP
    void finished(); //испускаем перед завершением работы

private slots:
    void sendToHTTPServer();
    void getAnswerHTTP(const QByteArray &answer); //получен ответ от сервеера
    void errorOccurredHTTP(const QString& msg); //ошибка передачи данных на сервер

private:
    void resetSending();

private:
    TConfig* _cnf = nullptr; //настройки
    Common::THTTPQuery* _HTTPQuery = nullptr;
    Common::TDBLoger* _loger = nullptr;

    QSqlDatabase _db;        //Промежуточная БД

    QTimer* _sendHTTPTimer = nullptr; //таймер отправки HTTP запросов
    QStringList _sendingDocsID; //список последних отправленных ID
    bool _sending = false;            //флаг что в текущий момент идет пересылка данных.

    TGetDocs* _getDocs = nullptr;
};

} //namespace LevelGauge

#endif // TLEVELGAUGEMONITORING_H

#endif // TTOPAZ_H
