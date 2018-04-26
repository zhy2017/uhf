#include "amplifierwidget.h"
#include "ui_amplifierwidget.h"
#include <QDesktopWidget>
#include <QPainter>
#include <QCloseEvent>

AmplifierWidget::AmplifierWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AmplifierWidget)
{
    ui->setupUi(this);
   // this->move((QApplication::desktop()->width() - this->width()) / 2, (QApplication::desktop()->height() - this->height()) / 4);
    this->move((parent->width() - this->width()) / 2, (parent->height() - this->height()) / 2);
    initStyle();
}

AmplifierWidget::~AmplifierWidget()
{
    delete ui;
}

QString AmplifierWidget::showMyMessageBox(QWidget *parent)
{
    AmplifierWidget *myWidget = new AmplifierWidget(parent);
    return myWidget->exec();
}

void AmplifierWidget::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void AmplifierWidget::closeEvent(QCloseEvent *event)
{
    // 关闭窗口时结束事件循环，在exec()方法中返回选择结果;
    if (m_eventLoop != NULL)
    {
        m_eventLoop->exit();
    }
    event->accept();
}

void AmplifierWidget::on_cancelBtn_clicked()
{
    m_resultStr.clear();
    m_resultStr = QString("cancel");
    close();
}

void AmplifierWidget::on_okBtn_clicked()
{
    m_resultStr.clear();
    m_resultStr = QString("ok");
    close();
}

QString AmplifierWidget::exec()
{
    this->setWindowModality(Qt::WindowModal);
    show();
    m_eventLoop = new QEventLoop(this);
    m_eventLoop->exec();
    return m_resultStr;
}

void AmplifierWidget::initStyle()
{
    ui->okBtn->setStyleSheet("QPushButton{border-radius: 4px;border: none;width: 75px;height: 25px;}"
                                 "QPushButton:enabled { background: rgb(68, 69, 73);color: white;}"
                                 "QPushButton:!enabled {background: rgb(100, 100, 100); color:rgb(200, 200, 200);}"
                                 "QPushButton:enabled:hover{ background: rgb(85, 85, 85);}"
                                 "QPushButton:enabled:pressed{ background: rgb(80, 80, 80);}");
    ui->cancelBtn->setStyleSheet("QPushButton{border-radius: 4px;border: none;width: 75px;height: 25px;}"
                                 "QPushButton:enabled { background: rgb(68, 69, 73);color: white;}"
                                 "QPushButton:!enabled {background: rgb(100, 100, 100); color:rgb(200, 200, 200);}"
                                 "QPushButton:enabled:hover{ background: rgb(85, 85, 85);}"
                                 "QPushButton:enabled:pressed{ background: rgb(80, 80, 80);}");
}
