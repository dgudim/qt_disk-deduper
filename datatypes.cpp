#include "datatypes.h"
#include "gutils.h"
#include "meta_converters.h"
#include <QMimeDatabase>
#include <QApplication>
#include <QStyle>
#include <QBuffer>
#include <QIcon>
#include <QMimeType>
#include <QPainter>

QVector<QString> empty_values = {"", "-", "--", "0000:00:00 00:00:00", "0000:00:00", "00:00:00"};

const QMap<QString, QVector<QPair<QString, std::function<void(QString&)>>>> metadataMap_name_to_fileds =
{
        {"Media type", {{"MIMEType", nullptr}}},

        {"Creation date", {{"MediaCreateDate", nullptr},
                           {"ContentCreateDate", nullptr},
                           {"ContentCreateDate", nullptr},
                           {"DateTimeOriginal", nullptr},
                           {"FileModifyDate", nullptr}}},

        {"Camera model", {{"Model", nullptr},
                          {"CameraModelName", nullptr}}},

        {"Camera manufacturer", {{"Make", nullptr}}},

        {"Width", {{"ImageWidth", nullptr},
                   {"ExifImageWidth", nullptr}}},

        {"Height", {{"ImageHeight", nullptr},
                    {"ExifImageWidth", nullptr}}},

        {"Artist", {{"Artist", nullptr}}},
        {"Title", {{"Title", nullptr}}},
        {"Album", {{"Album", nullptr}}},
        {"Genre", {{"Genre", nullptr}}},

        {"Duration", {{"MediaDuration", durationConverter},
                      {"Duration", durationConverter},
                      {"Track Duration", durationConverter}}},
};

// user-defined maps to remap any xmp value
// for instance E5823 to some meaningfull camera name
QMap<QString, QMap<QString, QString>> metaMaps;
bool metaMapsLoaded = false;

// procedurally generated
QMap<QString, QPair<QString, std::function<void(QString&)>>> metadataMap_field_to_name;
const QList<QString> metaFieldsList = metadataMap_name_to_fileds.keys();

void File::updateMetadata(const QFile &qfile) {

    QFileInfo info(qfile);
    path_without_name = info.fileName();

    path_without_name = info.absolutePath();
    name = info.fileName();
    extension = info.completeSuffix().toLower();
    size_bytes = qfile.size();

    metadata["extension"] = extension;
}

bool File::rename(const QString &new_name) {
    QFile qfile = QFile(full_path);
    bool success = qfile.rename(QDir(path_without_name).filePath(new_name));
    if(success) {
        updateMetadata(qfile);
    }
    return success;
}

bool File::renameWithoutExtension(const QString& new_name) {
    return rename(new_name + "." + extension);
};

void File::loadMetadata(ExifTool *ex_tool, QSqlDatabase db) {

    if(metadata_loaded){
        return;
    }
    metadata_loaded = true;

    // load user metamaps
    if(!metaMapsLoaded) {
        metaMapsLoaded = true;
        QFileInfoList config_files = QDir("./config").entryInfoList(QDir::Filter::Files);
        for(auto& config_file: config_files) {
            QString fileName = config_file.fileName();
            if(!metadataMap_name_to_fileds.contains(fileName)) {
                qWarning() << "Unknown config file:" << fileName;
            } else {
                QFile file(config_file.absoluteFilePath());
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                   QTextStream in(&file);
                   while (!in.atEnd()) {
                      QString line = in.readLine();
                      QStringList mapPair = line.split("=");
                      if(mapPair.size() != 2) {
                          qWarning() << "Invalid config file line:" << line;
                      } else {
                          QString key = mapPair[0].trimmed();
                          if(metaMaps[fileName].contains(key)) {
                               qWarning() << "Duplicate key in config file:" << fileName << "key:" << key;
                          } else {
                               metaMaps[fileName].insert(key, mapPair[1].trimmed());
                          }
                      }
                   }
                   file.close();
                }
            }
        }
    }

    // construct lookup table if it doesn't exist
    if (metadataMap_field_to_name.isEmpty()) {
        qDebug() << "metadataMap_field_to_name constructed";
        for (auto& key: metaFieldsList) {
            for (auto& meta_field_and_converter: metadataMap_name_to_fileds[key]){
                metadataMap_field_to_name.insert(meta_field_and_converter.first, {key, meta_field_and_converter.second});
            }
        }
    }

    if(loadMetadataFromDb(db)) {
        return;
    }

    // get known values
    QMap<QString, QString> gathered_values;

    TagInfo *info = ex_tool->ImageInfo(full_path.toStdString().c_str(), "-d\n\"%Y:%m:%d %H:%M:%S\"");
    if (info) {
        for (TagInfo *i = info; i; i = i->next) {
            QString name = QString::fromUtf8(i->name);
            QString value = QString::fromUtf8(i->value);
            if(metadataMap_field_to_name.contains(name) && !empty_values.contains(value)) {
                gathered_values.insert(name, value);
            }
        }
        delete info;
    } else if (ex_tool->LastComplete() <= 0) {
        qCritical() << "Error executing exiftool on " + full_path;
    }

    for(auto& out_field: metaFieldsList) {
        // iterate thrugh all suitable fields
        for(auto& meta_field_and_converter: metadataMap_name_to_fileds[out_field]) {
            // check if if was returned by exiftool and is not considered empty
            if(gathered_values.contains(meta_field_and_converter.first)) {

                QString value = gathered_values[meta_field_and_converter.first].trimmed();
                // use converter if provided
                if(meta_field_and_converter.second){
                    meta_field_and_converter.second(value);
                }
                metadata.insert(out_field, remapMetaValue(out_field, value));
            }
        }
        // no suitable field was found
        if (!metadata.contains(out_field)) {
            qDebug() << "Could not get" << out_field << "for" << full_path;
            metadata.insert(out_field, "");
        }
    }

    char *err = ex_tool->GetError();
    if (err) qWarning() << err;

    saveMetadataToDb(db);
}

