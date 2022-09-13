#include "mainwindow.h"

#include "ui_mainwindow.h"

#include <QtConcurrent/QtConcurrent>

#include "stats_dialog.h"
#include "dupe_results_dialog.h"
#include "exif_rename_builder_dialog.h"
#include "dynamic_selection_dialog.h"
#include "move_confirmation_dialog.h"

enum PiechartIndex {
    ALL_FILES = 0,
    PREPROCESSSED_FILES = 1,
    PROCESSSED_FILES = 2,
    DUPLICATE_FILES = 3,
    UNIQUE_FILES = 4,
    PRELOADED_FILES = 5
};

QVector<QPair<QString, QColor>> piechart_config = {
    {"A: %1", QColor(1, 184, 170)},
    {"PPr: %1", QColor(3, 191, 71)},
    {"Pr: %1", QColor(3, 171, 51)},
    {"Dup: %1", QColor(181, 113, 4)},
    {"U: %1", QColor(138, 84, 255)},
    {"Pre: %1", QColor(191, 140, 0)}};

enum ScanMode {
    HASH_COMPARE = 0,
    PHASH_COMPARE = 1,
    NAME_COMPARE = 2,
    AUTO_DEDUPE_MOVE = 3,
    AUTO_DEDUPE_RENAME = 4,
    EXIF_RENAME = 5,
    SHOW_STATS = 6
};

struct ScanModeProperties {
    QString name;
    QString description;
    std::function<QString(MainWindow*)> request_function;
    std::function<void(MainWindow*, QSqlDatabase)> process_function;
    std::function<void(MainWindow*)> display_function;
};

QList<ScanModeProperties> scan_modes = {
    {"Hash duplicates", "Compare files by hash and show results in groups for further action", nullptr, &MainWindow::hashCompare, &MainWindow::fileCompare_display},
    {"Find similar files", "Compare files by perceptual hash and show results in groups for further action", nullptr, &MainWindow::phashCompare, &MainWindow::fileCompare_display},
    {"Name duplicates", "Compare files by name and show results in groups for further action", nullptr, &MainWindow::nameCompare, &MainWindow::fileCompare_display},
    {"Auto dedupe(move)", "Compare master folder and slave folders by hash (Files from the slave folders are moved into the dupes folder if they are present in the master folder)", &MainWindow::autoDedupe_request, &MainWindow::autoDedupe_move, &MainWindow::autoDedupe_display},
    {"Auto dedupe(rename)", "Compare master folder and slave folders by hash (DELETED_ is added to the name of a file from the slave folders if it is present in the master folder)", &MainWindow::autoDedupe_request, &MainWindow::autoDedupe_rename, &MainWindow::autoDedupe_display},
    {"EXIF rename", "Rename files according to their EXIF data (Name format: <creation date and time>_<camera model>_numbers from file name)", &MainWindow::exifRename_request, &MainWindow::exifRename, nullptr},
    {"Show statistics", "Get statistics of selected folders (Extensions, camera models) and display them", &MainWindow::showStats_request, &MainWindow::showStats, &MainWindow::showStats_display}};

QList<QPair<QString, QList<QString>>> extension_bundles = {
    {"image", {"jpg", "jpeg", "jpe", "jif", "jfif", "jfi", "png", "gif", "webp", "tiff", "tif", "heif", "heic", "raw", "arw", "cr", "cr2", "rw2", "nrw", "k25", "svg" }},
    {"video", {"avi", "mp4", "mkv", "mk3d", "mov", "webm"}}
};

#define MOVE_TO_UI_THREAD(func, ...) QMetaObject::invokeMethod(this_window, func, Qt::QueuedConnection, __VA_ARGS__);

MainWindow *MainWindow::this_window = 0;
QTextStream MainWindow::currentLogFileStream;

