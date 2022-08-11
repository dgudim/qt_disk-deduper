#ifndef DATATYPES_H
#define DATATYPES_H

#include <QString>
#include <QVector>
#include <QObject>

#include "ExifTool.h"
#include "gutils.h"

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

    Countable_qstring (const QString& string, quint32 count, qint64 total_size_bytes, double count_percentage = 0, double size_percentage = 0) {
        this->string = string;
        this->count = count;
        this->total_size_bytes = total_size_bytes;
        this->count_percentage = count_percentage;
        this->size_percentage = size_percentage;
    }

    bool operator==(const Countable_qstring &other) const{
        return string == other.string;
    }

    bool operator==(const QString &other) const{
        return string == other;
    }
};

struct File {
    QString full_path;
    QString name;
    qint64 size_bytes;
    QString extension;
    QString hash = "";
    QMap<QString, QString> metadata;

    File (const QString& full_path, const QString& name, const QString& extension, qint64 size_bytes) {
        this->full_path = full_path;
        this->name = name;
        this->extension = extension;
        this->size_bytes = size_bytes;
        metadata.insert("extension", extension);
    }

    void loadMetadata(ExifTool *ex_tool);

    void computeHash() {
        hash = getFileHash(full_path);
    }

    bool operator==(const File &other) const {
        return full_path == other.full_path;
    }

    bool operator<(const File &other) const {
        return full_path < other.full_path;
    }

private:
    bool metadata_loaded = false;
};

QList<QString> getMetaFieldsList();

struct StatsContainer {

    int total_files = 0;
    quint64 total_size = 0;

    QVector<QPair<QString, QVector<Countable_qstring>>> meta_fields_stats;

    StatsContainer(){};

    StatsContainer (const QVector<QPair<QString, QVector<Countable_qstring>>>& meta_fields_stats,
                    int total_files, quint64 total_size){
        this->meta_fields_stats = meta_fields_stats;
        this->total_files = total_files;
        this->total_size = total_size;
    }
};

#endif // DATATYPES_H
