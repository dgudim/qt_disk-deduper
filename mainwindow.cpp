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
    std::function<void(MainWindow*)> called_function;
};

QList<ScanModeProperties> scan_modes = {
    {"Hash duplicates", "Compare files by hash and show results in groups for further action", &MainWindow::hashCompare},
    {"Name duplicates", "Compare files by name and show results in groups for further action", &MainWindow::nameCompare},
    {"Auto dedupe(move)", "Compare master folder and slave folders by hash (Files from the slave folders are moved into the dupes folder if they are present in the master folder)", &MainWindow::autoDedupeMove},
    {"Auto dedupe(rename)", "Compare master folder and slave folders by hash (DELETED_ is added to the name of a file from the slave folders if it is present in the master folder)", &MainWindow::autoDedupeRename},
    {"EXIF rename", "Rename files according to their EXIF data (Name format: <creation date and time>_<camera model>_numbers from file name)", &MainWindow::exifRename},
    {"Show statistics", "Get statistics of selected folders (Extensions, camera models, empty folders) and write them into a text file", &MainWindow::showStats}};

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow) {

    ui->setupUi(this);

    // setup initial values
    program_start_time = QDateTime::currentMSecsSinceEpoch();
    lastMeasuredDiskRead = getDiskReadSizeKb();

    setCurrentTask("Idle");

    QStringList modeNames;
    for(auto& [mode_name, mode_description, func]: scan_modes){
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
    timer_1s->setInterval(3000); // 3000ms (0.3 times per second)
    connect(timer_1s, &QTimer::timeout, this, &MainWindow::updateLoop3s);
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
    ui->uptime_label->setText(QString("Uptime: %1").arg(millisecondsToReadable(QDateTime::currentMSecsSinceEpoch() - program_start_time)));

    // update piechart
    series->slices().at(PiechartIndex::ALL_FILES)->setValue(unique_files.length() - hashed_files);
    series->slices().at(PiechartIndex::SCANNED_FILES)->setValue(hashed_files);
    for (int i = 0; i < series->count(); i++){
        auto slice = series->slices().at(i);
        slice->setLabel(QString(piechart_config[i].first).arg(slice->value()));
    }

}

void MainWindow::updateLoop3s() {

    // update disk usage
    auto disk_read = getDiskReadSizeKb();
    ui->disk_read_label->setText(QString("Disk read: %1/s").arg(kbToReadable((disk_read - lastMeasuredDiskRead) / 3.0)));
    lastMeasuredDiskRead = disk_read;

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

    QEventLoop loop;
    QFutureWatcher<void> futureWatcher;

    connect(&futureWatcher, SIGNAL(finished()), &loop, SLOT(quit()));

    futureWatcher.setFuture(QtConcurrent::run(this, &MainWindow::startScanAsync));

    loop.exec();
    futureWatcher.waitForFinished();

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
    if(QThread::currentThread() == this->thread()) {
        ui->current_task_label->setText(QString("Current task: %1").arg(status));
    } else {
        QMetaObject::invokeMethod(this, [this, status]{ui->current_task_label->setText(QString("Current task: %1").arg(status));});
    }
}

void MainWindow::setLastMessage(const QString &status) {
    if(QThread::currentThread() == this->thread()) {
     ui->last_output_label->setText(QString("Last output: %1").arg(status));
    } else {
       QMetaObject::invokeMethod(this, [this, status]{ ui->last_output_label->setText(QString("Last output: %1").arg(status));});
    }
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
    unique_files.erase( std::unique(unique_files.begin(), unique_files.end() ), unique_files.end() );

    scan_modes.at(currentMode).called_function(this);
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

}

void MainWindow::autoDedupeRename() {

}

void MainWindow::exifRename() {

}

void MainWindow::showStats() {

}

#pragma endregion}
