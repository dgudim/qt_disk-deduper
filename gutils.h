#ifndef GUTILS_H
#define GUTILS_H

#include "datatypes.h"

#include <chrono>

#include <QApplication>

#include <QDialog>
#include <QDialogButtonBox>

#include <QString>
#include <QStringList>
#include <QDate>
#include <QFile>
#include <QDirIterator>
#include <QtDebug>

#include <QThread>

#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>

#include <functional>
#include <fstream>
#include <sstream>
#include <string>

#include <QCryptographicHash>

#pragma region File utils {

QT_BEGIN_NAMESPACE

namespace FileUtils {

    enum ExtenstionFilterState {
        DISABLED,
        ENABLED_BLACK,
        ENABLED_WHITE
    };

    void walkDir(const QString& dir, const QStringList& blacklisted_dirs, const QStringList& extensions,
                 ExtenstionFilterState extFilterState, std::function<void(const QString&)> callback);

    bool deleteOrRenameFiles(QVector<File>& files_to_delete,
                             const std::function<void (const QString &)> &status_callback,
                             bool rename = false, const QString &target_dir = "", const QString &postfix = "");

    QString getFileHash(const QString& full_path);
    quint64 getDiskReadSizeB();

    quint64 getMemUsedKb();

    QString bytesToReadable(quint64 kb);
    quint64 readableToBytes(const QString &str);

    QString getFileGroupFingerprint(const QVector<File>& group);

    QPixmap generateThumbnail(const File& file, int size);

};

QT_END_NAMESPACE


QT_BEGIN_NAMESPACE

namespace StringUtils {
    QString getStringHash(const QString& string);

    bool stringStartsWith(const std::string& string, const std::string& prefix);
    bool stringStartsWithAny(const std::string& str, std::vector<std::string>& list);
};

QT_END_NAMESPACE



QT_BEGIN_NAMESPACE

namespace TimeUtils{
    QString millisecondsToReadable(quint64 ms);
    QString timeSinceTimestamp(quint64 ms);
};

QT_END_NAMESPACE



QT_BEGIN_NAMESPACE

namespace DbUtils{
    bool execQuery(QSqlQuery query);
    bool execQuery(QSqlDatabase db, const QString &query_str);
    QSqlDatabase openDbConnection();
};

QT_END_NAMESPACE


QT_BEGIN_NAMESPACE

namespace UiUtils{
    void connectDialogButtonBox(QDialog* self, QDialogButtonBox* buttonBox);
};

QT_END_NAMESPACE

#endif // GUTILS_H
