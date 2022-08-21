#ifndef GUTILS_H
#define GUTILS_H

#include <chrono>

#include <QApplication>

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

enum ExtenstionFilterState {
    DISABLED,
    ENABLED_BLACK,
    ENABLED_WHITE
};

void walkDir(const QString& dir, const QStringList& blacklisted_dirs, const QStringList& extensions,
             ExtenstionFilterState extFilterState, std::function<void(QString)> callback);

QString getFileHash(const QString& full_path);

bool stringStartsWith(const std::string& string, const std::string& prefix);

bool stringStartsWithAny(const std::string& str, std::vector<std::string>& list);

quint64 getDiskReadSizeB();

quint64 getMemUsedKb();

QString bytesToReadable(quint64 kb);

quint64 readableToBytes(QString str);

#pragma endregion}

QString millisecondsToReadable(quint64 ms);

QString timeSinceTimestamp(quint64 ms);

QT_BEGIN_NAMESPACE

namespace DbUtils{
    bool execQuery(QSqlQuery query);
    bool execQuery(QSqlDatabase db, QString query_str);
    QSqlDatabase openDbConnection();
};

QT_END_NAMESPACE

#endif // GUTILS_H
