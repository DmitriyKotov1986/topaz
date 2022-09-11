#include "tcleanerimportdoc.h"

using namespace Topaz;

TCleanerImportDoc::TCleanerImportDoc(const QString &topazImportDocDirName, int lastSmenaNumber) :
    _currentSmenaNumber(lastSmenaNumber)
{
    Q_ASSERT(!topazImportDocDirName.isEmpty());
    Q_ASSERT(lastSmenaNumber != -1);

    _topazImportDocDir = new QDir(topazImportDocDirName);
    if (!_topazImportDocDir->exists()) {
        _errorString = "Path Topaz-AZS ImportDoc not found: " + _topazImportDocDir->absolutePath();
        return;
    }
    _topazImportDocDir->setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::Writable);
}

TCleanerImportDoc::~TCleanerImportDoc()
{
    Q_ASSERT(_topazImportDocDir != nullptr);

    if (_topazImportDocDir != nullptr) {
        delete _topazImportDocDir;
        _topazImportDocDir = nullptr;
    }
}

QStringList TCleanerImportDoc::clearImportDoc(int currentSmenaNumber)
{
    Q_ASSERT(currentSmenaNumber != -1);
    Q_ASSERT(_topazImportDocDir != nullptr);
    Q_ASSERT(_topazImportDocDir->exists());

    _errorString.clear();

    if ((currentSmenaNumber == -1) || (_currentSmenaNumber == currentSmenaNumber)) {
        return QStringList();
    }

    QStringList res;
    bool isFail = false;
    //добрались до сюда - значит смена поменялась и нужно очищать папку ImportDoc
    QFileInfoList fileInfoList = _topazImportDocDir->entryInfoList();
    for (const auto& fileInfoItem: fileInfoList) {
        if (_topazImportDocDir->remove(fileInfoItem.absoluteFilePath())) {
            res.push_back(fileInfoItem.absoluteFilePath());
        }
        else {
            if (!isFail) {
                _errorString += QString("Cannot delete file: ");
                isFail = true;
            }
            _errorString += fileInfoItem.absoluteFilePath() +"; ";
        }
    }

    _currentSmenaNumber = currentSmenaNumber;

    return res;
}
