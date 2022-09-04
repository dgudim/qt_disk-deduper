#ifndef META_CONVERTERS_H
#define META_CONVERTERS_H

#include <QString>
#include <QRegExp>

void durationConverter(QString& duration) {
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
