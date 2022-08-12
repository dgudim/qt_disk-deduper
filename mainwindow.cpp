#include "mainwindow.h"
#include "ui_mainwindow.h"

enum PiechartIndex {
    ALL_FILES = 0,
    SCANNED_FILES = 1,
    DUPLICATE_FILES = 2
};

QVector<QPair<QString, QColor>> piechart_config = {
    {"A: %1", QColor(1, 184, 170)},
    {"Sc: %1", QColor(3, 171, 51)},
    {"Dup: %1", QColor(181, 113, 4)}};

enum ScanMode {
    HASH_COMPARE = 0,
    NAME_COMPARE = 1,
    AUTO_DEDUPE_MOVE = 2,
    AUTO_DEDUPE_RENAME = 3,
    EXIF_RENAME = 4,
    SHOW_STATS = 5
};

struct ScanModeProperties {
    QString name;
    QString description;
    std::function<void(MainWindow*)> process_function;
    std::function<void(MainWindow*)> display_function;
    std::function<bool(MainWindow*)> request_function;
};

QList<ScanModeProperties> scan_modes = {
    {"Hash duplicates", "Compare files by hash and show results in groups for further action", &MainWindow::hashCompare, &MainWindow::hashCompare_display, nullptr},
    {"Name duplicates", "Compare files by name and show results in groups for further action", &MainWindow::nameCompare, &MainWindow::nameCompare_display, nullptr},
    {"Auto dedupe(move)", "Compare master folder and slave folders by hash (Files from the slave folders are moved into the dupes folder if they are present in the master folder)", &MainWindow::autoDedupeMove, &MainWindow::autoDedupeMove_display, nullptr},
    {"Auto dedupe(rename)", "Compare master folder and slave folders by hash (DELETED_ is added to the name of a file from the slave folders if it is present in the master folder)", &MainWindow::autoDedupeRename, &MainWindow::autoDedupeRename_display, nullptr},
    {"EXIF rename", "Rename files according to their EXIF data (Name format: <creation date and time>_<camera model>_numbers from file name)", &MainWindow::exifRename, &MainWindow::exifRename_display, nullptr},
    {"Show statistics", "Get statistics of selected folders (Extensions, camera models) and display them", &MainWindow::showStats, &MainWindow::showStats_display, &MainWindow::showStats_request}};

#define MOVE_TO_UI_THREAD(func, ...) QMetaObject::invokeMethod(this_window, func, Qt::QueuedConnection, __VA_ARGS__);

MainWindow *MainWindow::this_window = 0;

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow) {

    ui->setupUi(this);

    ex_tool = new ExifTool();
    this_window = this;

    // setup initial values
    program_start_time = QDateTime::currentMSecsSinceEpoch();
    lastMeasuredDiskRead = getDiskReadSizeB();

    setCurrentTask("Idle");

    QStringList modeNames;
    for(auto& [mode_name, mode_description, pfunc, dfunc, rfunc]: scan_modes){
        modeNames.push_back(mode_name);
    }
    ui->mode_combo_box->insertItems(0, modeNames);
    ui->mode_combo_box->setCurrentIndex(ScanMode::HASH_COMPARE);
    onCurrentModeChanged(ScanMode::HASH_COMPARE);

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

    // setup ui listeners
    connect(ui->add_scan_folder_button, SIGNAL(clicked()), this, SLOT(onAddScanFolderClicked()));
    connect(ui->add_slave_folder_button, SIGNAL(clicked()), this, SLOT(onAddSlaveFolderClicked()));

    connect(ui->set_master_folder_button, SIGNAL(clicked()), this, SLOT(onSetMasterFolderClicked()));
    connect(ui->set_dupes_folder_button, SIGNAL(clicked()), this, SLOT(onSetDupesFolderClicked()));

    connect(ui->add_extention_button, SIGNAL(clicked()), this, SLOT(onAddExtensionButtonClicked()));

    connect(ui->mode_combo_box, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentModeChanged(int)));

    ui->extention_filter_enabled_checkbox->setCheckState(Qt::Unchecked);
    onExtentionCheckboxStateChanged(Qt::Unchecked);
    connect(ui->extention_filter_enabled_checkbox, SIGNAL(stateChanged(int)), this, SLOT(onExtentionCheckboxStateChanged(int)));

    connect(ui->start_scan_button, SIGNAL(clicked()), this, SLOT(onStartScanButtonClicked()));
}

