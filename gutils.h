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

#include <QThread>

#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>

#include <functional>
#include <fstream>
#include <sstream>
#include <string>

#include <QCryptographicHash>

#include <QDebug>

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

    PairList<File, QString> queueFilesToModify(QVector<File> &files_to_delete,
                                               const QString& target_dir, const QString& postfix);

    bool deleteOrRenameFiles(PairList<File, QString> &files_to_delete,
                             const std::function<void(const QString&)> &status_callback,
                             bool rename);

    bool deleteOrRenameFiles(MultiFile &files_to_delete,
                             const std::function<void(const QString&)> &status_callback,
                             bool rename = false, const QString& target_dir = "", const QString& postfix = "");

    QByteArray getFileHash(const QString& full_path);
    QByteArray getPartialFileHash(const QString& full_path);
    QByteArray getPerceptualImageHash(const QString& full_path, int img_size = 32);
    bool comparePerceptualHashes(const QByteArray& hash1, const QByteArray& hash2, int similarity);
    quint64 getDiskReadSizeB();

    quint64 getMemUsedKb();

    QString bytesToReadable(quint64 kb);
    quint64 readableToBytes(const QString &str);

    QByteArray getFileGroupFingerprint(const QVector<File>& group);

    QPixmap generateThumbnail(const File& file, int size);

    QString callDirSelectionDialogue(QWidget* parent, const QString &title);

};

QT_END_NAMESPACE


QT_BEGIN_NAMESPACE

namespace StringUtils {
    QByteArray getStringHash(const QString& string);

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
