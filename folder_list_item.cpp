#include "folder_list_item.h"
#include "ui_folder_list_item.h"

FolderListItemWidget::FolderListItemWidget(QWidget *parent, QListWidget* list) : QWidget(parent), ui(new Ui::FolderListItemWidget) {

    this->parent_list = list;

    ui->setupUi(this);

    connect(ui->exclusionButton, SIGNAL(clicked()), this, SLOT(onExecutionButtonClicked()));
    connect(ui->deleteButton, SIGNAL(clicked()), this, SLOT(onRemoveButtonClicked()));
    connect(this, SIGNAL(sendRemoveItem(QString,QListWidget*)), parent, SLOT(removeItemFromList(QString,QListWidget*)));
}

FolderListItemWidget::~FolderListItemWidget() {
    delete ui;
}

void FolderListItemWidget::setText(const QString &text) {
    ui->folderNameLabel->setText(text);
}

void FolderListItemWidget::onRemoveButtonClicked() {
    emit sendRemoveItem(ui->folderNameLabel->text(), parent_list);
}

void FolderListItemWidget::onExecutionButtonClicked() {
    setWhitelisted(!whitelisted);
    ui->exclusionButton->setIcon(QIcon::fromTheme(whitelisted ? "list-add" : "process-stop"));
}

QString FolderListItemWidget::getText() {
    return ui->folderNameLabel->text();
}

bool FolderListItemWidget::isWhitelisted() {
    return whitelisted;
}

void FolderListItemWidget::setWhitelisted(bool is_whitelisted) {
    whitelisted = is_whitelisted;
}