MainWindow::~MainWindow() {
  delete ex_tool;
  delete ui;
}

#pragma region MainWindow loops {

void MainWindow::updateLoop100Ms() {
     // update uptime
    ui->uptime_label->setText(QString("Uptime: %1").arg(timeSinceTimestamp(program_start_time)));

    // update piechart
    series->slices().at(PiechartIndex::ALL_FILES)->setValue(unique_files.length() - processed_files);
    series->slices().at(PiechartIndex::SCANNED_FILES)->setValue(processed_files);
    for (int i = 0; i < series->count(); i++){
        auto slice = series->slices().at(i);
        slice->setLabel(QString(piechart_config[i].first).arg(slice->value()));
    }

    if (etaMode != OFF) {
        ui->time_passed_label->setText(QString("Time passed: %1").arg(timeSinceTimestamp(scan_start_time)));

        float averagePerSec = etaMode == SPEED_BASED ? averageDiskReadSpeed : averageFilesPerSecond;

        if (averagePerSec == 0){
           ui->eta_label->setText("Eta: ∞");
        } else {
            quint64 left = etaMode == SPEED_BASED ? (files_size_all - files_size_processed) : (unique_files.length() - processed_files);
            ui->eta_label->setText(QString("Eta: %1").arg(millisecondsToReadable(left / averagePerSec * 1000)));
        }
    }
}

void MainWindow::updateLoop2s() {

    // update disk usage
    auto disk_read = getDiskReadSizeB();

    // floating average
    averageDiskReadSpeed = averageDiskReadSpeed * 0.9 + ((disk_read - lastMeasuredDiskRead) / 2.0) * 0.1;
    lastMeasuredDiskRead = disk_read;

    averageFilesPerSecond = averageFilesPerSecond * 0.9 + ((processed_files - previous_processed_files) / 2.0) * 0.1;
    previous_processed_files = processed_files;

    ui->disk_read_label->setText(QString("Disk read: %1/s").arg(bytesToReadable(averageDiskReadSpeed)));
    ui->items_per_sec_label->setText(QString("Items per sec: %1/s").arg(averageFilesPerSecond));

    // update memory usage
    ui->memory_usage_label->setText(QString("Memory usage: %1").arg(bytesToReadable(getMemUsedKb() * 1024)));

}

#pragma endregion}

#pragma region MainWindow Listeners {

void MainWindow::onAddScanFolderClicked() {
    addItemsToList(callMultiDirSelectionDialogue(), ui->folders_to_scan_list);
}

void MainWindow::onAddSlaveFolderClicked() {
    addItemsToList(callMultiDirSelectionDialogue(), ui->slave_folders_list);
}

void MainWindow::onSetMasterFolderClicked() {
    masterFolder = callDirSelectionDialogue("Choose master folder");
    ui->master_folder_label->setText(QString("Master folder: %1").arg(masterFolder));
}

void MainWindow::onSetDupesFolderClicked() {
    dupesFolder = callDirSelectionDialogue("Choose master folder");
    ui->dupes_folder_label->setText(QString("Dupes folder: %1").arg(dupesFolder));
}

void MainWindow::onExtentionCheckboxStateChanged(int state) {

    switch(state) {
        case ExtenstionFilterState::DISABLED:
            ui->extention_filter_enabled_checkbox->setText("Extension filter disabled");
            break;
        case ExtenstionFilterState::ENABLED_BLACK:
            ui->extention_filter_enabled_checkbox->setText("Extension filter enabled (blacklist)");
            break;
        case ExtenstionFilterState::ENABLED_WHITE:
            ui->extention_filter_enabled_checkbox->setText("Extension filter enabled (whitelist)");
            break;
    }

    ui->add_extention_button->setDisabled(state == Qt::CheckState::Unchecked);
    ui->extension_filter_list->setDisabled(state == Qt::CheckState::Unchecked);
}

void MainWindow::onAddExtensionButtonClicked() {
    addItemsToList(callTextDialogue("Input ext", "Extension:").replace(".", ""), ui->extension_filter_list, false, true);
}

void MainWindow::onCurrentModeChanged(int curr_mode) {
    ui->mode_description_label->setText(scan_modes.at(curr_mode).description);
    currentMode = curr_mode;
}