const LogLevel logLevel = LogLevel::INFO;
QSettings settings(QSettings::UserScope, "disk_deduper_qt", "ui_state");

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow) {

    // init local sqlite database
    QSqlDatabase storage_db = DbUtils::openDbConnection();

    if(storage_db.isOpen()) {

        QString columns;
        QList<QString> metaFieldsList = getMetaFieldsList();
        for(auto& meta_field: metaFieldsList) {
            columns += "," + meta_field.replace(" ", "_") + " TEXT";
        }

        QString init_query_metadata = QString("CREATE TABLE IF NOT EXISTS metadata (full_path TEXT PRIMARY KEY, size INTEGER %1)").arg(columns);
        QString init_query_hashes = "CREATE TABLE IF NOT EXISTS hashes (full_path TEXT PRIMARY KEY, size INTEGER, hash BLOB, partial_hash BLOB, perceptual_hash BLOB)";
        QString init_query_thumbnails = "CREATE TABLE IF NOT EXISTS thumbnails (full_path TEXT PRIMARY KEY, size INTEGER, thumbnail BLOB)";

        qInfo() << "Init db (metadata): " << init_query_metadata;
        qInfo() << "Init db (hashes): " << init_query_hashes;
        qInfo() << "Init db (thumbnails): " << init_query_thumbnails;

        DbUtils::execQuery(storage_db, init_query_metadata);
        DbUtils::execQuery(storage_db, init_query_hashes);
        DbUtils::execQuery(storage_db, init_query_thumbnails);
    }

    ui->setupUi(this);

    idealThreadCount = QThreadPool::globalInstance()->maxThreadCount();
    for(int i = 0; i < idealThreadCount; i++) {
        ex_tools.append(new ExifTool());
    }

    setWindowTitle("Disk deduper");

    this_window = this;

    // setup initial values
    program_start_time = QDateTime::currentMSecsSinceEpoch();
    lastMeasuredDiskRead = FileUtils::getDiskReadSizeB();
    currentLogFileStream.setDevice(&currentLogFile);
    QDir().mkdir("./logs");
    QDir().mkdir("./config");
    QDir().mkdir("./temp");

    File::loadStatisMetaMaps();

    setCurrentTask("Idle");

    QStringList modeNames;
    for(auto& [mode_name, mode_description, pfunc, dfunc, rfunc]: scan_modes) {
        modeNames.push_back(mode_name);
    }
    ui->mode_combo_box->insertItems(0, modeNames);

    // load previous mode
    int mode = settings.value("current_mode", ScanMode::HASH_COMPARE).toInt();
    ui->mode_combo_box->setCurrentIndex(mode);
    onCurrentModeChanged(mode);

    // load previous similarity
    int similarity = settings.value("similarity", 1).toInt();
    ui->similarity_slider->setValue(similarity);
    onSimilarityChanged(similarity);

    // setup piechart
    series = new QPieSeries();

    for (auto& [label, color]: piechart_config){
        auto slice = series->append("", 0);
        slice->setBrush(QBrush(color));
        slice->setLabelBrush(QBrush(Qt::white));
    }

    series->setLabelsVisible(true);
    series->setLabelsPosition(QPieSlice::LabelInsideHorizontal);
    series->setPieSize(2);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setBackgroundVisible(false);
    chart->setMargins({0, 0, 0, 0});
    chart->setBackgroundRoundness(0);
    chart->legend()->hide();

    ui->qchart->setChart(chart);

    auto timer_100Ms = new QTimer(this);
    timer_100Ms->setInterval(100); // 100ms (10 time per second)
    connect(timer_100Ms, &QTimer::timeout, this, &MainWindow::updateLoop100Ms);
    timer_100Ms->start();

    auto timer_2s = new QTimer(this);
    timer_2s->setInterval(2000); // 2000ms (0.5 times per second)
    connect(timer_2s, &QTimer::timeout, this, &MainWindow::updateLoop2s);
    timer_2s->start();

    // load folders and extensions
    loadList("folders", ui->folders_to_scan_list);
    loadList("extensions", ui->extension_filter_list);
    masterFolder = settings.value("master_folder").toString();
    dupesFolder = settings.value("dupes_folder").toString();
    ui->speed_based_eta_checkbox->setChecked(settings.value("eta_spped").toBool());
    ui->master_folder_label->setText(QString("Master folder: %1").arg(masterFolder));
    ui->dupes_folder_label->setText(QString("Dupes folder: %1").arg(dupesFolder));

    // setup ui listeners
    connect(ui->add_scan_folder_button, &QPushButton::clicked, this, &MainWindow::onAddScanFolderClicked);

    connect(ui->set_master_folder_button, &QPushButton::clicked, this, &MainWindow::onSetMasterFolderClicked);
    connect(ui->set_dupes_folder_button, &QPushButton::clicked, this, &MainWindow::onSetDupesFolderClicked);

    connect(ui->add_extention_button, &QPushButton::clicked, this, &MainWindow::onAddExtensionButtonClicked);
    connect(ui->add_extention_bundle_button, &QPushButton::clicked, this, &MainWindow::onAddExtensionBundleButtonClicked);

    connect(ui->mode_combo_box, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentModeChanged(int)));

    connect(ui->similarity_slider, &QSlider::sliderMoved, this, &MainWindow::onSimilarityChanged);

    // load extension filter state
    Qt::CheckState ext_filter_state = (Qt::CheckState)settings.value("ext_filter_state", FileUtils::DISABLED).toInt();
    ui->extention_filter_enabled_checkbox->setCheckState(ext_filter_state);
    onExtentionCheckboxStateChanged(ext_filter_state);

    connect(ui->extention_filter_enabled_checkbox, SIGNAL(stateChanged(int)), this, SLOT(onExtentionCheckboxStateChanged(int)));

    connect(ui->start_scan_button, &QPushButton::clicked, this, &MainWindow::onStartScanButtonClicked);
}

MainWindow::~MainWindow() {

    // save ui state on exit
    settings.setValue("current_mode", ui->mode_combo_box->currentIndex());
    settings.setValue("ext_filter_state", ui->extention_filter_enabled_checkbox->checkState());

    saveList("folders", ui->folders_to_scan_list);
    saveList("extensions", ui->extension_filter_list);

    settings.setValue("master_folder", masterFolder);
    settings.setValue("dupes_folder", dupesFolder);

    settings.setValue("eta_speed", ui->speed_based_eta_checkbox->isChecked());
    settings.setValue("similarity", ui->similarity_slider->value());

    for(auto& ex_tool: ex_tools) {
        delete ex_tool;
    }

    delete ui;
}

#pragma region MainWindow loops {

