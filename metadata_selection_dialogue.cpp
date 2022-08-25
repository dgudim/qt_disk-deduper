#include "metadata_selection_dialogue.h"
#include "ui_metadata_selection_dialogue.h"

Metadata_selection_dialogue::Metadata_selection_dialogue(QWidget *parent) : QDialog(parent), ui(new Ui::Metadata_selection_dialogue) {
    ui->setupUi(this);

    QList<QString> meta_fileds = getMetaFieldsList();
    for(const auto& meta_field: meta_fileds) {
        QCheckBox *checkBox = new QCheckBox(this);
        checkBoxes.push_back(checkBox);
        checkBox->setText(meta_field);
        ui->checkbox_container->addWidget(checkBox);
    }

    ui->checkbox_container->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QVector<QString> Metadata_selection_dialogue::getSelected() {
    QVector<QString> selected;
    for(const auto& checkBox: qAsConst(checkBoxes)) {
        if(checkBox->checkState() == Qt::CheckState::Checked) {
            selected.push_back(checkBox->text());
        }
    }
    return selected;
}

Metadata_selection_dialogue::~Metadata_selection_dialogue() {
    delete ui;
}
