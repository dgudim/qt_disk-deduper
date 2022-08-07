#ifndef STATS_DIALOG_H
#define STATS_DIALOG_H

#include <QDialog>

namespace Ui {
    class Stats_dialog;
}

class Stats_dialog : public QDialog {
    Q_OBJECT

public:
    explicit Stats_dialog(QWidget *parent = nullptr);
    ~Stats_dialog();

private:
    Ui::Stats_dialog *ui;
};

#endif // STATS_DIALOG_H
