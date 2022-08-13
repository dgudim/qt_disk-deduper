#include "dupes_list.h"
#include "ui_dupes_list.h"

dupes_list::dupes_list(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dupes_list)
{
    ui->setupUi(this);
}

dupes_list::~dupes_list()
{
    delete ui;
}