void MainWindow::updateLoop100Ms() {
     // update uptime
    ui->uptime_label->setText(QString("Uptime: %1").arg(TimeUtils::timeSinceTimestamp(program_start_time)));

    // update piechart
    series->slices().at(PiechartIndex::ALL_FILES)->setValue(total_files.num() - qMax(processed_files.num(), preprocessed_files.num()));
    // limit to 0 because we may load files withoud preloading
    series->slices().at(PiechartIndex::PREPROCESSSED_FILES)->setValue(qMax(preprocessed_files.num() - processed_files.num(), 0));
    // substract duplicate_files and unique_files
    series->slices().at(PiechartIndex::PROCESSSED_FILES)->setValue(processed_files.num() - duplicate_files.num() - unique_files.num());
    // avoid negative values as preloaded_files also preloads unique_files
    series->slices().at(PiechartIndex::DUPLICATE_FILES)->setValue(qMax(duplicate_files.num() - preloaded_files.num(), 0));
    series->slices().at(PiechartIndex::PRELOADED_FILES)->setValue(preloaded_files.num());
    // when preloaded_files finishes 'eating' duplicate_files the value of duplicate_files - preloaded_files
    // becomes negative and preloaded_files starts 'eating' unique_files
    series->slices().at(PiechartIndex::UNIQUE_FILES)->setValue(unique_files.num() + qMin(duplicate_files.num() - preloaded_files.num(), 0));

    for (int i = 0; i < series->count(); i++) {
        auto slice = series->slices().at(i);
        slice->setLabel(QString(piechart_config[i].first).arg(slice->value()));
    }

    // update stats
    ui->total_files_label->setText(QString("Total files: %1").arg(total_files.num()));
    ui->total_files_size_label->setText(QString("size: %1").arg(total_files.size_readable()));

    ui->preprocessed_files_label->setText(QString("Preprocessed files: %1").arg(preprocessed_files.num()));
    ui->preprocessed_files_size_label->setText(QString("size: %1").arg(preprocessed_files.size_readable()));

    ui->processed_files_label->setText(QString("Processed files: %1").arg(processed_files.num()));
    ui->processed_files_size_label->setText(QString("size: %1").arg(processed_files.size_readable()));

    ui->preloaded_files_label->setText(QString("Preloaded files: %1").arg(preloaded_files.num()));
    ui->preloaded_files_size_label->setText(QString("size: %1").arg(preloaded_files.size_readable()));

    ui->duplicate_files_label->setText(QString("Duplicate files: %1").arg(duplicate_files.num()));
    ui->duplicate_files_size_label->setText(QString("size: %1").arg(duplicate_files.size_readable()));

    ui->unique_files_label->setText(QString("Unique files: %1").arg(unique_files.num()));
    ui->unique_files_size_label->setText(QString("size: %1").arg(unique_files.size_readable()));

    if (etaMode == EtaMode::ENABLED) {

        bool speed_based = ui->speed_based_eta_checkbox->isChecked();

        ui->time_passed_label->setText(QString("Time passed: %1").arg(TimeUtils::timeSinceTimestamp(scan_start_time)));

        // values that represent total number of files
        FileQuantitySizeCounter all_c = total_files + duplicate_files + unique_files;
        quint64 all = speed_based ? all_c.size() : all_c.num();
        // values that represent portion of the total number of files
        // (preloaded_files is portion of duplicate_files + unique_files and processed_files is total_files)
        FileQuantitySizeCounter all_p = (preprocessed_files > processed_files ? preprocessed_files : processed_files) + preloaded_files;
        quint64 processed = speed_based ? all_p.size() : all_p.num();

        QString eta_arg = QString("Eta: %1").arg(TimeUtils::millisecondsToReadable((all - processed) / (speed_based ? averageDiskReadSpeed : averageFilesPerSecond) * 1000));

        ui->eta_label->setText(eta_arg);
        setWindowTitle(QString("Disk deduper (%1%, %2 left)").arg(processed / (double)all * 100).arg(eta_arg));
    }
}

void MainWindow::updateLoop2s() {

    // update disk usage
    auto disk_read = FileUtils::getDiskReadSizeB();

    // floating average
    averageDiskReadSpeed = averageDiskReadSpeed * 0.9 + ((disk_read - lastMeasuredDiskRead) / 2.0) * 0.1;
    lastMeasuredDiskRead = disk_read;

    qint32 currently_processed_files = preprocessed_files.num() + processed_files.num() + preloaded_files.num();
    averageFilesPerSecond = averageFilesPerSecond * 0.9 + ((currently_processed_files - previous_processed_files) / 2.0) * 0.1;
    previous_processed_files = currently_processed_files;

    ui->disk_read_label->setText(QString("Disk read: %1/s").arg(FileUtils::bytesToReadable(averageDiskReadSpeed)));
    ui->items_per_sec_label->setText(QString("Items per sec: %1/s").arg(averageFilesPerSecond));

    // update memory usage
    ui->memory_usage_label->setText(QString("Memory usage: %1").arg(FileUtils::bytesToReadable(FileUtils::getMemUsedKb() * 1024)));

}

#pragma endregion}

#pragma region MainWindow Listeners {

void MainWindow::onAddScanFolderClicked() {
    addItemsToList(callMultiDirSelectionDialogue(), ui->folders_to_scan_list);
}

void MainWindow::onSetMasterFolderClicked() {
    QString temp = FileUtils::callDirSelectionDialogue(this, "Choose master folder");
    if(!temp.isEmpty()) {
        masterFolder = temp;
        ui->master_folder_label->setText(QString("Master folder: %1").arg(masterFolder));
    }
}

