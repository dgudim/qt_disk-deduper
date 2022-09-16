#ifndef META_CONVERTERS_H
#define META_CONVERTERS_H

#include <QString>
#include <QRegExp>
#include <QList>

// supplied datetime should be of this format: %Y:%m:%d %H:%M:%S
void creationDateConverter(QString& datetime, QString target_format) {
    QStringList datetime_split = datetime.split(" ");
    QStringList date = datetime_split[0].split(":");
    QStringList time = datetime_split[1].split(":");
    QString years = date[0];
    QString months = date[1];
    QString days = date[2];
    QString hours = time[0];
    QString minutes = time[1];
    QString seconds = time[2];
    datetime = target_format
            .replace("%Y", years)
            .replace("%m", months)
            .replace("%d", days)
            .replace("%H", hours)
            .replace("%M", minutes)
            .replace("%S", seconds);
}

void durationConverter(QString& duration, QString) {
    if(duration.endsWith("(approx)")) {
        duration.chop(8);
    }
    duration = duration.trimmed();
    if (duration.endsWith("s")) {
        duration.chop(1);
        duration = duration.trimmed();
        duration = "00:00:" + duration;
    }
}

void illegalCharactersRemover(QString& name) {
    name = name.simplified();
    name = name.replace( QRegExp( "[" + QRegExp::escape( "\\/:*?\"<>|" ) + "]" ), QString( "_" ) );
}

#endif // META_CONVERTERS_H
