#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAction>
#include <QTime>
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

    void onSetMasterFolderClicked();
    void onSetDupesFolderClicked();

    void onAddExtensionButtonClicked();

    void onExtentionCheckboxStateChanged(int arg1);

    void removeItemFromList(const QString& text, QListWidget* list);

private:
    Ui::MainWindow *ui;

    void updateLoop100Ms();
    void updateLoop3s();

    void addItemToList(const QString& text, QListWidget* list);
    QListWidgetItem* getFromList(const QString& text, QListWidget* list);
    FolderListItemWidget* widgetFromWidgetItem(QListWidgetItem *item);

    QString callFileDialogue(const QString& title, QFileDialog::Options options);
    QString callTextDialogue(const QString &title, const QString &prompt);

    void displayWarning(const QString &message);

    void setCurrentStatus(const QString &status);

    unsigned long lastMeasuredDiskRead;
    unsigned long program_start_time;

    QString masterFolder;
    QString dupesFolder;

    QStringList directories_to_scan;
    QStringList all_files;
    QMap<QString, QList<QString>> dupes;

};
#endif // MAINWINDOW_H
