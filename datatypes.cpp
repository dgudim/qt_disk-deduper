#include "datatypes.h"
#include "gutils.h"
#include "meta_converters.h"
#include <QBuffer>
#include <QIcon>

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

// procedurally generated
QMap<QString, QPair<QString, std::function<void(QString&)>>> metadataMap_field_to_name;
const QList<QString> metaFieldsList = metadataMap_name_to_fileds.keys();

void File::loadMetadata(ExifTool *ex_tool, QSqlDatabase db) {

    if(metadata_loaded){
        return;
    }
    metadata_loaded = true;

    if(loadMetadataFromDb(db)) {
        return;
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

    // get known values
    QMap<QString, QString> gathered_values;

    TagInfo *info = ex_tool->ImageInfo(full_path.toStdString().c_str());
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
                metadata.insert(out_field, value);
            }
        }
        // no suitable field was found
        if (!metadata.contains(out_field)) {
            qDebug() << "Could not get" << out_field << "for" << full_path;
            metadata.insert(out_field, "");
        }
    }

    char *err = ex_tool->GetError();
    if (err) qCritical() << err;

    saveMetadataToDb(db);
}

void File::loadHash(QSqlDatabase db) {
    if(!loadHashFromDb(db)) {
        hash = FileUtils::getFileHash(full_path);
        saveHashToDb(db);
    }
}

void File::loadThumbnail(QSqlDatabase db) {
    if(!loadThumbnailFromDb(db)) {

        QPixmap thumbnail = QIcon(full_path).pixmap(200, 200);
        QBuffer inBuffer( &thumbnail_raw );
        inBuffer.open( QIODevice::WriteOnly );
        thumbnail.save( &inBuffer, "JPG" );

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
            metadata.insert(metaFieldsList.at(i), query.value(i + 2).toString());
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
