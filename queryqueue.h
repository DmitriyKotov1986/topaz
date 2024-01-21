#ifndef QUERYQUEUE_H
#define QUERYQUEUE_H

#include <QString>
#include <QQueue>

namespace Topaz
{

class QueryQueue
{
public:
    struct QueryData
    {
        quint64 queryId = 0;  //уникальный номер запроса
        quint64 id = 0;       //номер запроса в БД сервера
        QString queryText;    //текст запроса
    };

public:
    static QueryQueue* createQueryQueue();
    static void deleteConfig();

    qsizetype enqueue(const QueryData& data);
    QueryData dequeue() {return _queryQueue.dequeue(); };

    bool isEmpty() const { return _queryQueue.isEmpty(); };

private:
    QueryQueue() = default;
    ~QueryQueue() {};

private:
    QQueue<QueryData> _queryQueue;

};

} //namespace Topaz

#endif // QUERYQUEUE_H
