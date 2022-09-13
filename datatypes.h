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

#include <shared_mutex>

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

// stores a list of files
typedef QVector<File> MultiFile;

// stores a list of multiFiles
typedef QVector<MultiFile> MultiFileGroup;

// stores a list MultiFileGroups
typedef QVector<MultiFileGroup> MultiFileGroupArray;

struct CountableQString;

//stores a list of qstrings
typedef QVector<CountableQString> CountableQStringList;

//stores a list of CountableQStrings and a qstring specifying the name
typedef QPair<QString, CountableQStringList> NamedCountableQStringList;

//stores an std::function with a name
template<typename T>
using NamedFunction = QPair<QString, std::function<T>>;

//stores a list of named functions
template<typename T>
using NamedFunctionList = QVector<QPair<QString, std::function<T>>>;

//stores a list of pairs
template<typename T1, typename T2>
using PairList = QVector<QPair<T1, T2>>;

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

struct CountableQString {
    QString string;
    quint32 count;
    double count_percentage;
    qint64 total_size_bytes;
    double size_percentage;

    CountableQString(const QString& string) : string(string) {}

    CountableQString (const QString& string, quint32 count, qint64 total_size_bytes, double count_percentage = 0, double size_percentage = 0)
        : string(string), count(count), count_percentage(count_percentage), total_size_bytes(total_size_bytes), size_percentage(size_percentage) {};

    QString size_readable() const;

    bool operator==(const CountableQString &other) const{
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
    QPixmap thumbnail;
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

    static void loadStatisMetaMaps();

    bool loadMetadataFromDb(QSqlDatabase db);
    void loadMetadataFromExifTool(ExifTool* ex_tool);
    QFuture<void> loadHash(QSqlDatabase db, HashType hash_type);
    QFuture<void> loadThumbnail(QSqlDatabase db);
    void postLoadThumbnail();

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
    bool loadThumbnailFromDb(QSqlDatabase db);
};

struct FileQuantitySizeCounter {

private:

    std::shared_mutex mutex;
    qint32 v_quantity = 0;
    quint64 v_size = 0;

    FileQuantitySizeCounter(qint32 quantity, quint64 size) : v_quantity(quantity), v_size(size) { }

public:

    const FileQuantitySizeCounter& operator= (const FileQuantitySizeCounter &other) {
        std::unique_lock lock(mutex);
        v_quantity = other.v_quantity;
        v_size = other.v_size;
        return *this;
    }

    FileQuantitySizeCounter(const FileQuantitySizeCounter &other) {
        *this = other;
    }

    FileQuantitySizeCounter() { }

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
          std::unique_lock lock(mutex); // write lock
          v_quantity ++;
          v_size += file.size_bytes;
          return *this;
    }

    FileQuantitySizeCounter& operator+=(const QFile& file) {
          std::unique_lock lock(mutex); // write lock
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

    QVector<NamedCountableQStringList> meta_fields_stats;

    StatsContainer() {};

    StatsContainer (const QVector<NamedCountableQStringList>& meta_fields_stats, const FileQuantitySizeCounter& total_files)
        : total_files(total_files), meta_fields_stats(meta_fields_stats) {}
};

#endif // DATATYPES_H
