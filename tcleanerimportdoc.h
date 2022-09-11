#ifndef TCLEANERIMPORTDOC_H
#define TCLEANERIMPORTDOC_H

//QT
#include <QString>
#include <QDir>

namespace Topaz {

class TCleanerImportDoc final
{
public:
    explicit TCleanerImportDoc(const QString& topazImportDocDirName, int lastSmenaNumber);
    ~TCleanerImportDoc();

    QStringList clearImportDoc(int currentSmenaNumber); //Очищает папку ImportDoc если изменился номер текущей смены. Возвращает  false  если при удалении произошла ошибка
    const QString& errorString() const { return _errorString; }
    bool isError() const { return !_errorString.isEmpty(); }

private:
    QDir* _topazImportDocDir = nullptr;
    int _currentSmenaNumber = -1;
    QString _errorString;
}; // class TCleanerImportDoc

} // namespace Topaz

#endif // TCLEANERIMPORTDOC_H
