#ifndef STATS_DIALOG_H
#define STATS_DIALOG_H

#include <QDialog>
#include <QTableWidget>
#include "datatypes.h"

namespace Ui {
    class Stats_dialog;
}

class Stats_dialog : public QDialog {
    Q_OBJECT

public:
    explicit Stats_dialog(QWidget *parent, const StatsContainer &stats);
    ~Stats_dialog();

private:

    void loadTable(const QStringList& headers, const QString& name, const QVector<Countable_qstring>& data);

    Ui::Stats_dialog *ui;
};

#endif // STATS_DIALOG_H
