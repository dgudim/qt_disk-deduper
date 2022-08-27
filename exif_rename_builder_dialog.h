#ifndef EXIF_RENAME_BUILDER_DIALOG_H
#define EXIF_RENAME_BUILDER_DIALOG_H

#include "datatypes.h"
#include <QDialog>

namespace Ui {
    class Exif_rename_builder_dialog;
}

class Exif_rename_builder_dialog : public QDialog {
    Q_OBJECT

public:
    explicit Exif_rename_builder_dialog(QWidget *parent = nullptr);
    ~Exif_rename_builder_dialog();

    ExifFormat getFormat();

private:

    QList<QString> metaFieldsList;

    bool template_valid = true;

    void validateTemplate(const QString &new_text);

    Ui::Exif_rename_builder_dialog *ui;
};

#endif // EXIF_RENAME_BUILDER_DIALOG_H
