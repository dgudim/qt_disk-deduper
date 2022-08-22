#include "dupe_results_dialog.h"
#include <QScrollArea>
#include "ui_dupe_results_dialog.h"
#include "gutils.h"

Dupe_results_dialog::Dupe_results_dialog(QWidget *parent, const QVector<QVector<QVector<File>>>& results, int total_files, quint64 total_size) : QDialog(parent), ui(new Ui::Dupe_results_dialog) {
    ui->setupUi(this);

    setWindowTitle("View duplicate files");

    // open main window on dialogue close
    connect(this, SIGNAL(rejected()), parent, SLOT(show()));

    ui->res_label->setText(QString("Total files: %1 | Total groups: %2 | Total size: %3")
                           .arg(total_files).arg(results.size()).arg(FileUtils::bytesToReadable(total_size)));

    int index = 0;
    for(const auto& groups: results) {
       index++;
       loadTab(QString("Group %1").arg(index), groups);
    }

}

void Dupe_results_dialog::loadTab(const QString& name, const QVector<QVector<File>>& list) {

    //create new tab
    QWidget* tab_container = new QWidget();
    QVBoxLayout* tab_contents = new QVBoxLayout(tab_container);
    tab_contents->setSpacing(0);
    tab_contents->setContentsMargins(0, 0, 0, 0);

    // create new scroll area for items
    QScrollArea *scrollArea = new QScrollArea(tab_container);
    scrollArea->setWidgetResizable(true);
    QWidget* scrollAreaWidgetContents = new QWidget();

    // create vertical layout for scroll area (this is where our items will reside)
    QVBoxLayout* verticalLayout_inner = new QVBoxLayout(scrollAreaWidgetContents);
    verticalLayout_inner->setSpacing(3);
    verticalLayout_inner->setContentsMargins(3, 3, 3, 3);

    for(const auto& group: list) {
        QHBoxLayout* groupContainer = new QHBoxLayout();
        groupContainer->addWidget(new QLabel(group[0].path_without_name, scrollAreaWidgetContents));
        for(const auto& file: group) {
            QLabel *preview_label = new QLabel(scrollAreaWidgetContents);
            QPixmap thumbnail;
            thumbnail.loadFromData(file.thumbnail_raw);
            preview_label->setPixmap(thumbnail);
            groupContainer->addWidget(preview_label);
        }
        verticalLayout_inner->addLayout(groupContainer);
    }

    scrollArea->setWidget(scrollAreaWidgetContents);
    tab_contents->addWidget(scrollArea);
    ui->tabWidget->addTab(tab_container, name);
}

Dupe_results_dialog::~Dupe_results_dialog() {
    delete ui;
}
