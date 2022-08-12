#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAction>
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

#include "metadata_selection_dialogue.h"
#include "ExifTool.h"
#include "stats_dialog.h"
#include "folder_list_item.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

enum EtaMode {
    OFF,
    SPEED_BASED,
    ITEM_BASED
};

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

    void hashCompare_display();
    void nameCompare_display();
    void autoDedupeMove_display();
    void autoDedupeRename_display();
    void exifRename_display();
    void showStats_display();

    bool showStats_request();

    // for displaying log messages in the ui
    static MainWindow *this_window;

public slots:
    static void appendToLog(const QString &msg, bool error);

private slots:
    void onAddScanFolderClicked();
    void onAddSlaveFolderClicked();

    void onSetMasterFolderClicked();
    void onSetDupesFolderClicked();

    void onCurrentModeChanged(int curr_mode);

    void onStartScanButtonClicked();

    void onAddExtensionButtonClicked();
    void onExtentionCheckboxStateChanged(int arg1);

    void removeItemFromList(const QString &text, QListWidget *list);

    void setCurrentTask(const QString &status);

private:
    Ui::MainWindow *ui;

    void updateLoop100Ms();
    void updateLoop2s();

    void addItemsToList(QString text, QListWidget *list, bool canBlacklist = true, bool lowercase = false);
    void addItemsToList(const QStringList &items, QListWidget *list, bool canBlacklist = true, bool lowercase = false);

    QListWidgetItem* getFromList(const QString& text, QListWidget* list);
    void setListItemsDisabled(QListWidget* list, bool disable);
    FolderListItemWidget* widgetFromWidgetItem(QListWidgetItem *item);

    QString callDirSelectionDialogue(const QString &title);
    QStringList callMultiDirSelectionDialogue();
    QString callTextDialogue(const QString &title, const QString &prompt);

    void addEnumeratedFile(const QString& file);
    void displayWarning(const QString &message);

    bool startScanAsync();
    void hashAllFiles();

    void setUiDisabled(bool state);

    // status variables
    quint64 lastMeasuredDiskRead = 0;
    float averageDiskReadSpeed = 0;
    quint64 program_start_time = 0;
    quint64 scan_start_time = 0;
    bool scan_active = false;
    EtaMode etaMode = OFF;
    int currentMode;
    quint32 processed_files = 0; // max 4294967295
    quint32 previous_processed_files = 0; // for measuring speed
    float averageFilesPerSecond = 0;
    quint64 files_size_all = 0;
    quint64 files_size_processed = 0;

    QPieSeries *series;

    QString masterFolder;
    QString dupesFolder;

    // general variables
    QVector<File> unique_files;

    QStringList directories_to_scan;

    // metadata extraction
    StatsContainer stat_results;
    QVector<QString> selectedMetaFields;
    Stats_dialog *stats_dialog;
    Metadata_selection_dialogue *selection_dialogue;
    ExifTool *ex_tool;

    // utility functions
    template<typename T>
    void removeDuplicates(T &arr);
};
#endif // MAINWINDOW_H