void MainWindow::onSetDupesFolderClicked() {
    QString temp = FileUtils::callDirSelectionDialogue(this, "Choose dupes folder");
    if(!temp.isEmpty()) {
        dupesFolder = temp;
        ui->dupes_folder_label->setText(QString("Dupes folder: %1").arg(dupesFolder));
    }
}

void MainWindow::onExtentionCheckboxStateChanged(int state) {

    switch(state) {
        case FileUtils::DISABLED:
            ui->extention_filter_enabled_checkbox->setText("Extension filter disabled");
            break;
        case FileUtils::ENABLED_BLACK:
            ui->extention_filter_enabled_checkbox->setText("Extension filter enabled (blacklist)");
            break;
        case FileUtils::ENABLED_WHITE:
            ui->extention_filter_enabled_checkbox->setText("Extension filter enabled (whitelist)");
            break;
    }

    ui->add_extention_button->setDisabled(state == Qt::CheckState::Unchecked);
    ui->add_extention_bundle_button->setDisabled(state == Qt::CheckState::Unchecked);
    ui->extension_filter_list->setDisabled(state == Qt::CheckState::Unchecked);
}

void MainWindow::onAddExtensionButtonClicked() {
    addItemsToList(callTextDialogue("Input ext", "Extension:").replace(".", ""), ui->extension_filter_list, false, true);
}

void MainWindow::onAddExtensionBundleButtonClicked() {
    Dynamic_selection_dialog selection_dialogue(this, extension_bundles);
    selection_dialogue.setModal(true);
    int code = selection_dialogue.exec();
    auto selectedBundles = selection_dialogue.getSelected();

    if(!selectedBundles.empty() && code == QDialog::Accepted) {
        for(auto& pair: extension_bundles) {
            if(selectedBundles.contains(pair.first)) {
                addItemsToList(pair.second, ui->extension_filter_list, false, true);
            }
        }
    }
}

void MainWindow::onCurrentModeChanged(int curr_mode) {
    ui->mode_description_label->setText(scan_modes.at(curr_mode).description);
    currentMode = curr_mode;
    ui->similarity_widget->setVisible(currentMode == ScanMode::PHASH_COMPARE);
}

void MainWindow::onSimilarityChanged(int new_value) {
    ui->similarity_label->setText(QString("Similarity:%1").arg(new_value));
    currentSimilarity = new_value;
}

void MainWindow::onStartScanButtonClicked() {
    if(ui->folders_to_scan_list->count() == 0) {
        displayWarning("Nothing to scan, please add folders");
        return;
    }

    if(scan_modes.at(currentMode).request_function) {
        QString message = scan_modes.at(currentMode).request_function(this);
        if(!message.isEmpty()) {
            displayWarning(message);
            return;
        }
    }

    // reset variables
    ui->app_output_label->setText("Last app output:");
    total_files.reset();
    preprocessed_files.reset();
    processed_files.reset();
    previous_processed_files = 0;
    duplicate_files.reset();
    unique_files.reset();
    preloaded_files.reset();
    indexed_files.clear();
    master_files.clear();
    averageFilesPerSecond = 0;
    startNewLog();

    setUiDisabled(true);

    scan_start_time = QDateTime::currentMSecsSinceEpoch();
    etaMode = EtaMode::ENABLED;

    QEventLoop loop;
    QFutureWatcher<bool> futureWatcher;

    // process heavy stuff in a separate thread
    connect(&futureWatcher, SIGNAL(finished()), &loop, SLOT(quit()));
    futureWatcher.setFuture(QtConcurrent::run(this, &MainWindow::startScanAsync));
    loop.exec();

    if(!futureWatcher.result()) {
        displayWarning("Nothing to scan, your filters filter all the files");
    } else {
        // display results in main thread
        etaMode = EtaMode::DISABLED;
        if(scan_modes.at(currentMode).display_function) {
            scan_modes.at(currentMode).display_function(this);
        }
    }

    setUiDisabled(false);
    setCurrentTask("Idle");
}

#pragma endregion}

#pragma region MainWindow List operations {

void MainWindow::saveList(const QString &key, QListWidget *list) {
    QVector<FolderListItemData> dataList = getAllWidgetsDataFromList(list);
    settings.beginWriteArray(key);
    for (int i = 0; i < dataList.length(); i++) {
        settings.setArrayIndex(i);
        settings.setValue("data", QVariant::fromValue(dataList.at(i)));
    }
    settings.endArray();
}

void MainWindow::loadList(const QString &key, QListWidget *list) {
    int size = settings.beginReadArray(key);
    for (int i = 0; i < size; i++){
        settings.setArrayIndex(i);
        addWidgetToList(list, new FolderListItemWidget(this, list, settings.value("data").value<FolderListItemData>()));
    }
    settings.endArray();
}

void MainWindow::addItemsToList(QString text, QListWidget *list, bool canBlacklist, bool lowercase, bool display_warning) {

    if (text.isEmpty() || text.isNull()) {
        return;
    }

    if(lowercase) {
        text = text.toLower();
    }

    if (getFromList(text, list)) {
        if(display_warning) {
            displayWarning("Item is already in the list");
        }
        return;
    }

    auto widget = new FolderListItemWidget(this, list, canBlacklist);
    widget->setText(text);
    addWidgetToList(list, widget);
}

void MainWindow::addItemsToList(const QStringList &textList, QListWidget *list, bool canBlacklist, bool lowercase) {
    for(const auto& text: textList) {
        addItemsToList(text, list, canBlacklist, lowercase, false);
    }
}