void MainWindow::onStartScanButtonClicked() {
    if(ui->folders_to_scan_list->count() == 0) {
        displayWarning("Nothing to scan, please add folders");
        return;
    }

    if(scan_modes.at(currentMode).request_function && !scan_modes.at(currentMode).request_function(this)) {
        displayWarning("Invalid configuration, will not do anyting");
        return;
    }

    // reset variables
    files_size_all = 0;
    files_size_processed = 0;
    processed_files = 0;
    previous_processed_files = 0;
    unique_files.clear();
    averageFilesPerSecond = 0;

    setUiDisabled(true);

    scan_start_time = QDateTime::currentMSecsSinceEpoch();

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
        scan_modes.at(currentMode).display_function(this);
    }

    setUiDisabled(false);
    etaMode = OFF;
    setCurrentTask("Idle");
}

#pragma endregion}

#pragma region MainWindow List operations {

void MainWindow::addItemsToList(QString text, QListWidget *list, bool canBlacklist, bool lowercase) {

    if (text.isEmpty() || text.isNull()) {
        return;
    }

    if(lowercase) {
        text = text.toLower();
    }

    if (getFromList(text, list)) {
        displayWarning("Item is already in the list");
        return;
    }

    auto item = new QListWidgetItem();
    auto widget = new FolderListItemWidget(this, list, canBlacklist);

    widget->setText(text);
    item->setSizeHint(widget->sizeHint());

    list->addItem(item);
    list->setItemWidget(item, widget);
}

void MainWindow::addItemsToList(const QStringList &textList, QListWidget *list, bool canBlacklist, bool lowercase) {
    for(const auto& text: textList) {
        addItemsToList(text, list, canBlacklist, lowercase);
    }
}

QListWidgetItem* MainWindow::getFromList(const QString& text, QListWidget* list) {
    for(int i = 0; i < list->count(); i++){
        auto item = list->item(i);
        auto itemWidget = widgetFromWidgetItem(item);
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
        auto item = list->item(i);
        auto itemWidget = widgetFromWidgetItem(item);
        itemWidget->setDisabled(disable);
    }
}

#pragma endregion}

#pragma region MainWindow utility functions {

FolderListItemWidget* MainWindow::widgetFromWidgetItem(QListWidgetItem *item) {
    return dynamic_cast<FolderListItemWidget*>(item->listWidget()->itemWidget(item));
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

    w.exec();

    return w.selectedFiles();
}

