#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAction>
#include <QTime>
#include <QTime>
#include <QFileDialog>
#include <QListWidget>

#include <QtCharts>
#include <QChartView>
#include <QPieSeries>
#include <QPieSlice>

#include <QtConcurrent/QtConcurrent>
#include <QThread>
#include <QMetaMethod>
#include <QDateTime>
#include <QMessageBox>
#include <QtDebug>

#include <functional>

#include "folder_list_item.h"
#include "gutils.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void hashCompare();
    void nameCompare();
    void autoDedupeMove();
    void autoDedupeRename();
    void exifRename();
    void showStats();

private slots:
    void onAddScanFolderClicked();
    void onAddSlaveFolderClicked();

    void onSetMasterFolderClicked();
    void onSetDupesFolderClicked();

    void onCurrentModeChanged(int curr_mode);

    void onStartScanButtonClicked();

    void onAddExtensionButtonClicked();
    void onExtentionCheckboxStateChanged(int arg1);

    void removeItemFromList(const QString& text, QListWidget* list);

    void addEnumeratedFile(const QString& file);

    void setCurrentTask(const QString &status);
    void setLastMessage(const QString &status);

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

    void startScanAsync();

    void hashAllFiles();

    unsigned long lastMeasuredDiskRead;
    unsigned long program_start_time;

    int currentMode;

    QPieSeries *series;

    QString masterFolder;
    QString dupesFolder;

    QVector<File> unique_files;
    unsigned int hashed_files = 0;

    QStringList directories_to_scan;
    QMap<QString, QVector<QString>> dupes;

};
#endif // MAINWINDOW_H
