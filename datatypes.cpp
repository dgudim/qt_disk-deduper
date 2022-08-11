#include "datatypes.h"

QVector<QString> empty_values = {"", "-", "--", "0000:00:00 00:00:00", "0000:00:00", "00:00:00"};

QMap<QString, QVector<QPair<QString, std::function<QString(QString)>>>> metadataMap_name_to_fileds =
{
        {"Media type", {{"MIMEType", nullptr}}},

        {"Creation date", {{"MediaCreateDate", nullptr},
                           {"ContentCreateDate", nullptr},
                           {"FileModificationDate/Time", nullptr}}},

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

        {"Duration", {{"MediaDuration", nullptr},
                      {"Duration", nullptr},
                      {"Track Duration", nullptr}}},
};

// procedurally generated
QMap<QString, QPair<QString, std::function<QString(QString)>>> metadataMap_field_to_name;
QList<QString> fieldList = metadataMap_name_to_fileds.keys();

void File::loadMetadata(ExifTool *ex_tool) {

    if(metadata_loaded){
        return;
    }
    metadata_loaded = true;

    // construct lookup table if it doesn't exist
    if (metadataMap_field_to_name.isEmpty()) {
        qDebug() << "metadataMap_field_to_name constructed";
        for (auto& key: fieldList){
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
            if(metadataMap_field_to_name.contains(name)) {
                gathered_values.insert(name, QString::fromUtf8(i->value));
            }
        }
        delete info;
    } else if (ex_tool->LastComplete() <= 0) {
        qCritical() << "Error executing exiftool on " + full_path;
    }

    for(auto& out_field: fieldList) {
        // iterate thrugh all suitable fields
        for(auto& meta_field_and_converter: metadataMap_name_to_fileds[out_field]) {
            // check if if was returned by exiftool and is not considered empty
            if(gathered_values.contains(meta_field_and_converter.first) &&
                !empty_values.contains(gathered_values[meta_field_and_converter.first])) {

                QString value = gathered_values[meta_field_and_converter.first].trimmed();
                // use converter if provided
                if(meta_field_and_converter.second){
                    value = meta_field_and_converter.second(value);
                }
                metadata.insert(out_field, value);
            }
        }
        // no suitable field was found
        if (!metadata.contains(out_field)) {
            qInfo() << "Could not get" << out_field << "for" << full_path;
            metadata.insert(out_field, "none");
        }
    }

    char *err = ex_tool->GetError();
    if (err) qCritical() << err;
}

QList<QString> getMetaFieldsList() {
    return fieldList;
}
