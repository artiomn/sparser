/********************************************************************************
** Form generated from reading UI file 'sparser_gui.ui'
**
** Created: Wed Dec 26 10:21:20 2012
**      by: Qt User Interface Compiler version 4.6.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SPARSER_GUI_H
#define UI_SPARSER_GUI_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCommandLinkButton>
#include <QtGui/QFormLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QMainWindow>
#include <QtGui/QStatusBar>
#include <QtGui/QToolBar>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_sparser_gui
{
public:
    QWidget *centralWidget;
    QWidget *formLayoutWidget;
    QFormLayout *formLayout;
    QCommandLinkButton *clbtnStartStop;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *sparser_gui)
    {
        if (sparser_gui->objectName().isEmpty())
            sparser_gui->setObjectName(QString::fromUtf8("sparser_gui"));
        sparser_gui->resize(600, 400);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(1);
        sizePolicy.setHeightForWidth(sparser_gui->sizePolicy().hasHeightForWidth());
        sparser_gui->setSizePolicy(sizePolicy);
        sparser_gui->setAcceptDrops(false);
        sparser_gui->setInputMethodHints(Qt::ImhNone);
        sparser_gui->setDocumentMode(false);
        sparser_gui->setTabShape(QTabWidget::Rounded);
        sparser_gui->setUnifiedTitleAndToolBarOnMac(false);
        centralWidget = new QWidget(sparser_gui);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        centralWidget->setMinimumSize(QSize(500, 350));
        formLayoutWidget = new QWidget(centralWidget);
        formLayoutWidget->setObjectName(QString::fromUtf8("formLayoutWidget"));
        formLayoutWidget->setGeometry(QRect(0, 0, 596, 371));
        formLayout = new QFormLayout(formLayoutWidget);
        formLayout->setSpacing(6);
        formLayout->setContentsMargins(11, 11, 11, 11);
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        formLayout->setFormAlignment(Qt::AlignRight|Qt::AlignTop|Qt::AlignTrailing);
        formLayout->setContentsMargins(0, 0, 0, 0);
        clbtnStartStop = new QCommandLinkButton(formLayoutWidget);
        clbtnStartStop->setObjectName(QString::fromUtf8("clbtnStartStop"));
        clbtnStartStop->setMouseTracking(false);
        clbtnStartStop->setLayoutDirection(Qt::LeftToRight);
        clbtnStartStop->setAutoFillBackground(false);
        clbtnStartStop->setStyleSheet(QString::fromUtf8(""));

        formLayout->setWidget(0, QFormLayout::FieldRole, clbtnStartStop);

        sparser_gui->setCentralWidget(centralWidget);
        mainToolBar = new QToolBar(sparser_gui);
        mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
        sparser_gui->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(sparser_gui);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        sparser_gui->setStatusBar(statusBar);

        retranslateUi(sparser_gui);

        QMetaObject::connectSlotsByName(sparser_gui);
    } // setupUi

    void retranslateUi(QMainWindow *sparser_gui)
    {
        sparser_gui->setWindowTitle(QApplication::translate("sparser_gui", "sparser_gui", 0, QApplication::UnicodeUTF8));
        clbtnStartStop->setText(QApplication::translate("sparser_gui", "\320\241\321\202\320\260\321\200\321\202/\320\241\321\202\320\276\320\277", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class sparser_gui: public Ui_sparser_gui {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SPARSER_GUI_H