void MainWindow::addWidgetToList(QListWidget *list, FolderListItemWidget* widget) {
    auto item = new QListWidgetItem();
    item->setSizeHint(widget->sizeHint());

    list->addItem(item);
    list->setItemWidget(item, widget);
}

QStringList MainWindow::getAllStringsFromList(QListWidget* list) {
    QStringList entries;
    for(int i = 0; i < list->count(); i++){
        entries.append(widgetFromList(list, i)->getText());
    }
    return entries;
}

QVector<FolderListItemData> MainWindow::getAllWidgetsDataFromList(QListWidget* list) {
    QVector<FolderListItemData> entries;
    for(int i = 0; i < list->count(); i++){
        entries.append(widgetFromList(list, i)->getData());
    }
    return entries;
}


QListWidgetItem* MainWindow::getFromList(const QString& text, QListWidget* list) {
    for(int i = 0; i < list->count(); i++){
        auto item = list->item(i);
        auto itemWidget = widgetFromList(list, i);
        if(itemWidget->getText() == text) {
            return item;
        }
    }
    return nullptr;
}

void MainWindow::removeItemFromList(const QString& text, QListWidget* list) {
    auto item = getFromList(text, list);
    if(item) {
        delete item;
    } else {
        qWarning("Failed deleting item from list");
    }
}

void MainWindow::setListItemsDisabled(QListWidget* list, bool disable) {
    for(int i = 0; i < list->count(); i++){
        widgetFromList(list, i)->setDisabled(disable);
    }
}

#pragma endregion}

#pragma region MainWindow utility functions {

FolderListItemWidget* MainWindow::widgetFromList(QListWidget* list, int index) {
    return dynamic_cast<FolderListItemWidget*>(list->itemWidget(list->item(index)));
}

QStringList MainWindow::callMultiDirSelectionDialogue() {

    QFileDialog w;
    w.setOption(QFileDialog::DontUseNativeDialog, true);
    w.setFileMode(QFileDialog::DirectoryOnly);
    QListView *l = w.findChild<QListView*>("listView");
    if (l) {
         l->setSelectionMode(QAbstractItemView::MultiSelection);
    }
    QTreeView *t = w.findChild<QTreeView*>();
    if (t) {
       t->setSelectionMode(QAbstractItemView::MultiSelection);
    }

    if(w.exec() == QDialog::Accepted){
        return w.selectedFiles();
    } else {
        return {};
    }
}

QString MainWindow::callTextDialogue(const QString &title, const QString &prompt) {
    bool ok;
    QString text = QInputDialog::getText(this, title, prompt, QLineEdit::Normal, "", &ok);
    return ok ? text : "";
}

void MainWindow::displayWarning(const QString &message) {
    QMessageBox::warning(this, "Warning", message);
}

void MainWindow::setCurrentTask(const QString &status) {
    if(QThread::currentThread() == thread()) {
        ui->current_task_label->setText(QString("Current task: %1").arg(status));
    } else {
        MOVE_TO_UI_THREAD("setCurrentTask", Q_ARG(QString, status));
    }
}

void MainWindow::appendToLog(const QString &msg, bool log_to_ui, LogLevel log_level) {
    if(!this_window) {
        return;
    }
    if(QThread::currentThread() == this_window->thread()) {
        if(log_to_ui) {
            this_window->ui->app_output_label->setText(QString("Last app output: %1").arg(msg));
        }
        if(log_level >= logLevel && currentLogFileStream.device()) {
            currentLogFileStream << QDateTime::currentDateTime().toString("hh:mm:ss | ");
            currentLogFileStream << msg << "<br>" << Qt::endl;
        }
    } else {
        MOVE_TO_UI_THREAD("appendToLog", Q_ARG(QString, msg), Q_ARG(bool, log_to_ui), Q_ARG(LogLevel, log_level));
    }
}

void MainWindow::setUiDisabled(bool disabled) {

    setListItemsDisabled(ui->extension_filter_list, disabled);
    setListItemsDisabled(ui->folders_to_scan_list, disabled);

    ui->mode_combo_box->setDisabled(disabled);

    ui->add_extention_button->setDisabled(disabled);
    ui->add_scan_folder_button->setDisabled(disabled);

    ui->set_master_folder_button->setDisabled(disabled);
    ui->set_dupes_folder_button->setDisabled(disabled);

    ui->start_scan_button->setDisabled(disabled);
    ui->similarity_slider->setDisabled(disabled);

    ui->extention_filter_enabled_checkbox->setDisabled(disabled);
    ui->add_extention_button->setDisabled(ui->extention_filter_enabled_checkbox->checkState() == Qt::CheckState::Unchecked ? true : disabled);
}

void MainWindow::startNewLog() {
    QString formattedTime = QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss");
    currentLogFile.close();
    currentLogFile.setFileName(QString("./logs/log_%1.html").arg(formattedTime));
    currentLogFile.open(QIODevice::WriteOnly);
    currentLogFileStream << "<style> body { background: #282828; color: white } </style>";
}

#pragma endregion}

#pragma region MainWindow duper functions {

template<typename T>
void MainWindow::removeDuplicates(T &arr) {
    std::sort(arr.begin(), arr.end());
    arr.erase(std::unique(arr.begin(), arr.end()), arr.end());
}

