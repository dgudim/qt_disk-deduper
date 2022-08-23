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

#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>

#include <QSettings>

#include <functional>

#include "metadata_selection_dialogue.h"
#include "ExifTool.h"
#include "stats_dialog.h"
#include "dupe_results_dialog.h"
#include "folder_list_item.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

enum class EtaMode {
    DISABLED,
    ENABLED
};

enum class FileField {
    HASH,
    NAME
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void hashCompare(QSqlDatabase db);
    void nameCompare(QSqlDatabase);
    void autoDedupeMove(QSqlDatabase db);
    void autoDedupeRename(QSqlDatabase db);
    void exifRename(QSqlDatabase);
    void showStats(QSqlDatabase db);

    void fileCompare_display();
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

    void addWidgetToList(QListWidget *list, FolderListItemWidget *widget);

    QListWidgetItem* getFromList(const QString& text, QListWidget* list);
    QStringList getAllStringsFromList(QListWidget *list);
    QVector<FolderListItemData> getAllWidgetsDataFromList(QListWidget *list);
    void setListItemsDisabled(QListWidget *list, bool disable);
    FolderListItemWidget* widgetFromList(QListWidget *list, int undex);

    void saveList(const QString &key, QListWidget *list);
    void loadList(const QString &key, QListWidget *list);

    QString callDirSelectionDialogue(const QString &title);
    QStringList callMultiDirSelectionDialogue();
    QString callTextDialogue(const QString &title, const QString &prompt);

    template<FileField field>
    void findDuplicateFiles(QSqlDatabase db);

    void addEnumeratedFile(const QString& file);
    void displayWarning(const QString &message);

    bool startScanAsync();
    void hashAllFiles(QSqlDatabase db);

    void setUiDisabled(bool state);

    // status variables
    quint64 lastMeasuredDiskRead = 0;
    float averageDiskReadSpeed = 0;
    quint64 program_start_time = 0;
    quint64 scan_start_time = 0;
    bool scan_active = false;
    EtaMode etaMode = EtaMode::DISABLED;
    int currentMode;
    qint32 processed_files = 0; // max 4294967295
    qint32 previous_processed_files = 0; // for measuring speed
    qint32 duplicate_files = 0;
    qint32 preloaded_files = 0;
    float averageFilesPerSecond = 0;
    quint64 files_size_all = 0;
    quint64 files_size_dupes = 0;
    quint64 files_size_processed = 0;
    quint64 files_size_preloaded = 0;

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

    // dedupe results
    QVector<QVector<QVector<File>>> dedupe_resuts;
    Dupe_results_dialog *dupe_results_dialog;

    // utility functions
    template<typename T>
    void removeDuplicates(T &arr);
};
#endif // MAINWINDOW_H
