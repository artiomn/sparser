#ifndef SPARSER_GUI_H
#define SPARSER_GUI_H

#include <QMainWindow>

namespace Ui {
    class sparser_gui;
}

class sparser_gui : public QMainWindow {
    Q_OBJECT
public:
    sparser_gui(QWidget *parent = 0);
    ~sparser_gui();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::sparser_gui *ui;

private slots:


private slots:
    void on_clbtnStartStop_clicked();
};

#endif // SPARSER_GUI_H
