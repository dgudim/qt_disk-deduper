#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <QString>
#include <QStringList>
#include <QDir>
#include <QDirIterator>
#include <QtDebug>

QStringList walkDir(const QString dir) {
    QStringList list;
    QDirIterator it(dir, QStringList() << "*.*", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()){
        list << it.next();
    }
    return list;
}

#endif // FILE_UTILS_H
