//Qt
#include <QCoreApplication>
#include <QTimer>
#include <QCommandLineParser>
#include <windows.h>

//My
#include "Common/common.h"
#include "Common/regcheck.h"
#include "tconfig.h"
#include "ttopaz.h"

using namespace Topaz;

using namespace Common;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    //устаналиваем основные настройки
    QCoreApplication::setApplicationName("Topaz");
    QCoreApplication::setOrganizationName("OOO 'SA'");
    QCoreApplication::setApplicationVersion(QString("Version:0.2a Build: %1 %2").arg(__DATE__).arg(__TIME__));

    setlocale(LC_CTYPE, ""); //настраиваем локаль

    if (checkAlreadyRun())
    {
        qDebug() << QString("WARNING: %1 already running.").arg(QCoreApplication::applicationName());
    }

    //Создаем парсер параметров командной строки
    QCommandLineParser parser;
    parser.setApplicationDescription("Program work with Topaz-AZS.");
    parser.addHelpOption();
    parser.addVersionOption();

    //добавляем опцию Config
    QCommandLineOption Config(QStringList() << "Config", "Config file name", "ConfigFileNameValue", QString("%1.ini").arg(QCoreApplication::applicationName()));
    parser.addOption(Config);

    //парсим опции командной строки
    parser.process(a);

    //читаем конфигурацию из файла
    QString configFileName = parser.value(Config);
    if (!parser.isSet(Config))
    {
        configFileName = a.applicationDirPath() + "/" + parser.value(Config);
    }

    //Читаем конигурацию
    auto cnf = TConfig::config(configFileName);
    if (cnf->isError())
    {
        QString msg = "Error load configuration: " + cnf->errorString();
        qCritical() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
        Common::writeLogFile("ERR>", msg);

        return EXIT_CODE::LOAD_CONFIG_ERR; // -1
    }

#ifndef QT_DEBUG
    //проверяем регистрацию ПО
    auto regCheck = new RegCheck(cnf->srv_UserName());

    if (regCheck->isChecket())
    {
        QString msg = QString("Registration verification passed successfully");
        qDebug() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
    }
    else
    {
        QString msg = QString("Unregistered copy of the program. Registration key: %1. Message: %2")
                .arg(regCheck->id()).arg(regCheck->messageString());

        qCritical() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
        Common::writeLogFile("ERR>", msg);

        TConfig::deleteConfig();
        delete regCheck;

        return EXIT_CODE::UNREGISTER_COPY; // -4
    }

    delete regCheck;
#endif

    //настраиваем таймер
    QTimer startTimer;
    startTimer.setInterval(0);       //таймер сработает так быстро как только это возможно
    startTimer.setSingleShot(true);  //таймер сработает 1 раз

    //создаем основной класс программы
    TTopaz topaz(&a);

    //При запуске выполняем слот Start
    QObject::connect(&startTimer, SIGNAL(timeout()), &topaz, SLOT(start()));
    //Для завершения работы необходимо подать сигнал Finished
    QObject::connect(&topaz, SIGNAL(finished()), &a, SLOT(quit()));

    //запускаем таймер
    startTimer.start();

    //запускаем цикл обработчика событий
    auto res = a.exec();

    TConfig::deleteConfig();

    return res;
}
