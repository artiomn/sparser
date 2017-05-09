#include <QtGui>

#include "sparser_gui.h"
#include "ui_sparser_gui.h"

sparser_gui::sparser_gui(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::sparser_gui)
{
    ui->setupUi(this);

//    ui->centralWidget->
}

sparser_gui::~sparser_gui()
{
    delete ui;
}

void sparser_gui::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }

}

void sparser_gui::on_clbtnStartStop_clicked()
{

}
