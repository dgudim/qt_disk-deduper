#ifndef DUPE_RESULTS_DIALOG_H
#define DUPE_RESULTS_DIALOG_H

#include "datatypes.h"

#include <QDialog>

namespace Ui {
    class Dupe_results_dialog;
}

class Dupe_results_dialog : public QDialog {
    Q_OBJECT

public:
    explicit Dupe_results_dialog(QWidget *parent, const QVector<QVector<QVector<File>>>& results, int total_files, quint64 total_size);
    ~Dupe_results_dialog();

private:

    void loadTab(const QString& name, const QVector<QVector<File>>& list);

    Ui::Dupe_results_dialog *ui;
};

#endif // DUPE_RESULTS_DIALOG_H
