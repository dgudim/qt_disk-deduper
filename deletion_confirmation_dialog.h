#ifndef DELETION_CONFIRMATION_DIALOG_H
#define DELETION_CONFIRMATION_DIALOG_H

#include "datatypes.h"
#include <QDialog>

namespace Ui {
    class Deletion_confirmation_dialog;
}

class Deletion_confirmation_dialog : public QDialog {
    Q_OBJECT

public:
    explicit Deletion_confirmation_dialog(QWidget *parent, const QVector<File>& files_to_delete);
    ~Deletion_confirmation_dialog();

private slots:

    void safeDelete();
    void hardDelete();

private:

    QVector<File> files_to_delete;

    Ui::Deletion_confirmation_dialog *ui;

};

#endif // DELETION_CONFIRMATION_DIALOG_H
