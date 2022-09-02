#include "metadata_selection_dialog.h"
#include "ui_metadata_selection_dialog.h"

#include "gutils.h"

Metadata_selection_dialog::Metadata_selection_dialog(QWidget *parent) : QDialog(parent), ui(new Ui::Metadata_selection_dialog) {
    ui->setupUi(this);

    QList<QString> meta_fileds = getMetaFieldsList();
    for(const auto& meta_field: meta_fileds) {
        QCheckBox *checkBox = new QCheckBox(this);
        checkBoxes.push_back(checkBox);
        checkBox->setText(meta_field);
        ui->checkbox_container->addWidget(checkBox);
    }

    ui->checkbox_container->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

    connect(ui->select_all_button, &QPushButton::clicked, this, &Metadata_selection_dialog::checkAll);
    UiUtils::connectDialogButtonBox(this, ui->buttonBox);
}

void Metadata_selection_dialog::checkAll() {
    for(auto& checkBox: checkBoxes) {
        checkBox->setChecked(true);
    }
}

QVector<QString> Metadata_selection_dialog::getSelected() {
    QVector<QString> selected;
    for(auto& checkBox: checkBoxes) {
        if(checkBox->checkState() == Qt::CheckState::Checked) {
            selected.push_back(checkBox->text());
        }
    }
    return selected;
}

Metadata_selection_dialog::~Metadata_selection_dialog() {
    delete ui;
}
