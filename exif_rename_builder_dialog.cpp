#include "exif_rename_builder_dialog.h"
#include "ui_exif_rename_builder_dialog.h"

#include "gutils.h"

#include <QSettings>

QSettings exif_settings(QSettings::UserScope, "disk_deduper_qt", "exif_ui_state");

Exif_rename_builder_dialog::Exif_rename_builder_dialog(QWidget *parent) :
    QDialog(parent), ui(new Ui::Exif_rename_builder_dialog) {

    ui->setupUi(this);

    metaFieldsList = getMetaFieldsList();

    for(auto& metaField: metaFieldsList) {
        ui->availbale_metaFields->addItem(metaField);
    }

    connect(ui->add_filed_to_template_button, &QPushButton::clicked, this,
    [this](){
         QList<QListWidgetItem*> items = ui->availbale_metaFields->selectedItems();
         for(auto& item: items) {
            ui->input->setText(ui->input->text() + "[" + item->text() + "]");
         }
    });

    UiUtils::connectDialogButtonBox(this, ui->buttonBox);

    ui->input->setText(exif_settings.value("template", "[Camera manufacturer]_[Camera model]_[Creation date]").toString());
    validateTemplate(ui->input->text());

    ui->if_metadata_not_available_selector->addItem("Do nothing");
    ui->if_metadata_not_available_selector->addItem("Skip file");
    ui->if_metadata_not_available_selector->addItem("Stop renaming");

    ui->if_file_exists_selector->addItem("Append index");
    ui->if_file_exists_selector->addItem("Skip file");
    ui->if_file_exists_selector->addItem("Stop renaming");

    ui->if_file_exists_selector->setCurrentIndex(exif_settings.value("on_file_exists", 0).toInt());
    ui->if_metadata_not_available_selector->setCurrentIndex(exif_settings.value("on_fail", 1).toInt());

    connect(ui->input, &QLineEdit::textChanged, this, &Exif_rename_builder_dialog::validateTemplate);
}

void Exif_rename_builder_dialog::validateTemplate(const QString& new_text) {
    bool parenth_opened = false;
    QString already_parsed;
    QString metaField;
    template_valid = true;
    for(auto& ch: new_text) {
        already_parsed += ch;
        if(parenth_opened && ch != ']') {
            metaField += ch;
        }
        if(ch == '[') {
            if(parenth_opened) {
                ui->template_invalid_label->setText(QString("Template invalid: double opened parenthesis, error near %1").arg(already_parsed));
                template_valid = false;
                break;
            }
            parenth_opened = true;
        } else if (ch == ']') {
            if(!parenth_opened) {
                ui->template_invalid_label->setText(QString("Template invalid: closed not opened parenthesis, error near %1").arg(already_parsed));
                template_valid = false;
                break;
            }
            if(!metaFieldsList.contains(metaField)) {
                ui->template_invalid_label->setText(QString("Template invalid: unknown field: %1, error near %2").arg(metaField, already_parsed));
                template_valid = false;
                break;
            }
            metaField = "";
            parenth_opened = false;
        }
    }
    if(template_valid && parenth_opened) {
        ui->template_invalid_label->setText(QString("Template invalid: missing closing parenthesis, error near %1").arg(already_parsed));
        template_valid = false;
    }
    if(template_valid && new_text.isEmpty()) {
        ui->template_invalid_label->setText(QString("Template invalid: empty template"));
        template_valid = false;
    }
    ui->template_invalid_label->setVisible(!template_valid);
}

Exif_rename_builder_dialog::~Exif_rename_builder_dialog() {

    exif_settings.setValue("on_file_exists", ui->if_file_exists_selector->currentIndex());
    exif_settings.setValue("on_fail", ui->if_metadata_not_available_selector->currentIndex());
    exif_settings.setValue("template", ui->input->text());

    delete ui;
}

ExifFormat Exif_rename_builder_dialog::getFormat() {
    return ExifFormat(ui->input->text(),
                      (OnFailAction)ui->if_metadata_not_available_selector->currentIndex(),
                      (OnFileExistsAction)ui->if_file_exists_selector->currentIndex());
}
