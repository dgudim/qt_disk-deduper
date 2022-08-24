#include "deletion_confirmation_dialog.h"
#include "gutils.h"
#include "ui_deletion_confirmation_dialog.h"

#include <QFile>

Deletion_confirmation_dialog::Deletion_confirmation_dialog(QWidget *parent, const QVector<File>& files_to_delete) : QDialog(parent),
    files_to_delete(files_to_delete), ui(new Ui::Deletion_confirmation_dialog) {

    ui->setupUi(this);

    ui->n_files_to_delete->setText(QString("%1 files are going to be deleted").arg(files_to_delete.size()));
    for(const auto& file: files_to_delete) {
        ui->files_to_delete_list->addItem(file);
    }

    // close dialog on cancel button clicked
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    // connect deleteion buttons to appropriate functions
    connect(ui->safeDeleteButton, &QPushButton::clicked, this, &Deletion_confirmation_dialog::safeDelete);
    connect(ui->deleteButton, &QPushButton::clicked, this, &Deletion_confirmation_dialog::hardDelete);
}

void Deletion_confirmation_dialog::hardDelete() {
    if (!FileUtils::deleteOrRenameFiles(files_to_delete,
    [this](const QString& status) {
        ui->status_label->setText(status);
    })) {
        // set status color to red on failure
        ui->status_label->setStyleSheet("color: red");
        return;
    };
    // all good, close dialog
    accept();
}

void Deletion_confirmation_dialog::safeDelete() {
    if (!FileUtils::deleteOrRenameFiles(files_to_delete,
    [this](const QString& status) {
        ui->status_label->setText(status);
    }, true, "", "_DELETED_")) {
        // set status color to red on failure
        ui->status_label->setStyleSheet("color: red");
        return;
    };
    // all good, close dialog
    accept();
}

Deletion_confirmation_dialog::~Deletion_confirmation_dialog() {
    delete ui;
}
