#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <QString>
#include <QStringList>
#include <QDir>
#include <QDirIterator>
#include <QtDebug>

#include <functional>
#include <fstream>
#include <sstream>
#include <string>

#include <QCryptographicHash>

using std::function;

QStringList walkDir(const QString& dir, function<void(QString)> callback) {
    QStringList list;
    QDirIterator it(dir, QStringList() << "*.*", QDir::Files, QDirIterator::Subdirectories);
    QString current;
    while (it.hasNext()){
        current = it.next();
        callback(current);
        list << current;
    }
    return list;
}

QString getFileHash(const QString& file_path) {
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error("Failed opening " + file_path.toStdString());
    QString hash = QCryptographicHash::hash(file.readAll(), QCryptographicHash::Algorithm::Sha256).toHex();
    file.close();
    return hash;
}

bool string_starts_with(const std::string& string, const std::string& prefix) {
    if (prefix.size() > string.size()) {
        return false;
    }
    if (prefix.size() == string.size()) {
        return string == prefix;
    }
    for (size_t i = 0; i < prefix.size(); i++) {
        if (string[i] != prefix[i]) {
            return false;
        }
    }
    return true;
}

bool stringStartsWithAny(const std::string& str, std::vector<std::string>& list) {
    for(auto& item: list) {
        if(string_starts_with(str, item)){
            return true;
        }
    }
    return false;
}

unsigned long getDiskReadSizeKb() {

    unsigned long sectors_read = 0;

    std::ifstream diskstat("/proc/diskstats");
    std::string line;
    std::vector<std::string> discs;

    while (getline(diskstat, line)){
        int stat_index = 0;
        std::string temp;
        std::stringstream   line_ss{ line };
        while (std::getline(line_ss, temp, ' ')) {

            //skip spaces
            if(temp.empty()){
               continue;
            }

            //only get whole drives, not partitions
            if(stat_index == 2){
                if( stringStartsWithAny(temp, discs)) {
                    break;
                } else {
                    discs.push_back(temp);
                }
            }

            // get 'sectors read' field (https://www.kernel.org/doc/Documentation/ABI/testing/procfs-diskstats)
            if(stat_index == 5){
                sectors_read += stoul(temp);
                break;
            }
            stat_index ++;
        }
    }

    return sectors_read * 512 / 1024;
}

QString kbToReadable(unsigned long kb){
    float mb = kb / 1024.0;
    float gb = mb / 1024.0;
    if(mb > 500){
       return QString("%1 Gb").arg(gb);
    }
    if(mb > 5){
       return QString("%1 Mb").arg(mb);
    }
    return QString("%1 Kb").arg(kb);
}

#endif // FILE_UTILS_H
