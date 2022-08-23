#ifndef DUPE_RESULTS_DIALOG_H
#define DUPE_RESULTS_DIALOG_H

#include "datatypes.h"

#include <QDialog>
#include <QButtonGroup>

template<typename T>
using ptr = std::shared_ptr<T>;

// stores one button group in the tab
typedef ptr<QButtonGroup> ButtonGroup;

// stores all button groups of one tab
typedef QVector<ButtonGroup> ButtonGroups;

// stores all button groups of one tab (pointer)
typedef ptr<ButtonGroups> pButtonGroups;

// stores all button groups of all tabs
typedef QVector<pButtonGroups> ButtonGroupsPerTab;

namespace Ui {
    class Dupe_results_dialog;
}

class Dupe_results_dialog : public QDialog {
    Q_OBJECT

public:
    explicit Dupe_results_dialog(QWidget *parent, const QVector<QVector<QVector<File>>>& results, int total_files, quint64 total_size);
    ~Dupe_results_dialog();

private:

    void loadTab(const QString& name, const QVector<QVector<File>>& list, pButtonGroups button_groups);

    ButtonGroupsPerTab button_groups_per_tab;
    Ui::Dupe_results_dialog *ui;
};

#endif // DUPE_RESULTS_DIALOG_H
