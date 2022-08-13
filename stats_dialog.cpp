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
            return readableToBytes(str1) < readableToBytes(str2);
        }

        return str1.toDouble() < str2.toDouble();
    }
};

Stats_dialog::Stats_dialog(QWidget *parent, const StatsContainer& stats) : QDialog(parent), ui(new Ui::Stats_dialog) {
    ui->setupUi(this);

    setWindowTitle("View statistics");

    // open main window on dialogue close
    connect(this, SIGNAL(accepted()), parent, SLOT(show()));
    connect(this, SIGNAL(rejected()), parent, SLOT(show()));
    connect(ui->discard_button, SIGNAL(clicked()), this, SLOT(reject()));
    connect(ui->save_button, SIGNAL(clicked()), this, SLOT(accept()));

    ui->total_files_label->setText(QString("Total files: %1").arg(stats.total_files));
    ui->total_size_label->setText(QString("Total size: %1").arg(bytesToReadable(stats.total_size)));

    for(auto& [stats_name, stats_data]: stats.meta_fields_stats) {
        loadTable({stats_name, "Count", "Count percentage", "Size", "Size percentage"}, stats_name, stats_data);
    }
}

void Stats_dialog::loadTable(const QStringList& headers, const QString& name, const QVector<Countable_qstring>& data) {

    QWidget* tab_container = new QWidget();
    QVBoxLayout* verticalLayout = new QVBoxLayout(tab_container);
    QTableWidget* table = new QTableWidget(tab_container);
    verticalLayout->addWidget(table);

    ui->stats_tabs->addTab(tab_container, QString());
    ui->stats_tabs->setTabText(ui->stats_tabs->indexOf(tab_container), name + " stats");

    for(int i = 0; i < headers.length(); i++) {
        table->insertColumn(i);
    }
    table->setHorizontalHeaderLabels(headers);
    int row = 0;
    for(auto& ext: data) {
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem (ext.string));
        table->setItem(row, 1, new TableNumberItem (QString::number(ext.count)));
        table->setItem(row, 2, new TableNumberItem (QString("%1%").arg(ext.count_percentage)));
        table->setItem(row, 3, new TableNumberItem (bytesToReadable(ext.total_size_bytes)));
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
