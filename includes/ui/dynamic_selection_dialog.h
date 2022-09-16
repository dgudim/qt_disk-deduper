#ifndef DYNAMIC_SELECTION_DIALOG_H
#define DYNAMIC_SELECTION_DIALOG_H

#include <QDialog>
#include <QCheckBox>

#include "gutils.h"
#include "ui_dynamic_selection_dialog.h"

namespace Ui {
    class Dynamic_selection_dialog;
}

class Dynamic_selection_dialog : public QDialog {
    Q_OBJECT

public:

    template<typename T>
    explicit Dynamic_selection_dialog(QWidget *parent, QList<T> selectionList) : QDialog(parent), ui(new Ui::Dynamic_selection_dialog) {
        ui->setupUi(this);

        for(const auto& field: selectionList) {
            QCheckBox *checkBox = new QCheckBox(this);
            checkBoxes.push_back(checkBox);
            if constexpr (std::is_base_of<QString, T>::value) {
                checkBox->setText(field);
            } else {
                checkBox->setText(field.first);
            }
            ui->checkbox_container->addWidget(checkBox);
        }

        ui->checkbox_container->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

        connect(ui->select_all_button, &QPushButton::clicked, this, &Dynamic_selection_dialog::checkAll);
        UiUtils::connectDialogButtonBox(this, ui->buttonBox);
    }

    ~Dynamic_selection_dialog() {
        delete ui;
    }

    QVector<QString> getSelected() {
        QVector<QString> selected;
        for(auto& checkBox: checkBoxes) {
            if(checkBox->isChecked()) {
                selected.push_back(checkBox->text());
            }
        }
        return selected;
    }

private:

    Ui::Dynamic_selection_dialog *ui;

    void checkAll() {
        for(auto& checkBox: checkBoxes) {
            checkBox->setChecked(true);
        }
    };

    QVector<QCheckBox*> checkBoxes;

};

#endif // DYNAMIC_SELECTION_DIALOG_H