bool MainWindow::startScanAsync() {

    QStringList blacklisted_dirs;

    extension_filter_state = (FileUtils::ExtenstionFilterState)ui->extention_filter_enabled_checkbox->checkState();

    for(int i = 0; i < ui->folders_to_scan_list->count(); i++) {
        auto itemWidget = widgetFromList(ui->folders_to_scan_list, i);
        if(!itemWidget->isWhitelisted()) {
            blacklisted_dirs.append(itemWidget->getText());
        }
    }

    if(extension_filter_state != FileUtils::DISABLED) {
        listed_exts = getAllStringsFromList(ui->extension_filter_list);
    }

    for(int i = 0; i < ui->folders_to_scan_list->count(); i++){
        auto itemWidget = widgetFromList(ui->folders_to_scan_list, i);
        if(itemWidget->isWhitelisted()) {
            walkDir(itemWidget->getText(), blacklisted_dirs, listed_exts,
                    extension_filter_state,
                    [this](QString file) {addEnumeratedFile(file, indexed_files);});
        }
    }

    quint64 size = 0;
    for(auto& file: indexed_files) {
        size += file.size_bytes;
    }
    qInfo() << FileUtils::bytesToReadable(size);

    // remove duplicates
    removeDuplicates(indexed_files);

    if(indexed_files.empty()) {
        return false;
    }

    // recalculate after removing duplicates
    total_files.reset();
    for(auto& file: indexed_files) {
        total_files += file;
    }

    // open a connection from this thread
    QSqlDatabase storage_db = DbUtils::openDbConnection();
    storage_db.transaction();
    qInfo() << "Started db transaction";

    scan_modes.at(currentMode).process_function(this, storage_db);

    storage_db.commit();
    storage_db.close();
    qInfo() << "Db commit";
    return true;
}

void MainWindow::addEnumeratedFile(const QString& file, MultiFile& files) {
    QFile file_r = file;
    if(files.size() % 100 == 0) {
        setCurrentTask(QString("Enumerating file: %1").arg(file));
    }
    total_files += file_r;
    files.push_back({file});
}

void MainWindow::hashAllFiles(QSqlDatabase db, MultiFile& files, File::HashType hash_type, const std::function<void(File&)>& callback) {
    // threaded hash loading
    QVector<QFuture<void>> futures;
    for (auto& file: files) {
        setCurrentTask(QString("Hashing file: %1").arg(file));
        futures.append(file.loadHash(db, hash_type));
    }
    // save to db if the hash was computed
    for(int i = 0; i < files.size(); i++) {
        if(!futures[i].isCanceled()) {
            futures[i].waitForFinished();
            files[i].saveHashToDb(db);
        }
        setCurrentTask(QString("Hashed file: %1").arg(files[i]));
        callback(files[i]);
        (hash_type == File::PARTIAL ? preprocessed_files : processed_files) += files[i];
    }
}

void MainWindow::loadAllMetadataFromFiles(QSqlDatabase db, MultiFile& files_all, const std::function<bool (File &)> &callback) {

    MultiFile files;
    for(auto& file: files_all) {
        // check if we need to get metadata using exiftool
        setCurrentTask(QString("Checking db data of file: %1").arg(file));
        if(!file.loadMetadataFromDb(db)) {
            files.append(file);
        } else {
            preprocessed_files += file;
            processed_files += file;
            if(!callback(file)) {
                break;
            }
        }
    }

    // split into chunks for multithreaded processing

    QVector<QFuture<void>> futures;
    if(files.size() / idealThreadCount > 100) {
        for(int offset = 0; offset < ex_tools.size(); offset++) {
            futures.append(QtConcurrent::run([&files, this, offset]() {
                for(int i = offset; i < files.size(); i+= idealThreadCount) {
                    setCurrentTask(QString("Getting info about file: %1").arg(files[i]));
                    files[i].loadMetadataFromExifTool(ex_tools[offset]);
                    preprocessed_files += files[i];
                }
            }));
        }
    } else {
        for(auto& file: files) {
            setCurrentTask(QString("Getting info about file: %1").arg(file));
            file.loadMetadataFromExifTool(ex_tools[0]);
            preprocessed_files += file;
        }
    }

    // save to db if the metadata was loaded
    for(auto& future: futures) {
        future.waitForFinished();
    }

    for(auto& file: files) {
        setCurrentTask(QString("Got info about file: %1").arg(file));
        file.saveMetadataToDb(db);
        if(!callback(file)) {
            break;
        }
        processed_files += file;
    }
}

void MainWindow::loadAllThumbnailsFromFiles(QSqlDatabase db, MultiFile& files) {
    // threaded thumbnail genration
    QVector<QFuture<void>> futures;
    for (auto& file: files) {
        setCurrentTask(QString("Getting preview for file: %1").arg(file));
        futures.append(file.loadThumbnail(db));
    }
    // save to db if the thumbnail was loaded
    for(int i = 0; i < files.size(); i++) {
        if(!futures.at(i).isCanceled()) {
            futures[i].waitForFinished();
            files[i].postLoadThumbnail();
            files[i].saveThumbnailToDb(db);
        };
        setCurrentTask(QString("Got preview for file: %1").arg(files[i]));
        preloaded_files += files[i];
    }
}


