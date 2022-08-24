#ifndef DATATYPES_H
#define DATATYPES_H

#include <QString>
#include <QVector>
#include <QObject>

#include <QPixmap>

#include <QMap>
#include <QDataStream>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "ExifTool.h"

enum MediaType {
    UNKNOWN,
    IMAGE,
    VIDEO,
    AUDIO
};

struct Countable_qstring {
    QString string;
    quint32 count;
    double count_percentage;
    qint64 total_size_bytes;
    double size_percentage;

    Countable_qstring(const QString& string) {
        this->string = string;
    }

    Countable_qstring (const QString& string, quint32 count, qint64 total_size_bytes, double count_percentage = 0, double size_percentage = 0)
        : string(string), count(count), count_percentage(count_percentage), total_size_bytes(total_size_bytes), size_percentage(size_percentage) {};

    bool operator==(const Countable_qstring &other) const{
        return string == other.string;
    }

    bool operator==(const QString &other) const{
        return string == other;
    }
};

struct File {
    bool valid = true;
    QString path_without_name;
    QString name;
    qint64 size_bytes;
    QString extension;
    QString hash = "";
    QByteArray thumbnail_raw;
    QMap<QString, QString> metadata;

    File (){ valid = false; }

    File (const QString& full_path, const QString& path_without_name, const QString& name, const QString& extension, qint64 size_bytes)
        : path_without_name(path_without_name), name(name), size_bytes(size_bytes), extension(extension), full_path(full_path) {
        metadata.insert("extension", extension);
    }

    void loadMetadata(ExifTool *ex_tool, QSqlDatabase db);
    void loadHash(QSqlDatabase db);
    void loadThumbnail(QSqlDatabase db);

    bool operator==(const File &other) const {
        return full_path == other.full_path;
    }

    bool operator<(const File &other) const {
        return full_path < other.full_path;
    }

    operator QString() const { return full_path; }

private:
     QString full_path;

    bool metadata_loaded = false;

    void saveHashToDb(QSqlDatabase db);
    void saveMetadataToDb(QSqlDatabase db);
    void saveThumbnailToDb(QSqlDatabase db);

    bool loadHashFromDb(QSqlDatabase db);
    bool loadMetadataFromDb(QSqlDatabase db);
    bool loadThumbnailFromDb(QSqlDatabase db);
};

QList<QString> getMetaFieldsList();

struct StatsContainer {

    int total_files = 0;
    quint64 total_size = 0;

    QVector<QPair<QString, QVector<Countable_qstring>>> meta_fields_stats;

    StatsContainer(){};

    StatsContainer (const QVector<QPair<QString, QVector<Countable_qstring>>>& meta_fields_stats, int total_files, quint64 total_size)
        : total_files(total_files), total_size(total_size), meta_fields_stats(meta_fields_stats) {}
};

#endif // DATATYPES_H
