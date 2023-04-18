#include "tgetdocs.h"

//Qt
#include <QSqlQuery>
#include <QSqlError>

//My
#include "toffact.h"
#include "tinputact.h"
#include "tcoupons.h"
#include "tnewsmena.h"

using namespace Topaz;

Topaz::TGetDocs::TGetDocs()
    : _cnf(TConfig::config())
{
    Q_CHECK_PTR(_cnf);

    //фабрика загружаемых документов
    //добавляем документы которые необходимо выдергивать из Топаза
    if (_cnf->topaz_OffActEnabled())
    {
        auto offAct = new TOffAct();
        if (offAct->isError())
        {
            _errorString = QString("Error in creating Off Act class. Error: %1").arg(offAct->errorString());
            delete offAct;

            return;
        }
        _docs.push_back(offAct);
    }
    if (_cnf->topaz_InputActEnabled())
    {
        auto inputAct = new TInputAct();
        if (inputAct->isError())
        {
            _errorString = QString("Error in creating Input Act class. Error: %1").arg(inputAct->errorString());
            delete inputAct;

            return;
        }
        _docs.push_back(inputAct);
    }
    if (_cnf->topaz_CouponsEnable())
    {
        auto coupons = new TCoupons();
        if (coupons->isError())
        {
            _errorString = QString("Error in creating Coupons class. Error: %1").arg(coupons->errorString());
            delete coupons;

            return;
        }
        _docs.push_back(coupons);
    }
    if (_cnf->topaz_NewSmenaEnabled())
    {
        auto newSmena = new TNewSmena();
        if (newSmena->isError())
        {
            _errorString = QString("Error in creating New Smena class. Error: %1").arg(newSmena->errorString());
            delete newSmena;

            return;
        }
        _docs.push_back(newSmena);
    }
}

Topaz::TGetDocs::~TGetDocs()
{
    for (auto doc_it = _docs.begin(); doc_it != _docs.end(); ++doc_it)
    {
        delete *doc_it;
    }
    _docs.clear();
}

QString Topaz::TGetDocs::errorString()
{
    auto res = _errorString;
    _errorString.clear();

    return res;
}