QString MainWindow::callDirSelectionDialogue(const QString &title) {
    return QFileDialog::getOpenFileName(this, title, "", "", nullptr, QFileDialog::Options(QFileDialog::ShowDirsOnly));
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

void MainWindow::appendToLog(const QString &msg, bool error) {
    if(QThread::currentThread() == this_window->thread()) {
        if(error) {
            this_window->ui->log_text->insertHtml(msg);
            this_window->ui->log_text->insertHtml("<br>");
        }
    } else {
        MOVE_TO_UI_THREAD("appendToLog", Q_ARG(QString, msg), Q_ARG(bool, error));
    }
};

void MainWindow::setUiDisabled(bool disabled) {

    setListItemsDisabled(ui->extension_filter_list, disabled);
    setListItemsDisabled(ui->slave_folders_list, disabled);
    setListItemsDisabled(ui->folders_to_scan_list, disabled);

    ui->mode_combo_box->setDisabled(disabled);

    ui->add_extention_button->setDisabled(disabled);
    ui->add_scan_folder_button->setDisabled(disabled);
    ui->add_slave_folder_button->setDisabled(disabled);

    ui->set_master_folder_button->setDisabled(disabled);
    ui->set_dupes_folder_button->setDisabled(disabled);

    ui->start_scan_button->setDisabled(disabled);
    ui->extention_filter_enabled_checkbox->setDisabled(disabled);
    ui->add_extention_button->setDisabled(ui->extention_filter_enabled_checkbox->checkState() == Qt::CheckState::Unchecked ? true : disabled);
};

#pragma endregion}

#pragma region MainWindow duper functions {

template<typename T>
void MainWindow::removeDuplicates(T &arr) {
    std::sort(arr.begin(), arr.end());
    arr.erase(std::unique(arr.begin(), arr.end()), arr.end());
}

bool MainWindow::startScanAsync() {

    QStringList blacklisted_dirs;

    ExtenstionFilterState extFilterState = (ExtenstionFilterState)ui->extention_filter_enabled_checkbox->checkState();
    QStringList listed_exts;

    for(int i = 0; i < ui->folders_to_scan_list->count(); i++) {
        auto itemWidget = widgetFromWidgetItem(ui->folders_to_scan_list->item(i));
        if(!itemWidget->isWhitelisted()) {
            blacklisted_dirs.append(itemWidget->getText());
        }
    }

    if(extFilterState != ExtenstionFilterState::DISABLED) {
        for(int i = 0; i < ui->extension_filter_list->count(); i++) {
            auto itemWidget = widgetFromWidgetItem(ui->extension_filter_list->item(i));
            listed_exts.append(itemWidget->getText());
        }
    }

    for(int i = 0; i < ui->folders_to_scan_list->count(); i++){
        auto itemWidget = widgetFromWidgetItem(ui->folders_to_scan_list->item(i));

        if(itemWidget->isWhitelisted()) {
            walkDir(itemWidget->getText(), blacklisted_dirs, listed_exts,
                    extFilterState,
                    [this](QString file) {addEnumeratedFile(file);});
        }
    }

    // remove duplicates
    removeDuplicates(unique_files);

    if(unique_files.empty()) {
        return false;
    }

    for(auto& file: unique_files) {
        files_size_all += QFile(file.full_path).size();
    }

    scan_modes.at(currentMode).process_function(this);
    return true;
}

void MainWindow::addEnumeratedFile(const QString& file) {
    QFile file_r = file;
    QFileInfo fileInfo(file);
    if(unique_files.size() % 100 == 0) {
        setCurrentTask(QString("Enumerating file: %1").arg(file));
    }
    unique_files.push_back({file, fileInfo.fileName(), fileInfo.suffix().toLower(), file_r.size()});
};

void MainWindow::hashAllFiles() {
     for (auto& u_file: unique_files){
         processed_files ++;
         files_size_processed += u_file.size_bytes;
         setCurrentTask(QString("Hashing file: %1").arg(u_file.full_path));
         u_file.computeHash();
     }
}

void MainWindow::hashCompare() {
    hashAllFiles();
}

void MainWindow::nameCompare() {

}

void MainWindow::autoDedupeMove() {
    hashAllFiles();
}

void MainWindow::autoDedupeRename() {
    hashAllFiles();
}

void MainWindow::exifRename() {

}

void MainWindow::showStats() {

    etaMode = ITEM_BASED;

    QVector<QPair<QString, QVector<Countable_qstring>>> meta_fields_stats;

    if (!unique_files.empty()) {
        for (auto& metadata_key: selectedMetaFields) {
            meta_fields_stats.append({metadata_key, {}});
        }
    }

    for(auto& file: unique_files) {

        setCurrentTask(QString("Gathering information about: %1").arg(file.full_path));

        // load metadata
        file.loadMetadata(ex_tool, selectedMetaFields);

        // iterate through name-array pairs
        for (auto& [metadata_key, metadata_array]: meta_fields_stats) {
            QString& metadata_value = file.metadata[metadata_key];

            // if value is already present (for instance extension "png") add to it, othrewise append
            if (metadata_array.contains(metadata_value)) {
                Countable_qstring& temp = metadata_array[metadata_array.indexOf(metadata_value)];
                temp.count ++;
                temp.total_size_bytes += file.size_bytes;
            } else {
                metadata_array.append({metadata_value, 1, file.size_bytes});
            }
        }

        processed_files ++;
        files_size_processed += file.size_bytes;
    }

    // calculate relative percentages for all meta fileds
    for (auto& [metadata_key, metadata_array]: meta_fields_stats) {
        for (auto& metadata_value: metadata_array) {
            metadata_value.count_percentage = metadata_value.count / (double)unique_files.length() * 100;
            metadata_value.size_percentage = metadata_value.total_size_bytes / (long double)files_size_all * 100;
        }
    }

    stat_results = {meta_fields_stats, unique_files.length(), files_size_all};
}

void MainWindow::hashCompare_display() {

}

void MainWindow::nameCompare_display() {

}

void MainWindow::autoDedupeMove_display() {

}

void MainWindow::autoDedupeRename_display() {

}

void MainWindow::exifRename_display() {

}

bool MainWindow::showStats_request() {
    selection_dialogue = new Metadata_selection_dialogue(this);
    selection_dialogue->setModal(true);
    selection_dialogue->exec();
    selectedMetaFields = selection_dialogue->getSelected();
    return !selectedMetaFields.empty();
}

void MainWindow::showStats_display() {
    stats_dialog = new Stats_dialog(this, stat_results);
    stats_dialog->setModal(true);
    hide();
    stats_dialog->show();
}

#pragma endregion}
