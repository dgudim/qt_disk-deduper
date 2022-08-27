#ifndef DATATYPES_H
#define DATATYPES_H

#include <QString>
#include <QVector>
#include <QObject>
#include <QDebug>

#include <QPixmap>

#include <QDir>
#include <QFile>

#include <QMap>
#include <QDataStream>

#include <QButtonGroup>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "ExifTool.h"

template<typename T>
using ptr = std::shared_ptr<T>;

// stores one button group in the tab
typedef ptr<QButtonGroup> ButtonGroup;

// stores all button groups of one tab
typedef QVector<ButtonGroup> ButtonGroups;

// stores all button groups of one tab (pointer)
typedef ptr<ButtonGroups> pButtonGroups;

// stores all button groups of all tabs
typedef QVector<pButtonGroups> ButtonGroupsPerTab;

struct File;

// stores files with the same hash
typedef QVector<File> MultiFile;

// stores multiFiles with thre same fingerprint
typedef QVector<MultiFile> MultiFileGroup;

// stores multiple MultiFileGroups
typedef QVector<MultiFileGroup> MultiFileGroupArray;

enum class MediaType {
    UNKNOWN,
    IMAGE,
    VIDEO,
    AUDIO
};

enum class OnFailAction {
    DO_NOTHING = 0,
    SKIP_FILE = 1,
    STOP_PROCESS = 2
};

enum class OnFileExistsAction {
    APPEND_INDEX = 0,
    SKIP_FILE = 1,
    STOP_PROCESS = 2
};

struct Countable_qstring {
    QString string;
    quint32 count;
    double count_percentage;
    qint64 total_size_bytes;
    double size_percentage;

    Countable_qstring(const QString& string) : string(string) {}

    Countable_qstring (const QString& string, quint32 count, qint64 total_size_bytes, double count_percentage = 0, double size_percentage = 0)
        : string(string), count(count), count_percentage(count_percentage), total_size_bytes(total_size_bytes), size_percentage(size_percentage) {};

    bool operator==(const Countable_qstring &other) const{
        return string == other.string;
    }

    bool operator==(const QString &other) const{
        return string == other;
    }
};

QList<QString> getMetaFieldsList();

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

    File (const QString& full_path) : full_path(full_path) {
        updateMetadata(full_path);
    }

    void updateMetadata(const QFile& qfile);

    bool rename(const QString& new_name);
    bool renameWithoutExtension(const QString& new_name);

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

// exif rename format container
struct ExifFormat {

    QString format_string;
    QVector<QString> metaFieldKeys;
    OnFailAction onFailAction;
    OnFileExistsAction onFileExistsAction;
    bool valid = true;

    ExifFormat (){}

    ExifFormat(const QString& format_string_raw, OnFailAction onFailAction, OnFileExistsAction onFileExistsAction);

    bool isValid() {
        return valid;
    }

    bool rename(File &file);
};

struct StatsContainer {

    int total_files = 0;
    quint64 total_size = 0;

    QVector<QPair<QString, QVector<Countable_qstring>>> meta_fields_stats;

    StatsContainer() {};

    StatsContainer (const QVector<QPair<QString, QVector<Countable_qstring>>>& meta_fields_stats, int total_files, quint64 total_size)
        : total_files(total_files), total_size(total_size), meta_fields_stats(meta_fields_stats) {}
};

#endif // DATATYPES_H
