#ifndef FOLDER_LIST_ITEM_H
#define FOLDER_LIST_ITEM_H

#include <QWidget>
#include <QListWidget>

#include <QMetaType>
#include <QVariant>
#include <QDataStream>

QT_BEGIN_NAMESPACE
namespace Ui { class FolderListItemWidget; }
QT_END_NAMESPACE

struct FolderListItemData {
    QString text;
    bool canBlacklist;
    bool whitelisted;

    friend QDataStream& operator<<(QDataStream& out, const FolderListItemData& data);
    friend QDataStream& operator>>(QDataStream& in, FolderListItemData& data);
};

Q_DECLARE_METATYPE(FolderListItemData);

class FolderListItemWidget : public QWidget {
    Q_OBJECT

public:
    explicit FolderListItemWidget(QWidget *parent, QListWidget *parent_list, bool canBlacklist);
    explicit FolderListItemWidget(QWidget *parent, QListWidget* parent_list, const FolderListItemData &data);

    ~FolderListItemWidget();

    void setText(const QString &text);
    QString getText();

    void setDisabled(bool disabled);

    bool isWhitelisted();
    void setWhitelisted(bool is_whitelisted);

    FolderListItemData getData();

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
