#include "move_confirmation_dialog.h"
#include "gutils.h"
#include "ui_move_confirmation_dialog.h"

MoveConfirmationDialog::MoveConfirmationDialog(QWidget *parent, PairList<File, QString> files_to_rename) : QDialog(parent),
    ui(new Ui::MoveConfirmationDialog) {

    ui->setupUi(this);

    for(auto& [file, target]: files_to_rename) {
        ui->list->addItem(QString("%1 ---> %2").arg(file, target));
    }

    UiUtils::connectDialogButtonBox(this, ui->buttonBox);

}

MoveConfirmationDialog::~MoveConfirmationDialog() {
    delete ui;
}
