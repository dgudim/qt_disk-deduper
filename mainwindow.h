#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAction>
#include <QFileDialog>
#include <QListWidget>

#include "folder_list_item.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAddScanFolderClicked();
    void onAddSlaveFolderClicked();
    void removeItemFromList(const QString& text, QListWidget* list);
    void onSetMasterFolderClicked();
    void onSetDupesFolderClicked();

private:
    Ui::MainWindow *ui;

    void addItemToList(const QString& text, QListWidget* list);
    QString callFileDialogue(const QString& title, QFileDialog::Options options);
    QListWidgetItem* getFromList(const QString& text, QListWidget* list);
    FolderListItemWidget* widgetFromWidgetItem(QListWidgetItem *item);

    QString masterFolder;
    QString dupesFolder;

};
#endif // MAINWINDOW_H