template<FileField field>
void MainWindow::findDuplicateFiles(QSqlDatabase db) {

    MultiFile indexed_files_filtered;
    if constexpr(field == FileField::HASH) {

        // map of groups of files with the same partial hash
        QMap<QByteArray, MultiFile> probably_duplicate_files_map;
        hashAllFiles(db, indexed_files, File::PARTIAL, [&probably_duplicate_files_map](File& file) {
            probably_duplicate_files_map[file.partial_hash].append(file);
        });

        for(auto& fileGroup: probably_duplicate_files_map) {
            if(fileGroup.size() > 1) {
                indexed_files_filtered.append(fileGroup);
            }
        }

        hashAllFiles(db, indexed_files_filtered, File::FULL);
    } else {
        indexed_files_filtered = indexed_files;
    }

    if constexpr(field == FileField::PHASH) {
        hashAllFiles(db, indexed_files_filtered, File::PERCEPTUAL);
    }

    // map of groups of files with the same key field (name or hash)
    QMap<QByteArray, MultiFile> duplicate_files_map;

    // we group all files with the same key field
    for(auto& file: indexed_files_filtered) {

        QByteArray value;
        if constexpr(field == FileField::NAME) {
            value = file.name.toLower().toUtf8();
        } else if constexpr(field == FileField::HASH) {
            value = file.hash;
        } else if constexpr(field == FileField::PHASH) {
            // no perceptual hash for file (file is not an image / video)
            if(file.perceptual_hash.isEmpty()) {
                continue;
            }
        }

        // normal equals comparison
        if constexpr(field == FileField::NAME || field == FileField::HASH) {
            setCurrentTask(QString("Comparing: %1").arg(file));
            if(duplicate_files_map.contains(value)) {
                // we have our first hit, this means that the first file is unique
                if(duplicate_files_map[value].size() == 1) {
                    unique_files += file;
                }
                duplicate_files += file;
            }
            duplicate_files_map[value].append(file);
        // perceptual hashes need to be compared differently
        } else {
            const auto unique_hashes = duplicate_files_map.keys();
            bool found_similar = false;
            for(const auto& uhash: unique_hashes) {
                if(FileUtils::comparePerceptualHashes(file.perceptual_hash, uhash, currentSimilarity)) {
                    duplicate_files_map[uhash].append(file);
                    duplicate_files += file;
                    found_similar = true;
                    break;
                }
            }
            if(!found_similar) {
                unique_files += file;
                duplicate_files_map[file.perceptual_hash].append(file);
            }
        }
    }

    // map of fingerprint-to-multiFile, fingerprint will be the same in two groups if files in 2 groups group have the same dupe locations

    // example
    // group1: /testdir/image1.png, /testdir2/image.png, /testdir3/image3.png
    // group2: /testdir3/image1.png, /testdir/image.png, /testdir2/image3.png
    // fingerprint will be the same

    QMap<QByteArray, MultiFileGroup> multiFiles_fingerprintMatched;

    // we group all groups bigger than 1 (at least one duplicate) with the same fingerprint
    for(auto& multiFile: duplicate_files_map) {
        if(multiFile.size() > 1) {
            // load thumbnails while we are here
            loadAllThumbnailsFromFiles(db, multiFile);
            multiFiles_fingerprintMatched[FileUtils::getFileGroupFingerprint(multiFile)].append(multiFile);
        }
    }

    // transformed map

    // input (dedupe_resuts_intermediate)
    //QList(
    //QVector(
    //    QVector("/home/kloud/Downloads/test1/pict1___.png", "/home/kloud/Downloads/test2/pict1___.png"),
    //    QVector("/home/kloud/Downloads/test1/pict03d.png", "/home/kloud/Downloads/test2/pict03d.png"),
    //    QVector("/home/kloud/Downloads/test1/pict2--.png", "/home/kloud/Downloads/test2/pict2--.png"),
    //    QVector("/home/kloud/Downloads/test1/pict33.png", "/home/kloud/Downloads/test2/pict33.png"),
    //    QVector("/home/kloud/Downloads/test1/pict0__.png", "/home/kloud/Downloads/test2/pict0__.png")
    //)
    //)
    // output (dedupe_resuts)
    //QList(
    //QVector(
    //    QVector("/home/kloud/Downloads/test1/pict1___.png", "/home/kloud/Downloads/test1/pict03d.png", "/home/kloud/Downloads/test1/pict2--.png", "/home/kloud/Downloads/test1/pict33.png", "/home/kloud/Downloads/test1/pict0__.png"),
    //    QVector( "/home/kloud/Downloads/test2/pict1___.png", "/home/kloud/Downloads/test2/pict03d.png", "/home/kloud/Downloads/test2/pict2--.png", "/home/kloud/Downloads/test2/pict33.png", "/home/kloud/Downloads/test2/pict0__.png")
    //)
    //)

    const QList<MultiFileGroup> dedupe_resuts_intermediate = multiFiles_fingerprintMatched.values();

    dedupe_resuts.clear();
    for(const auto& group_of_groups: dedupe_resuts_intermediate) {

        MultiFileGroup rotated_vector;

        for(const auto& group: group_of_groups) {
            for(int i = 0; i < group.size(); i++) {
                if(rotated_vector.size() > i){
                    rotated_vector[i].append(group.at(i));
                } else {
                    rotated_vector.append({group.at(i)});
                }
            }
        }

        dedupe_resuts.append(rotated_vector);
    }
}

void MainWindow::hashCompare(QSqlDatabase db) {
    findDuplicateFiles<FileField::HASH>(db);
}

void MainWindow::phashCompare(QSqlDatabase db) {
    findDuplicateFiles<FileField::PHASH>(db);
}

