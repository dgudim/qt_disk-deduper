#include "gutils.h"

#pragma region File utils {

QMap<QString, quint64> size_multipliers = {{"b", 1}, {"Kb", 1024}, {"Mb", 1048576}, {"Gb", 1073741824}};

void walkDir(const QString& dir, const QStringList& blacklisted_dirs, const QStringList& extensions,
             ExtenstionFilterState extFilterState, std::function<void(QString)> callback) {

    bool ext_filter_enabled = extFilterState != ExtenstionFilterState::DISABLED;
    bool blacklist_ext = extFilterState == ExtenstionFilterState::ENABLED_BLACK;

    QDir directory(dir);
    directory.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);

    QFileInfoList list = directory.entryInfoList();
    for(const auto& file_or_folder: list) {
        if(file_or_folder.isFile()) {
            bool add = !ext_filter_enabled;
            for(const auto& extension: extensions) {
                if(file_or_folder.completeSuffix().toLower().endsWith(extension) != blacklist_ext) {
                    add = true;
                    break;
                }
            }
            if(add) {
                callback(file_or_folder.absoluteFilePath());
            }
        } else if (file_or_folder.isDir() && !blacklisted_dirs.contains(file_or_folder.absoluteFilePath())){
            walkDir(file_or_folder.absoluteFilePath(), blacklisted_dirs, extensions, extFilterState, callback);
        }
    }
}

QString getFileHash(const QString& full_path) {
    QFile file = QFile(full_path);
    if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error("Failed opening " + full_path.toStdString());
    QString hash = QCryptographicHash::hash(file.readAll(), QCryptographicHash::Algorithm::Sha256).toHex();
    file.close();
    return hash;
}

bool stringStartsWith(const std::string& string, const std::string& prefix) {
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
        if(stringStartsWith(str, item)){
            return true;
        }
    }
    return false;
}

quint64 getDiskReadSizeB() {

    quint64 sectors_read = 0;

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

    return sectors_read * 512;
}

quint64 getMemUsedKb() {

    quint64 total_mem_all = 0;
    quint64 total_mem_free = 0;

    std::ifstream meminfo("/proc/meminfo");
    std::string line;

    while (getline(meminfo, line)) {
        bool available = stringStartsWith(line, "MemAvailable") || stringStartsWith(line, "SwapFree");
        bool total = stringStartsWith(line, "MemTotal") || stringStartsWith(line, "SwapTotal");
        // get RAM available                            get SWAP available
        if (available || total) {
            std::string temp;
            std::stringstream   line_ss{ line };
            while (std::getline(line_ss, temp, ' ')) {
                //skip spaces and var name
                if(temp.empty() || !isdigit(temp.at(0))){
                   continue;
                }
                if (total) {
                    total_mem_all += stoul(temp);
                } else {
                    total_mem_free += stoul(temp);
                }
                break;
            }
        }
        // we don't need subsequent values
        if (stringStartsWith(line, "SwapFree") || stringStartsWith(line, "Dirty")){
            break;
        }
    }

    return total_mem_all - total_mem_free;
};

QString bytesToReadable(quint64 b) {
    double kb = b / 1024.0;
    double mb = kb / 1024.0;
    double gb = mb / 1024.0;
    if(mb > 500){
       return QString("%1 Gb").arg(gb);
    }
    if(mb > 5){
       return QString("%1 Mb").arg(mb);
    }
    if(kb > 1){
       return QString("%1 Kb").arg(kb);
    }
    return QString("%1 b").arg(b);
}

quint64 readableToBytes(QString str) {
    QStringList split = str.split(" ");
    return split[0].toDouble() * size_multipliers[split[1]];
}

#pragma endregion}

QString millisecondsToReadable(quint64 ms) {

    QString time_str = "%1h %2m %3s";
    int seconds = ms / 1000;
    int minutes = seconds / 60;
    int hours = minutes / 60;

    seconds = seconds % 60;
    minutes = minutes % 60;

    return time_str.arg(hours).arg(minutes).arg(seconds);
}

QString timeSinceTimestamp(quint64 ms) {
    return millisecondsToReadable(QDateTime::currentMSecsSinceEpoch() - ms);
};



bool DbUtils::execQuery(QSqlQuery query) {
    if(!query.exec()) {
        qCritical() << query.lastError() << " query: " << query.lastQuery();
        return false;
    }
    return true;
}

bool DbUtils::execQuery(QSqlDatabase db, QString query_str) {
    QSqlQuery query(db);
    if(!query.exec(query_str)) {
        qCritical() << query.lastError() << " query: " << query.lastQuery();
        return false;
    }
    return true;
}

QSqlDatabase DbUtils::openDbConnection() {
    // make a new connaction with current thread id
    QSqlDatabase storage_db = QSqlDatabase::addDatabase("QSQLITE", QString("conn_%1").arg((uintptr_t)QThread::currentThread()));
    qInfo() << "Opened db connection " << storage_db.connectionName();
    storage_db.setDatabaseName(QDir(QApplication::applicationDirPath()).filePath("index.db"));
    if(!storage_db.open()) {
        qCritical() << storage_db.lastError();
    }
    return storage_db;
}
