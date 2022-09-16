#ifndef MOVE_CONFIRMATION_DIALOG_H
#define MOVE_CONFIRMATION_DIALOG_H

#include "datatypes.h"

#include <QDialog>

namespace Ui {
class MoveConfirmationDialog;
}

class MoveConfirmationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MoveConfirmationDialog(QWidget *parent, PairList<File, QString> files_to_rename);
    ~MoveConfirmationDialog();

private:
    Ui::MoveConfirmationDialog *ui;
};

#endif // MOVE_CONFIRMATION_DIALOG_H