QString File::remapMetaValue(const QString& field, const QString& value) {
    if(metaMaps.contains(field) && metaMaps.value(field).contains(value)) {
        qDebug() << "Mapped" << value << "to" << metaMaps[field][value];
        return metaMaps[field][value];
    }
    return value;
}

void File::loadHash(QSqlDatabase db) {
    if(!loadHashFromDb(db)) {
        hash = FileUtils::getFileHash(full_path);
        saveHashToDb(db);
    }
}

QMimeDatabase mime_database;

void File::loadThumbnail(QSqlDatabase db) {
    if(!loadThumbnailFromDb(db)) {

        // try to get thumbnail directly from file
        QPixmap icon_pix = QIcon(full_path).pixmap(200, 200);

        if(icon_pix.isNull()) {
            icon_pix = FileUtils::generateThumbnail(full_path, 200);
        }

        if(icon_pix.isNull()) {
            // try to get filetype icon from system theme

            QIcon icon;
            QList<QMimeType> mime_types = mime_database.mimeTypesForFileName(name);
            for (int i = 0; i < mime_types.count() && icon.isNull(); i++) {
                icon = QIcon::fromTheme(mime_types[i].iconName());
            }

            if (icon.isNull()) {
                icon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
            }

            icon_pix = icon.pixmap(128, 128);
        }

        QBuffer inBuffer( &thumbnail_raw );
        inBuffer.open( QIODevice::WriteOnly );
        icon_pix.save( &inBuffer, "PNG" );
        saveThumbnailToDb(db);
    }
}

void File::saveThumbnailToDb(QSqlDatabase db) {
    QString insertionString = QString("INSERT OR REPLACE INTO thumbnails (full_path, size, thumbnail) "
                                      "VALUES(:full_path, :size, :thumbnail)");

    QSqlQuery query(db);
    query.prepare(insertionString);

    query.bindValue(":full_path", full_path);
    query.bindValue(":size", size_bytes);
    query.bindValue(":thumbnail", thumbnail_raw);

    DbUtils::execQuery(query);
}

void File::saveHashToDb(QSqlDatabase db) {
    QString insertionString = QString("INSERT OR REPLACE INTO hashes (full_path, size, hash) "
                                      "VALUES(:full_path, :size, :hash)");
    QSqlQuery query(db);
    query.prepare(insertionString);

    query.bindValue(":full_path", full_path);
    query.bindValue(":size", size_bytes);
    query.bindValue(":hash", hash);

    DbUtils::execQuery(query);
}

void File::saveMetadataToDb(QSqlDatabase db) {

    QString columns;
    QString values;
    QList<QString> metaFieldsForDb = metaFieldsList;
    for(auto& meta_field: metaFieldsForDb) {
        meta_field.replace(" ", "_");
        columns += "," + meta_field;
        values += ", :" + meta_field;
        meta_field = ":" + meta_field;
    }

    QString insertionString = QString("INSERT OR REPLACE INTO metadata (full_path, size%1) "
                                      "VALUES(:full_path, :size%2)")
            .arg(columns, values);

    QSqlQuery query(db);
    query.prepare(insertionString);

    query.bindValue(":full_path", full_path);
    query.bindValue(":size", size_bytes);

    for(int i = 0; i < metaFieldsList.length(); i++) {
        query.bindValue(metaFieldsForDb.at(i), metadata[metaFieldsList.at(i)]);
    }

    DbUtils::execQuery(query);
}

