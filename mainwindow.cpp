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
};

QList<ScanModeProperties> scan_modes = {
    {"Hash duplicates", "Compare files by hash and show results in groups for further action", &MainWindow::hashCompare, &MainWindow::hashCompare_display},
    {"Name duplicates", "Compare files by name and show results in groups for further action", &MainWindow::nameCompare, &MainWindow::nameCompare_display},
    {"Auto dedupe(move)", "Compare master folder and slave folders by hash (Files from the slave folders are moved into the dupes folder if they are present in the master folder)", &MainWindow::autoDedupeMove, &MainWindow::autoDedupeMove_display},
    {"Auto dedupe(rename)", "Compare master folder and slave folders by hash (DELETED_ is added to the name of a file from the slave folders if it is present in the master folder)", &MainWindow::autoDedupeRename, &MainWindow::autoDedupeRename_display},
    {"EXIF rename", "Rename files according to their EXIF data (Name format: <creation date and time>_<camera model>_numbers from file name)", &MainWindow::exifRename, &MainWindow::exifRename_display},
    {"Show statistics", "Get statistics of selected folders (Extensions, camera models, empty folders) and write them into a text file", &MainWindow::showStats, &MainWindow::showStats_display}};

#define MOVE_TO_UI_THREAD(func, ...) QMetaObject::invokeMethod(this, func, Qt::QueuedConnection, __VA_ARGS__);

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow) {

    ui->setupUi(this);

    // setup initial values
    program_start_time = QDateTime::currentMSecsSinceEpoch();
    lastMeasuredDiskRead = getDiskReadSizeB();

    setCurrentTask("Idle");

    QStringList modeNames;
    for(auto& [mode_name, mode_description, pfunc, dfunc]: scan_modes){
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

    auto timer_1s = new QTimer(this);
    timer_1s->setInterval(2000); // 2000ms (0.5 times per second)
    connect(timer_1s, &QTimer::timeout, this, &MainWindow::updateLoop2s);
    timer_1s->start();

    // setup ui listeners
    connect(ui->add_scan_folder_button, SIGNAL(clicked()), this, SLOT(onAddScanFolderClicked()));
    connect(ui->add_slave_folder_button, SIGNAL(clicked()), this, SLOT(onAddSlaveFolderClicked()));

    connect(ui->set_master_folder_button, SIGNAL(clicked()), this, SLOT(onSetMasterFolderClicked()));
    connect(ui->set_dupes_folder_button, SIGNAL(clicked()), this, SLOT(onSetDupesFolderClicked()));

    connect(ui->add_extention_button, SIGNAL(clicked()), this, SLOT(onAddExtensionButtonClicked()));

    connect(ui->mode_combo_box, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentModeChanged(int)));

    ui->extention_filter_enabled_checkbox->setChecked(true);
    connect(ui->extention_filter_enabled_checkbox, SIGNAL(stateChanged(int)), this, SLOT(onExtentionCheckboxStateChanged(int)));

    connect(ui->start_scan_button, SIGNAL(clicked()), this, SLOT(onStartScanButtonClicked()));
}

MainWindow::~MainWindow() {
  delete ui;
}

#pragma region MainWindow loops {

void MainWindow::updateLoop100Ms() {
     // update uptime
    ui->uptime_label->setText(QString("Uptime: %1").arg(timeSinceTimestamp(program_start_time)));

    // update piechart
    series->slices().at(PiechartIndex::ALL_FILES)->setValue(unique_files.length() - hashed_files);
    series->slices().at(PiechartIndex::SCANNED_FILES)->setValue(hashed_files);
    for (int i = 0; i < series->count(); i++){
        auto slice = series->slices().at(i);
        slice->setLabel(QString(piechart_config[i].first).arg(slice->value()));
    }

    if (scan_active) {
        ui->time_passed_label->setText(QString("Time passed: %1").arg(timeSinceTimestamp(scan_start_time)));
        if (averageDiskReadSpeed == 0 || files_size_all == 0){
           ui->eta_label->setText("Eta: ???");
        } else {
           ui->eta_label->setText(QString("Eta: %1").arg(millisecondsToReadable(((files_size_all - files_size_scanned) / averageDiskReadSpeed) * 1000)));
        }
    }

}

void MainWindow::updateLoop2s() {

    // update disk usage
    auto disk_read = getDiskReadSizeB();
    ui->disk_read_label->setText(QString("Disk read: %1/s").arg(bytesToReadable((disk_read - lastMeasuredDiskRead) / 2.0)));
    // floating average
    averageDiskReadSpeed = averageDiskReadSpeed * 0.9 + ((disk_read - lastMeasuredDiskRead) / 2.0) * 0.1;
    lastMeasuredDiskRead = disk_read;

    // update memory usage
    ui->memory_usage_label->setText(QString("Memory usage: %1").arg(bytesToReadable(getMemUsedKb() * 1024)));

}

#pragma endregion}

#pragma region MainWindow Listeners {

void MainWindow::onAddScanFolderClicked() {
    addItemToList(callFileDialogue("Open a folder", QFileDialog::Options(QFileDialog::ShowDirsOnly)), ui->folders_to_scan_list);
}

