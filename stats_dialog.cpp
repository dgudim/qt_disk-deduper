#include "stats_dialog.h"
#include "ui_stats_dialog.h"

Stats_dialog::Stats_dialog(QWidget *parent) : QDialog(parent), ui(new Ui::Stats_dialog) {
    ui->setupUi(this);

    // open main window on dialogue close
    connect(this, SIGNAL(accepted()), parent, SLOT(show()));
    connect(this, SIGNAL(rejected()), parent, SLOT(show()));
    connect(ui->discard_button, SIGNAL(clicked()), this, SLOT(rejected()));
    connect(ui->save_button, SIGNAL(clicked()), this, SLOT(accept()));

    ui->table->insertColumn(0);
    ui->table->insertRow(0);
    QTableWidgetItem item;
    item.setBackground(QBrush(QColor(200, 40, 20)));
    ui->table->setItem(0, 0, &item);
}

Stats_dialog::~Stats_dialog() {
    delete ui;
}
