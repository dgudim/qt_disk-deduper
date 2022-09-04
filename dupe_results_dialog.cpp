#include "dupe_results_dialog.h"
#include <QDesktopServices>
#include <QUrl>
#include <QPushButton>
#include <QScrollArea>

#include "clickableQLabel.h"
#include "deletion_confirmation_dialog.h"
#include "ui_dupe_results_dialog.h"

Dupe_results_dialog::Dupe_results_dialog(QWidget *parent, const MultiFileGroupArray& results, const FileQuantitySizeCounter& total_files) :
    QDialog(parent), ui(new Ui::Dupe_results_dialog) {

    ui->setupUi(this);

    setWindowTitle("View duplicate files");

    ui->res_label->setText(QString("Total files: %1 | Total groups: %2 | Total size: %3")
                           .arg(total_files.num()).arg(results.size()).arg(total_files.size_readable()));

    allGroups = results;

    loadNTabs();
    connect(ui->load_more_groups, &QPushButton::clicked, this, &Dupe_results_dialog::loadNTabs);
}

void Dupe_results_dialog::loadNTabs(int n) {
    // QPushButton::clicked returns boolean (false) which is 0 so we set n to default value (10)
    n = n ? n : 10;
    for(int i = 0; i < n && current_group_index < allGroups.length(); i++) {
        pButtonGroups button_groups;
        const auto& group = allGroups.value(current_group_index);
        button_groups.reset(new ButtonGroups());
        button_groups_per_tab.append(button_groups);
        current_group_index++;
        loadTab(QString("Group %1").arg(current_group_index), group, button_groups);
    }

    // all tabs were loaded
    if(current_group_index >= allGroups.length()) {
        ui->load_more_groups->setDisabled(true);
        ui->load_more_groups->setHidden(true);
    }
}

void Dupe_results_dialog::loadTab(const QString& name, const MultiFileGroup& list, pButtonGroups button_groups) {

    //create new tab
    QWidget* tab_container = new QWidget();
    QVBoxLayout* tab_contents = new QVBoxLayout(tab_container);
    tab_contents->setSpacing(0);
    tab_contents->setContentsMargins(0, 0, 0, 0);

    QPushButton* applyButton = new QPushButton("Keep selected, delete the rest", tab_container);

    connect(applyButton, &QPushButton::clicked, tab_container, [button_groups, this](){
        QVector<File> files_to_delete;
        for(auto& button_group: *button_groups) {
            QList<QAbstractButton*> buttons = button_group->buttons();
            for (auto button: buttons){
                // get all files except selected
                if(button != button_group->checkedButton()) {
                    files_to_delete.append(button_to_file_map.value(button));
                }
            }
        }
        Deletion_confirmation_dialog deleteion_confirmation_dialog(this, files_to_delete);
        if (deleteion_confirmation_dialog.exec() == QDialog::Accepted){
            ui->tabWidget->removeTab(ui->tabWidget->currentIndex());
            if (!ui->tabWidget->count()){
                // close dialog
                accept();
            }
            loadNTabs(1);
        }
    });

    tab_contents->addWidget(applyButton);

    int items_in_group = 0;
    for(auto& group: list) {
        items_in_group += group.size();
    }
    QLabel* stats_label = new QLabel(QString("Files in group: %1 / %2 (unique)").arg(items_in_group).arg(list[0].size()), tab_container);

    tab_contents->addWidget(stats_label);

    // create new scroll area for items
    QScrollArea *scrollArea = new QScrollArea(tab_container);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMouseTracking(true);
    QWidget* scrollAreaWidgetContents = new QWidget();

    // create vertical layout for scroll area (this is where our items will reside)
    QVBoxLayout* verticalLayout_inner = new QVBoxLayout(scrollAreaWidgetContents);
    verticalLayout_inner->setSpacing(3);
    verticalLayout_inner->setContentsMargins(3, 3, 3, 3);

    int group_index = 0;
    for(const auto& group: list) {
        QHBoxLayout* groupContainer = new QHBoxLayout();

        QVBoxLayout* infoAndButtonContainer = new QVBoxLayout();
        QPushButton* selectWholeGroupButton = new QPushButton("Select whole line", scrollAreaWidgetContents);
        selectWholeGroupButton->setMaximumWidth(200);

        connect(selectWholeGroupButton, &QPushButton::clicked, this,
        [group_index, button_groups, list]() {
            for(int file_index = 0; file_index < list[0].size(); file_index ++) {
                button_groups->at(file_index)->buttons().at(group_index)->setChecked(true);
            }
        });

        QLabel* path_label = new QLabel(group[0].path_without_name, scrollAreaWidgetContents);
        path_label->setMaximumWidth(150);
        path_label->setMinimumWidth(150);
        path_label->setWordWrap(true);
        infoAndButtonContainer->addWidget(path_label);
        infoAndButtonContainer->addWidget(selectWholeGroupButton);
        groupContainer->addLayout(infoAndButtonContainer);

        int index = 0;
        for(const auto& file: group) {

            QVBoxLayout* preview_container = new QVBoxLayout();

            // construct 2 labels (preview and filename)
            ClickableQLabel *preview_label = new ClickableQLabel(scrollAreaWidgetContents);
            connect(preview_label, &ClickableQLabel::clicked, preview_label,
            [file](){
                QDesktopServices::openUrl(QUrl(QString("file://%1").arg(file)));
            });
            preview_label->setDisabled(true);

            QLabel* filename_label = new ClickableQLabel(file.name, scrollAreaWidgetContents);
            filename_label->setWordWrap(true);
            filename_label->setDisabled(true);
            filename_label->setAlignment(Qt::AlignTop);

            // load thumbnail
            QPixmap thumbnail;
            thumbnail.loadFromData(file.thumbnail_raw);
            preview_label->setPixmap(thumbnail);
            preview_container->addWidget(preview_label);


            QRadioButton* selection_button = new QRadioButton("select", scrollAreaWidgetContents);
            button_to_file_map.insert(selection_button, file);
            preview_container->addWidget(selection_button);

            connect(selection_button, &QRadioButton::toggled, this,
            [preview_label, filename_label](bool checked){
                preview_label->setDisabled(!checked);
                filename_label->setDisabled(!checked);
            });

            if(button_groups->size() <= index) {
                auto group = std::make_shared<QButtonGroup>();
                selection_button->setChecked(true);
                group->addButton(selection_button);
                button_groups->append(group);
            } else {
                button_groups->at(index)->addButton(selection_button);
            }



            preview_container->addWidget(filename_label);

            groupContainer->addSpacing(30);

            QFrame *v_line = new QFrame(scrollAreaWidgetContents);
            v_line->setMinimumWidth(5);
            v_line->setMaximumWidth(5);
            v_line->setStyleSheet("background: #888");
            groupContainer->addWidget(v_line);

            groupContainer->addLayout(preview_container);

            index++;
        }

        verticalLayout_inner->addLayout(groupContainer);

        QFrame *h_line = new QFrame(scrollAreaWidgetContents);
        h_line->setMinimumHeight(3);
        h_line->setMaximumHeight(3);
        h_line->setStyleSheet("background: #666");
        verticalLayout_inner->addWidget(h_line);

        group_index ++;
    }

    scrollArea->setWidget(scrollAreaWidgetContents);
    tab_contents->addWidget(scrollArea);
    ui->tabWidget->addTab(tab_container, name);
}

Dupe_results_dialog::~Dupe_results_dialog() {
    delete ui;
}