void MainWindow::onAddSlaveFolderClicked() {
    addItemToList(callFileDialogue("Open a folder", QFileDialog::Options(QFileDialog::ShowDirsOnly)), ui->slave_folders_list);
}

void MainWindow::onSetMasterFolderClicked() {
    masterFolder = callFileDialogue("Choose master folder", QFileDialog::Options(QFileDialog::ShowDirsOnly));
    ui->master_folder_label->setText(QString("Master folder: %1").arg(masterFolder));
}

void MainWindow::onSetDupesFolderClicked() {
    dupesFolder = callFileDialogue("Choose master folder", QFileDialog::Options(QFileDialog::ShowDirsOnly));
    ui->dupes_folder_label->setText(QString("Dupes folder: %1").arg(dupesFolder));
}

void MainWindow::onExtentionCheckboxStateChanged(int state) {
    ui->add_extention_button->setDisabled(state == Qt::CheckState::Unchecked);
    ui->extension_filter_list->setDisabled(state == Qt::CheckState::Unchecked);
}

void MainWindow::onAddExtensionButtonClicked() {
    addItemToList(callTextDialogue("Input ext", "Extension:").replace(".", ""), ui->extension_filter_list);
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

    // reset variables
    hashed_files = 0;
    files_size_all = 0;
    files_size_scanned = 0;
    unique_files.clear();

    setUiDisabled(true);

    scan_active = true;
    scan_start_time = QDateTime::currentMSecsSinceEpoch();

    // start separate event loop so the main thread does not hang
    QEventLoop loop;
    QFutureWatcher<void> futureWatcher;

    // QueuedConnection is necessary in case the signal finished is emitted before the loop starts (if the task is already finished when setFuture is called)
    connect(&futureWatcher, SIGNAL(finished()), &loop, SLOT(quit()), Qt::QueuedConnection);

    futureWatcher.setFuture(QtConcurrent::run(this, &MainWindow::startScanAsync));

    // process heavy stuff in a separate thread
    loop.exec();
    futureWatcher.waitForFinished();

    // display results in main thread
    scan_modes.at(currentMode).display_function(this);

    setUiDisabled(false);
    scan_active = false;
    setCurrentTask("Idle");
}

#pragma endregion}

#pragma region MainWindow List operations {

void MainWindow::addItemToList(const QString &text, QListWidget *list) {

    if (text.isEmpty() || text.isNull()) {
        return;
    }

    if (getFromList(text, list)) {
        displayWarning("Item is already in the list");
        return;
    }

    auto item = new QListWidgetItem();
    auto widget = new FolderListItemWidget(this, list);

    widget->setText(text);
    item->setSizeHint(widget->sizeHint());

    list->addItem(item);
    list->setItemWidget(item, widget);
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

QString MainWindow::callFileDialogue(const QString &title, QFileDialog::Options options) {
    return QFileDialog::getOpenFileName(this, title, "", "", nullptr, options);
}

QString MainWindow::callTextDialogue(const QString &title, const QString &prompt) {
    bool ok;
    QString text = QInputDialog::getText(this, title, prompt, QLineEdit::Normal, "", &ok);
    return ok ? text : "";
}

void MainWindow::displayWarning(const QString &message){
    QMessageBox::warning(this, "Warning", message);
}

void MainWindow::setCurrentTask(const QString &status){
    if(QThread::currentThread() == thread()) {
        ui->current_task_label->setText(QString("Current task: %1").arg(status));
    } else {
        MOVE_TO_UI_THREAD("setCurrentTask", Q_ARG(QString, status));
    }
}

void MainWindow::setLastMessage(const QString &status) {
    if(QThread::currentThread() == thread()) {
       ui->last_output_label->setText(QString("Last output: %1").arg(status));
    } else {
       MOVE_TO_UI_THREAD("setLastMessage", Q_ARG(QString, status));
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

void MainWindow::startScanAsync() {

    QStringList folders_to_enumerate;
    for(int i = 0; i < ui->folders_to_scan_list->count(); i++){
        auto item = ui->folders_to_scan_list->item(i);
        auto itemWidget = widgetFromWidgetItem(item);
        walkDir(itemWidget->getText(), [this](QString file) {
            addEnumeratedFile(file);});
    }

    // remove duplicates
    unique_files.erase(std::unique(unique_files.begin(), unique_files.end() ), unique_files.end());

    for(auto& file: unique_files) {
        files_size_all += QFile(file.full_path).size();
    }

    scan_modes.at(currentMode).process_function(this);
}

void MainWindow::addEnumeratedFile(const QString& file) {
    QFile file_r = file;
    if(unique_files.size() % 100 == 0) {
        setCurrentTask(QString("Enumerating file: %1").arg(file));
    }
    unique_files.push_back({file, file_r.fileName()});
};

void MainWindow::hashAllFiles() {
     for (auto& u_file: unique_files){
         hashed_files ++;
         files_size_scanned += QFile(u_file.full_path).size();
         setCurrentTask(QString("Hashing file: %1").arg(u_file.full_path));
         u_file.hash = getFileHash(u_file.full_path);
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

void MainWindow::showStats_display() {
    stats_dialog = new Stats_dialog(this);
    stats_dialog->setModal(true);
    hide();
    stats_dialog->show();
}

#pragma endregion}
