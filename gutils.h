#ifndef GUTILS_H
#define GUTILS_H

#include <chrono>

#include <QString>
#include <QStringList>
#include <QDate>
#include <QFile>
#include <QDirIterator>
#include <QtDebug>

#include <functional>
#include <fstream>
#include <sstream>
#include <string>

#include <QCryptographicHash>


#pragma region File utils {

struct File {
    QString full_path;
    QString name;
    QString hash = "";

    bool operator==(const File &other) const{
        return full_path == other.full_path;
    }
};

void walkDir(const QString& dir, std::function<void(QString)> callback);

QString getFileHash(const QString& full_path);

bool stringStartsWith(const std::string& string, const std::string& prefix);

bool stringStartsWithAny(const std::string& str, std::vector<std::string>& list);

quint64 getDiskReadSizeB();

quint64 getMemUsedKb();

QString bytesToReadable(quint64 kb);

#pragma endregion}

QString millisecondsToReadable(quint64 ms);

QString timeSinceTimestamp(quint64 ms);

#endif // GUTILS_H
