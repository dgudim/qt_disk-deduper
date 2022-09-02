#ifndef META_CONVERTERS_H
#define META_CONVERTERS_H

#include <QString>

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
    name.remove("\n");
    name.remove("\r");
    name.remove("\\");
    name.remove("/");
}

#endif // META_CONVERTERS_H
