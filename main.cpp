#include <QCoreApplication>
#include <QTimer>
#include <QCommandLineParser>
#include <windows.h>
#include "tconfig.h"
#include "ttopaz.h"
#include "common.h"

using namespace Topaz;

using namespace Common;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    //устаналиваем основные настройки
    QCoreApplication::setApplicationName("Topaz");
    QCoreApplication::setOrganizationName("OOO 'SA'");
    QCoreApplication::setApplicationVersion(QString("Version:0.1a Build: %1 %2").arg(__DATE__).arg(__TIME__));

    setlocale(LC_CTYPE, ""); //настраиваем локаль

    //Создаем парсер параметров командной строки
    QCommandLineParser parser;
    parser.setApplicationDescription("Program work with Topaz-AZS.");
    parser.addHelpOption();
    parser.addVersionOption();

    //добавляем опцию Config
    QCommandLineOption Config(QStringList() << "Config", "Config file name", "ConfigFileNameValue", "Topaz.ini");
    parser.addOption(Config);

    //Парсим опции командной строки
    parser.process(a);

    //читаем конфигурацию из файла
    QString configFileName = parser.value(Config);
    if (!parser.isSet(Config)) {
        configFileName = a.applicationDirPath() +"/" + parser.value(Config);
    }

    //Читаем конигурацию
    TConfig* cnf = TConfig::config(configFileName);
    if (cnf->isError()) {
        qCritical() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg("Error load configuration: " + cnf->errorString());
        exit(EXIT_CODE::LOAD_CONFIG_ERR); // -1
    }

    //настраиваем таймер
    QTimer startTimer;
    startTimer.setInterval(0);       //таймер сработает так быстро как только это возможно
    startTimer.setSingleShot(true);  //таймер сработает 1 раз

    //создаем основной класс программы
    TTopaz topaz(cnf, &a);

    //При запуске выполняем слот Start
    QObject::connect(&startTimer, SIGNAL(timeout()), &topaz, SLOT(start()));
    //Для завершения работы необходимоподать сигнал Finished
    QObject::connect(&topaz, SIGNAL(finished()), &a, SLOT(quit()));

    //запускаем таймер
    startTimer.start();
    //запускаем цикл обработчика событий
    return a.exec();
}
