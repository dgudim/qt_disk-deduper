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

bool string_starts_with(const std::string& string, const std::string& prefix);

bool stringStartsWithAny(const std::string& str, std::vector<std::string>& list);

unsigned long getDiskReadSizeB();

QString bytesToReadable(unsigned long kb);

#pragma endregion}

QString millisecondsToReadable(unsigned long ms);

QString timeSinceTimestamp(unsigned long ms);

#endif // GUTILS_H
