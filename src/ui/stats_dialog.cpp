#include "gutils.h"
#include "stats_dialog.h"
#include "ui_stats_dialog.h"

class TableNumberItem : public QTableWidgetItem {
public:
    TableNumberItem(const QString txt) : QTableWidgetItem(txt) {}

    bool operator < (const QTableWidgetItem &other) const {
        QString str1 = text();
        QString str2 = other.text();

        if (str1[str1.length() - 1] == '%') {
            str1.chop(1);
            str2.chop(1);
        } else if (str1.endsWith("b") || str1.endsWith("Kb") || str1.endsWith("Mb") || str1.endsWith("Gb")){
            return FileUtils::readableToBytes(str1) < FileUtils::readableToBytes(str2);
        }

        return str1.toDouble() < str2.toDouble();
    }
};

Stats_dialog::Stats_dialog(QWidget *parent, const StatsContainer& stats) : QDialog(parent), ui(new Ui::Stats_dialog) {
    ui->setupUi(this);

    setWindowTitle("View statistics");

    UiUtils::connectDialogButtonBox(this, ui->buttonBox);

    ui->total_files_label->setText(QString("Total files: %1").arg(stats.total_files.num()));
    ui->total_size_label->setText(QString("Total size: %1").arg(stats.total_files.size_readable()));

    for(const auto& [stat_name, stat_data]: stats.meta_fields_stats) {
        loadTable({stat_name, "Count", "Count percentage", "Size", "Size percentage"}, stat_name, stat_data);
    }
}

void Stats_dialog::loadTable(const QStringList& headers, const QString& name, const CountableQStringList& fields) {

    // create new tab
    QWidget* tab_container = new QWidget();
    QVBoxLayout* tab_contents = new QVBoxLayout(tab_container);
    tab_contents->setSpacing(0);
    tab_contents->setContentsMargins(0, 0, 0, 0);

    // create new table
    QTableWidget* table = new QTableWidget(tab_container);
    tab_contents->addWidget(table);

    // add tab to widget
    ui->stats_tabs->addTab(tab_container, name + " stats");

    // populate table
    for(int i = 0; i < headers.length(); i++) {
        table->insertColumn(i);
    }
    table->setHorizontalHeaderLabels(headers);
    int row = 0;
    for(auto& ext: fields) {
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem (ext.string));
        table->setItem(row, 1, new TableNumberItem (QString::number(ext.count)));
        table->setItem(row, 2, new TableNumberItem (QString("%1%").arg(ext.count_percentage)));
        table->setItem(row, 3, new TableNumberItem (ext.size_readable()));
        table->setItem(row, 4, new TableNumberItem (QString("%1%").arg(ext.size_percentage)));
        row ++;
    }
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSortingEnabled(true);
    table->setAlternatingRowColors(true);
}

Stats_dialog::~Stats_dialog() {
    delete ui;
}