// load metadata from database if present, return true if loading succeeded
bool File::loadMetadataFromDb(QSqlDatabase db) {

    QString columns;
    for(auto meta_field: metaFieldsList) {
        columns += "," + meta_field.replace(" ", "_");
    }

    QSqlQuery query(db);
    query.prepare(QString("SELECT full_path, size %1 FROM metadata WHERE full_path = ?").arg(columns));
    query.bindValue(0, full_path);

    DbUtils::execQuery(query);

    if(query.first() && query.value(1) == size_bytes) {
        for(int i = 0; i < metaFieldsList.size(); i++) {
            metadata.insert(metaFieldsList.at(i), remapMetaValue(metaFieldsList.at(i), query.value(i + 2).toString()));
        }
        return true;
    }
    return false;
}

// load hash from database if present, return true if loading succeeded
bool File::loadHashFromDb(QSqlDatabase db) {
    QSqlQuery query(db);
    query.prepare("SELECT full_path, size, hash FROM hashes WHERE full_path = ?");
    query.bindValue(0, full_path);

    DbUtils::execQuery(query);

    if(query.first() && query.value(1) == size_bytes) {
        hash = query.value(2).toString();
        return true;
    }
    return false;
}

bool File::loadThumbnailFromDb(QSqlDatabase db) {
    QSqlQuery query(db);
    query.prepare(QString("SELECT full_path, size, thumbnail FROM thumbnails WHERE full_path = ?"));
    query.bindValue(0, full_path);

    DbUtils::execQuery(query);

    if(query.first() && query.value(1) == size_bytes) {
        thumbnail_raw = query.value(2).toByteArray();
        return true;
    }
    return false;
}

QList<QString> getMetaFieldsList() {
    return metaFieldsList;
}

ExifFormat::ExifFormat(const QString &format_string_raw, OnFailAction onFailAction, OnFileExistsAction onFileExistsAction)
    : onFailAction(onFailAction), onFileExistsAction(onFileExistsAction) {

    QList<QString> metaFieldsList = getMetaFieldsList();

    bool parenth_opened = false;
    int param_index = 1;

    QString metaField;
    for(auto& ch: format_string_raw) {
        if(ch != '[' && ch != ']') {
            if(parenth_opened) {
                metaField += ch;
            } else {
                format_string += ch;
            }
        }
        if(ch == '[') {
            if(parenth_opened) {
                valid = false;
                break;
            }
            parenth_opened = true;
        } else if (ch == ']') {
            if(!parenth_opened) {
                valid = false;
                break;
            }
            if(!metaFieldsList.contains(metaField)) {
                valid = false;
                break;
            }
            metaFieldKeys.append(metaField);
            metaField = "";
            format_string += "%" + QString::number(param_index);
            param_index ++;
            parenth_opened = false;
        }
    }
    if(valid && parenth_opened) {
        valid = false;
    }
    if(valid && format_string_raw.isEmpty()) {
        valid = false;
    }
}

bool ExifFormat::rename(File &file) {
    if(valid) {
        QString final_str = format_string;
        for(auto& key: metaFieldKeys) {
            if(file.metadata[key].isEmpty()) {
                switch(onFailAction) {
                    case OnFailAction::STOP_PROCESS:
                        qCritical() << "stopped renaming on:" << file << ", (could not get" << key << ")";
                        return false;
                        break;
                    case OnFailAction::SKIP_FILE:
                        qWarning() << "skipped file:" << file << ", (could not get" << key << ")";
                        return true;
                    case OnFailAction::DO_NOTHING:
                        break;
                }
            }
            final_str = final_str.arg(file.metadata[key]);
        }
        bool success = file.renameWithoutExtension(final_str);
        if(!success) {
            switch(onFileExistsAction) {
                case OnFileExistsAction::STOP_PROCESS:
                    qCritical() << "stopped renaming on:" << file << ", (file already exists)";
                    return false;
                    break;
                case OnFileExistsAction::SKIP_FILE:
                    qWarning() << "skipped file:" << file << ", (file already exists)";
                    return true;
                    break;
                case OnFileExistsAction::APPEND_INDEX:
                    int index = 1;
                    while(true) {
                        qInfo() << "file:" << file << "already exists, trying index:" << index;
                        success = file.renameWithoutExtension(QString("%1(%2)").arg(final_str).arg(index));
                        index ++;
                        if(success) {
                            return true;
                        }
                    }
                    break;
            }
        }
        return true;
    }
    return false;
}
