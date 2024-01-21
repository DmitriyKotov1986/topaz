#include "queryqueue.h"

using namespace Topaz;

static QueryQueue* ptrQueryQueue = nullptr;

QueryQueue* QueryQueue::createQueryQueue()
{

    if (ptrQueryQueue == nullptr)
    {
        ptrQueryQueue = new QueryQueue();
    }

    return ptrQueryQueue;
};

void QueryQueue::deleteConfig()
{
    delete ptrQueryQueue;

    ptrQueryQueue = nullptr;
}

qsizetype  QueryQueue::enqueue(const QueryData &data)
{
    //декодируем тело файла
    QString decodeData;
    auto result = QByteArray::fromBase64Encoding(data.queryText.toUtf8());

    if (result.decodingStatus == QByteArray::Base64DecodingStatus::Ok)
    {
        decodeData = QString::fromUtf8(result.decoded); // Декодируем файл
    }

    _queryQueue.enqueue({data.queryId, data.id, decodeData});

    return _queryQueue.size();
}
