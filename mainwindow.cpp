#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtDebug>
#include <QMessageBox>

#include<QtCharts>
#include<QChartView>
#include<QPieSeries>
#include<QPieSlice>

QMap<QString, QColor> piechart_map = {
    {"A: %1", QColor(1, 184, 170)},
    {"Sc: %1", QColor(3, 171, 51)},
    {"Dup: %1", QColor(181, 113, 4)}};

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow) {

    ui->setupUi(this);

    QPieSeries *series = new QPieSeries();

    QList<QString> keys = piechart_map.keys();
    for (auto& key: keys){
        auto slice = series->append(key, 10);
        slice->setBrush(QBrush(piechart_map[key]));
        slice->setLabelBrush(QBrush(Qt::white));
        slice->setLabel(QString(key).arg(slice->value()));
    }

    series->setLabelsVisible();
    series->setLabelsPosition(QPieSlice::LabelInsideHorizontal);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setBackgroundVisible(false);
    chart->setMargins({0, 0, 0, 0});
    chart->setBackgroundRoundness(0);
    chart->legend()->hide();

    ui->qchart->setChart(chart);

    // setup button listeners
    connect(ui->add_scan_folder_button, SIGNAL(clicked()), this, SLOT(onAddScanFolderClicked()));
    connect(ui->add_slave_folder_button, SIGNAL(clicked()), this, SLOT(onAddSlaveFolderClicked()));

    connect(ui->set_master_folder_button, SIGNAL(clicked()), this, SLOT(onSetMasterFolderClicked()));
    connect(ui->set_dupes_folder_button, SIGNAL(clicked()), this, SLOT(onSetDupesFolderClicked()));
}

MainWindow::~MainWindow() {
  delete ui;
}

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

void MainWindow::addItemToList(const QString &text, QListWidget *list) {

    if (text.isEmpty() || text.isNull()) {
        return;
    }

    if (getFromList(text, list)) {
        QMessageBox::warning(this, "Warning", "Folder is already in the list");
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

FolderListItemWidget* MainWindow::widgetFromWidgetItem(QListWidgetItem *item) {
    return dynamic_cast<FolderListItemWidget*>(item->listWidget()->itemWidget(item));
}

QString MainWindow::callFileDialogue(const QString &title, QFileDialog::Options options) {
    return QFileDialog::getOpenFileName(this, title, "", "", nullptr, options);
}
