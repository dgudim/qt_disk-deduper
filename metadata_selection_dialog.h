#ifndef METADATA_SELECTION_DIALOG_H
#define METADATA_SELECTION_DIALOG_H

#include "stats_dialog.h"
#include <QDialog>
#include <QCheckBox>

namespace Ui {
    class Metadata_selection_dialogue;
}

class Metadata_selection_dialog : public QDialog {
    Q_OBJECT

public:
    explicit Metadata_selection_dialog(QWidget *parent);
    ~Metadata_selection_dialog();

    QVector<QString> getSelected();

private:

    Ui::Metadata_selection_dialogue *ui;
    Stats_dialog *stats_dialog;
    QVector<QCheckBox*> checkBoxes;

};

#endif // METADATA_SELECTION_DIALOG_H
