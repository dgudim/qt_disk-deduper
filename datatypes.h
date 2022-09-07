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

#include <QFuture>

#include <QButtonGroup>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "ExifTool.h"

template<typename T>
using ptr = std::shared_ptr<T>;

template<typename T>
using uptr = std::unique_ptr<T>;

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

enum class LogLevel { DEBUG, INFO, WARNING, ERROR };

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
    QByteArray hash = "";
    QByteArray partial_hash = "";
    QByteArray perceptual_hash;
    QByteArray thumbnail_raw;
    QMap<QString, QString> metadata;

    enum HashType {
        FULL,
        PARTIAL,
        PERCEPTUAL
    };

    File (){ valid = false; }

    File (const QString& full_path) : full_path(full_path) {
        updateMetadata(full_path);
    }

    void updateMetadata(const QFile& qfile);

    QString remapMetaValue(const QString& field, const QString& value);

    bool rename(const QString& new_name);
    bool renameWithoutExtension(const QString& new_name);

    QFuture<void> loadMetadata(const std::function<ExifTool*(QThread* thread)> &ex_tool_factory, QSqlDatabase db);
    QFuture<void> loadHash(QSqlDatabase db, HashType hash_type);
    QFuture<void> loadThumbnail(QSqlDatabase db);

    void saveHashToDb(QSqlDatabase db);
    void saveMetadataToDb(QSqlDatabase db);
    void saveThumbnailToDb(QSqlDatabase db);

    bool operator==(const File &other) const {
        return full_path == other.full_path;
    }

    bool operator<(const File &other) const {
        return full_path < other.full_path;
    }

    operator QString() const { return full_path; }

private:
    QString full_path;

    bool loadHashFromDb(QSqlDatabase db, HashType hash_type);
    bool loadMetadataFromDb(QSqlDatabase db);
    bool loadThumbnailFromDb(QSqlDatabase db);
};

struct FileQuantitySizeCounter {

private:

    qint32 v_quantity = 0;
    quint64 v_size = 0;

public:

    FileQuantitySizeCounter() { };

    FileQuantitySizeCounter(qint32 quantity, quint64 size) : v_quantity(quantity), v_size(size) { }

    qint32 num() const {
        return v_quantity;
    }

    quint64 size() const {
        return v_size;
    }

    QString size_readable() const;

    void reset() {
        v_quantity = 0;
        v_size = 0;
    }

    FileQuantitySizeCounter operator+ (const FileQuantitySizeCounter & other) const {
        return FileQuantitySizeCounter(v_quantity + other.v_quantity, v_size + other.v_size);
    }

    FileQuantitySizeCounter& operator+=(const File& file) {
          v_quantity ++;
          v_size += file.size_bytes;
          return *this;
    }

    FileQuantitySizeCounter& operator+=(const QFile& file) {
          v_quantity ++;
          v_size += file.size();
          return *this;
    }

    bool operator > (const FileQuantitySizeCounter& file) {
        return v_quantity > file.v_quantity;
    }

    bool operator < (const FileQuantitySizeCounter& file) {
        return v_quantity < file.v_quantity;
    }

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

    FileQuantitySizeCounter total_files;

    QVector<QPair<QString, QVector<Countable_qstring>>> meta_fields_stats;

    StatsContainer() {};

    StatsContainer (const QVector<QPair<QString, QVector<Countable_qstring>>>& meta_fields_stats, const FileQuantitySizeCounter& total_files)
        : total_files(total_files), meta_fields_stats(meta_fields_stats) {}
};

#endif // DATATYPES_H