void MainWindow::nameCompare(QSqlDatabase db) {
    findDuplicateFiles<FileField::NAME>(db);
}

void MainWindow::autoDedupe_move(QSqlDatabase db) {
    autoDedupe(db, false);
}

void MainWindow::autoDedupe_rename(QSqlDatabase db) {
    autoDedupe(db, true);
}

void MainWindow::autoDedupe(QSqlDatabase db, bool safe) {
    walkDir(masterFolder, {}, listed_exts,
                        extension_filter_state,
                        [this](QString file) {addEnumeratedFile(file, master_files);});

    hashAllFiles(db, master_files, File::FULL);

    QMap<QString, File> master_hashes;
    QSet<QString> master_hashes_hit;
    MultiFile dupes;

    for(auto& master_file: master_files) {
        master_hashes[master_file.hash] = master_file;
    }

    hashAllFiles(db, indexed_files, File::FULL,
    [this, &master_hashes, &dupes, &master_hashes_hit](const File& file) {
        setCurrentTask(QString("Comparing file: %1").arg(file));
        if(master_hashes.contains(file.hash)) {
            duplicate_files += file;
            dupes.append(file);
            if (!master_hashes_hit.contains(file.hash)){
                // our first hit, increment the number on unique files
                unique_files += file;
                master_hashes_hit.insert(file.hash);
            }
        }
    });

    autoDedupe_files = FileUtils::queueFilesToModify(dupes, safe ? "" : dupesFolder, safe ? "_DELETED_" : "");
}

void MainWindow::exifRename(QSqlDatabase db) {
    loadAllMetadataFromFiles(db, indexed_files,
    [this](File& file){
        setCurrentTask(QString("Renaming file: %1").arg(file));
        return exifRenameFormat.rename(file);
    });
}

void MainWindow::showStats(QSqlDatabase db) {

    QVector<NamedCountableQStringList> meta_fields_stats;

    for (auto& metadata_key: selectedMetaFields) {
        meta_fields_stats.append({metadata_key, {}});
    }

    loadAllMetadataFromFiles(db, indexed_files,
    [&meta_fields_stats](const File& file){

        // iterate through name-array pairs
        for (auto& [metadata_key, metadata_array]: meta_fields_stats) {
            const QString& metadata_value = file.metadata.value(metadata_key);

            // if value is already present (for instance extension "png") add to it, othrewise construct a new one
            if (metadata_array.contains(metadata_value)) {
                CountableQString& temp = metadata_array[metadata_array.indexOf(metadata_value)];
                temp.count ++;
                temp.total_size_bytes += file.size_bytes;
            } else {
                metadata_array.append({metadata_value, 1, file.size_bytes});
            }
        }

        return true;
    });

    // calculate relative percentages for all meta fileds
    for (auto& [metadata_key, metadata_array]: meta_fields_stats) {
        for (auto& metadata_value: metadata_array) {
            metadata_value.count_percentage = metadata_value.count / (double)total_files.num() * 100;
            metadata_value.size_percentage = metadata_value.total_size_bytes / (long double)total_files.size() * 100;
        }
    }

    stat_results = {meta_fields_stats, total_files };
}

void MainWindow::fileCompare_display() {
    if(dedupe_resuts.empty()) {
       displayWarning("No dupes found");
       return;
    }
    Dupe_results_dialog dupe_results_dialog(this, dedupe_resuts, duplicate_files);
    dupe_results_dialog.setModal(true);
    dupe_results_dialog.exec();
}

QString MainWindow::showStats_request() {
    Dynamic_selection_dialog selection_dialogue(this, getMetaFieldsList());
    selection_dialogue.setModal(true);
    int code = selection_dialogue.exec();
    selectedMetaFields = selection_dialogue.getSelected();
    if(!selectedMetaFields.empty() && code == QDialog::Accepted) {
        return "";
    }
    return "Nothing selected";
}

QString MainWindow::exifRename_request() {
    Exif_rename_builder_dialog exif_rename_builder_dialog(this);
    exif_rename_builder_dialog.setModal(true);
    int code = exif_rename_builder_dialog.exec();
    exifRenameFormat = exif_rename_builder_dialog.getFormat();
    if(code != QDialog::Accepted) {
        return "Nothing to do";
    }
    if(!exifRenameFormat.isValid()) {
        return "Template invalid";
    }
    return "";
}


QString MainWindow::autoDedupe_request() {
    if(masterFolder.isEmpty()) {
        return "No master folder specified, can't proceed";
    }
    if(dupesFolder.isEmpty()) {
        return "No duplicate folder specified, can't proceed";
    }
    if(!QDir(masterFolder).exists()) {
        return "Master folder doesn't exist, can't proceed";
    }
    if(!QDir(dupesFolder).exists()) {
        return "Duplicate folder doesn't exist, can't proceed";
    }
    return "";
}

void MainWindow::autoDedupe_display() {

    MoveConfirmationDialog confirmation_dialog(this, autoDedupe_files);

    confirmation_dialog.setModal(true);
    if(confirmation_dialog.exec() == QDialog::Accepted) {
        FileUtils::deleteOrRenameFiles(autoDedupe_files,
        [this](const QString& status) {
            setCurrentTask(status);
            qInfo() << status;
        }, true);
    };
}

void MainWindow::showStats_display() {
    Stats_dialog stats_dialog(this, stat_results);
    stats_dialog.setModal(true);
    stats_dialog.exec();
}

#pragma endregion}
