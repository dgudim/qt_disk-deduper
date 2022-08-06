#ifndef FOLDER_LIST_ITEM_H
#define FOLDER_LIST_ITEM_H

#include <QWidget>
#include <QListWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class FolderListItemWidget; }
QT_END_NAMESPACE

class FolderListItemWidget : public QWidget {
    Q_OBJECT

public:
    explicit FolderListItemWidget(QWidget *parent, QListWidget *list);
    ~FolderListItemWidget();

    void setText(const QString &text);
    QString getText();

    void setDisabled(bool disabled);

    bool isWhitelisted();
    void setWhitelisted(bool is_whitelisted);

signals:
    void sendRemoveItem(const QString &text, QListWidget *parent_list);

private slots:
    void onRemoveButtonClicked();
    void onExecutionButtonClicked();

private:
    Ui::FolderListItemWidget *ui;
    bool whitelisted = true;
    QListWidget *parent_list;
};

#endif // FOLDER_LIST_ITEM_H
