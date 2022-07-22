#include "common.h"

//Qt
#include <QString>
#include <QFile>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>

using namespace Common;

void Common::writeLogFile(const QString& prefix, const QString& msg) {
    const QString logFileName = QCoreApplication::applicationDirPath() + "/log/LevelGauge.log"; //имя файла лога

    QDir logDir(QFileInfo(logFileName).absolutePath());
    if (!logDir.exists()) {
        if (!logDir.mkdir(logDir.absolutePath())) {
            qCritical() << QString("%1(!)Cannot make log dir: %2").arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                           .arg(logDir.absolutePath());
            return;
        }
    }

    QFile file(logFileName);
    if (file.open(QFile::WriteOnly | QFile::Append | QFile::Text)) {
        QByteArray tmp = QString("%1 %2\n%3\n\n").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")).arg(prefix).arg(msg).toUtf8();
        file.write(tmp);
        file.close();
    }
    else {
        qCritical() << QString("%1(!) Message not save to log file: %2 Error: %3 Message: %4 %5").arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                       .arg(logFileName).arg(file.errorString()).arg(prefix).arg(msg);
    }
}

void Common::writeDebugLogFile(const QString& prefix, const QString& msg) {
    #ifdef QT_DEBUG
        writeLogFile(prefix, msg);
    #endif
}

