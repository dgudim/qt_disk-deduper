#ifndef DUPES_LIST_H
#define DUPES_LIST_H

#include <QDialog>

namespace Ui {
    class dupes_list;
}

class dupes_list : public QDialog
{
    Q_OBJECT

public:
    explicit dupes_list(QWidget *parent = nullptr);
    ~dupes_list();

private:
    Ui::dupes_list *ui;
};

#endif // DUPES_LIST_H
