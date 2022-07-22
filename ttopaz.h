#ifndef TTOPAZ_H
#define TTOPAZ_H
#ifndef TLEVELGAUGEMONITORING_H
#define TLEVELGAUGEMONITORING_H

//Qt
#include <QObject>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QTimer>
#include <QTime>
#include <QStringList>
//My
#include "thttpquery.h"
#include "tconfig.h"

namespace Topaz {

typedef enum {
    CODE_OK = 0,
    CODE_ERROR = 1,
    CODE_INFORMATION = 2
} MSG_CODE;

class TTopaz final : public QObject
{
    Q_OBJECT

public:
    explicit TTopaz(TConfig* cnf, QObject *parent = nullptr);
    ~TTopaz();

public slots:
    void start();    //Событике запуска. Приходит сразу после запуска loop events

signals:
    void sendHTTP(const QByteArray& data); //отправка сообщения по HTTP
    void finished(); //испускаем перед завершением работы

private:
    void sendLogMsg(const uint16_t category, const QString& msg); //Сохранения логов
    void saveLogToFile(const QString& msg); //сохраняет сообщение в файл исли произошел сбой записи лога в БД

    void DBQueryExecute(const QString& queryText); //ывполняет запрос к DB
    void DBCommit(); //выполняет commit db
    void errorDBQuery(const QSqlQuery& query); //обрабатывает ошибку выполнения запроса к DB

private slots:
    //HTTP
    void sendToHTTPServer();
    void getAnswerHTTP(const QByteArray &answer); //получен ответ от сервеера
    void errorOccurredHTTP(const QString& msg); //ошибка передачи данных на се

private:
    TConfig* _cnf = nullptr; //настройки
    QSqlDatabase _db; //Промежуточная БД
    Common::THTTPQuery* _HTTPQuery = nullptr;

    QTimer* _sendHTTPTimer = nullptr; //таймер отправки HTTP запросов
    bool _sending = false; //флаг что в текущий момент идет перылка данных.
};

} //namespace LevelGauge

#endif // TLEVELGAUGEMONITORING_H

#endif // TTOPAZ_H
