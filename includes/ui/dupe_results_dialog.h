#ifndef DUPE_RESULTS_DIALOG_H
#define DUPE_RESULTS_DIALOG_H

#include "datatypes.h"

#include <QDialog>
#include <QButtonGroup>
#include <QRadioButton>

namespace Ui {
    class Dupe_results_dialog;
}

class Dupe_results_dialog : public QDialog {
    Q_OBJECT

public:
    explicit Dupe_results_dialog(QWidget *parent, const MultiFileGroupArray& results, const FileQuantitySizeCounter& total_files);
    ~Dupe_results_dialog();

private:

    void loadTab(const QString& name, const MultiFileGroup& list, pButtonGroups button_groups);

    void loadNTabs(int n = 10);

    ButtonGroupsPerTab button_groups_per_tab;

    QMap<QAbstractButton*, File> button_to_file_map;

    MultiFileGroupArray allGroups;
    int current_group_index = 0;

    Ui::Dupe_results_dialog *ui;
};

#endif // DUPE_RESULTS_DIALOG_H
