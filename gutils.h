#ifndef GUTILS_H
#define GUTILS_H

#include <chrono>

#include <QString>

QString millisecondsToReadable(unsigned long ms) {
    using namespace std::chrono;
    QString time_str = "%1h %2m %3s";
    int seconds = ms / 1000;
    int minutes = seconds / 60;
    int hours = minutes / 60;

    seconds = seconds % 60;
    minutes = minutes % 60;

    return time_str.arg(hours).arg(minutes).arg(seconds);
}

#endif // GUTILS_H
