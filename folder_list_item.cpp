#include "folder_list_item.h"
#include "ui_folder_list_item.h"

QDataStream& operator<<(QDataStream& out, const FolderListItemData& data) {
    out << data.text << data.canBlacklist << data.whitelisted;
    return out;
}

QDataStream& operator>>(QDataStream& in, FolderListItemData& data) {
    in >> data.text >> data.canBlacklist >> data.whitelisted;
    return in;
}

FolderListItemWidget::FolderListItemWidget(QWidget *parent, QListWidget* parent_list, bool canBlacklist)
    : QWidget(parent), ui(new Ui::FolderListItemWidget) {

    this->parent_list = parent_list;
    ui->setupUi(this);
    ui->exclusionButton->setHidden(!canBlacklist);

    connect(ui->exclusionButton, SIGNAL(clicked()), this, SLOT(onExecutionButtonClicked()));
    connect(ui->deleteButton, SIGNAL(clicked()), this, SLOT(onRemoveButtonClicked()));
    connect(this, SIGNAL(sendRemoveItem(QString,QListWidget*)), parent, SLOT(removeItemFromList(QString,QListWidget*)));
}

FolderListItemWidget::FolderListItemWidget(QWidget *parent, QListWidget* parent_list, const FolderListItemData &data)
    : FolderListItemWidget(parent, parent_list, data.canBlacklist) {

    setText(data.text);
    setWhitelisted(data.whitelisted);
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
}

QString FolderListItemWidget::getText() {
    return ui->folderNameLabel->text();
}

void FolderListItemWidget::setDisabled(bool disabled) {
    ui->deleteButton->setDisabled(disabled);
    ui->exclusionButton->setDisabled(disabled);
}

bool FolderListItemWidget::isWhitelisted() {
    return whitelisted;
}

void FolderListItemWidget::setWhitelisted(bool is_whitelisted) {
    whitelisted = is_whitelisted;
    ui->exclusionButton->setIcon(QIcon::fromTheme(whitelisted ? "list-add" : "process-stop"));
}

FolderListItemData FolderListItemWidget::getData() {
    return { getText(), !ui->exclusionButton->isHidden(), whitelisted };
}
