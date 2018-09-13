#include "widget.h"
#include "ui_widget.h"
#include "SendTask.h"
#include <QDebug>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QFile>
#include <QFileDialog>
#include <QDataStream>
#include <QtAlgorithms>
#include "amplifierwidget.h"
#include "clickedlabel.h"
#include <QSettings>
#include <windows.h>
#include <QKeyEvent>
#include <QProcess>

#define REG_RUN "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinimizeButtonHint);
    initForm();
    initStopData();
    initExcelData();
    initseData();

    lineSelect = 0;//没有输入框选中 1 = 发送总次数 2 = 发送间隔1 3 = 发送总时间 4 = 发送间隔2

    ui->countEdit->installEventFilter(this);
    ui->interval1Edit->installEventFilter(this);
    ui->timeEdit->installEventFilter(this);
    ui->interval2Edit->installEventFilter(this);
    ui->delayEdit->installEventFilter(this);

     m_cmd = CMD_NORMAL;
     m_bInit = false;
     waitFMC = false;
     startOrStop = false;
     sendFlag = false;
     serialFlag = false;
     freFlag = false;
     serialfreFlag = false;
     calibrationFlag = false;
     calibrationFlag2 = false;
     serialFlag1 = false;
     sendCalibration = false;

     loadSize = 4*1024;
     totalBytes = 0;
     bytesWritten = 0;
     bytesToWrite = 0;
     preFre = 0;
     endFlag = false;
     tcpClient = new QTcpSocket(this);
     cmd = new QProcess;

     //当服务器连接成功时，发出connected()信号，我们开始传送文件
     connect(tcpClient, SIGNAL(connected()), this, SLOT(sendData()));
     //当有数据发送成功时，我们更新进度条
     connect(tcpClient, SIGNAL(bytesWritten(qint64)), this, SLOT(updateClientProgress(qint64)));
     //处理接收返回的数据
      connect(tcpClient, SIGNAL(readyRead()), this, SLOT(receiveData()));

      setClient = new QTcpSocket(this);
      connect(setClient, SIGNAL(connected()), this, SLOT(startSet()));
      connect(setClient, SIGNAL(readyRead()), this, SLOT(receiveSetData()));

      connect(ui->label, SIGNAL(Clicked()), this, SLOT(fwidgetClick()));
      connect(ui->label_2, SIGNAL(Clicked()), this, SLOT(dwidgetClick()));

      this->move(0, 0);

      setAutoStart(true);//设置开机自启动

      this->showFullScreen();
      m_timerId = startTimer(200);
      m_aliveId = startTimer(1000);//开启维持心跳定时器
}


Widget::~Widget()
{
    delete ui;
}

void Widget::formatString(QString &org, int n, const QChar &ch)
{
    int size= org.size();
    int space = qRound(size * 1.0 / n + 0.5) - 1;
    if(space <= 0)
        return;
    for(int i = 0, pos = n; i < space; ++i, pos += (n + 1))
    {
        org.insert(pos, ch);
    }
}

QByteArray Widget::hexStringtoByteArray(QString hex)
{
    QByteArray ret;
    hex = hex.trimmed();
    formatString(hex, 2, ' ');
    QStringList sl = hex.split(" ");
    foreach(QString s, sl)
    {
        if(!s.isEmpty())
            ret.append((char)s.toInt(0, 16) & 0xFF);
    }
    return ret;
}

void Widget::timerEvent(QTimerEvent *event)
{
    if(event->timerId() == m_timerId)
    {
        if(ui->amplifierClose->isChecked())
        {
            QString str_value = QString::number(ui->dwidget1->getCurrentIndex()) + QString::number(ui->dwidget2->getCurrentIndex()) + "." + QString::number(ui->dwidget3->getCurrentIndex());
            double value = str_value.toDouble();
            if(ui->label_4->text() != "-")
            {
                ui->dwidget1->setCurrentIndex(0);
                ui->dwidget2->setCurrentIndex(0);
                ui->dwidget3->setCurrentIndex(0);
            }
            else
            {
                if(value > 40.0)
                {
                    ui->dwidget1->setCurrentIndex(4);
                    ui->dwidget2->setCurrentIndex(0);
                    ui->dwidget3->setCurrentIndex(0);
                }
            }
        }
        else
        {
             QString str_value = QString::number(ui->dwidget1->getCurrentIndex()) + QString::number(ui->dwidget2->getCurrentIndex()) + "." + QString::number(ui->dwidget3->getCurrentIndex());
             double value = str_value.toDouble();
             if(ui->label_4->text() == "-")
             {
                 if(value > 10.0)
                 {
                     ui->dwidget1->setCurrentIndex(1);
                     ui->dwidget2->setCurrentIndex(0);
                     ui->dwidget3->setCurrentIndex(0);
                 }
             }
             else
             {
                 if(value > 37.0)
                 {
                     ui->dwidget1->setCurrentIndex(3);
                     ui->dwidget2->setCurrentIndex(7);
                     ui->dwidget3->setCurrentIndex(0);
                 }
             }
        }
    }
    else if(event->timerId() == m_aliveId)
    {
        QString strArg = "ping 192.168.0.10 -n 1 -w 2000";
        cmd->start(strArg);
        cmd->waitForReadyRead();
        cmd->waitForFinished();
        QString retStr = cmd->readAll();
        if (retStr.indexOf("TTL") != -1)
        {
            QListWidgetItem *item = ui->listWidget->item(0);
            item->setText(QString("设备在线"));
            initDevice();
        }
        else
        {
            QListWidgetItem *item = ui->listWidget->item(0);
            item->setText(QString("设备离线"));
        }
    }
}

bool Widget::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == ui->countEdit)
    {
        if(event->type() == QEvent::FocusIn)
        {
            lineSelect = 1;
        }
    }
    else if(watched == ui->interval1Edit)
    {
        if(event->type() == QEvent::FocusIn)
        {
            lineSelect = 2;
        }
    }
    else if(watched == ui->timeEdit)
    {
        if(event->type() == QEvent::FocusIn)
        {
            lineSelect = 3;
        }
    }
    else if(watched == ui->interval2Edit)
    {
        if(event->type() == QEvent::FocusIn)
        {
            lineSelect = 4;
        }
    }
    else if(watched == ui->delayEdit)
    {
        if(event->type() == QEvent::FocusIn)
        {
            lineSelect = 5;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void Widget::initForm()
{
    ui->comboBox->setStyleSheet("QComboBox {height: 25px; border-radius: 4px; border: 1px solid rgb(111, 156, 207);background: white;}"
                        "QComboBox:enabled { color: rgb(84, 84, 84);}"
                        "QComboBox:!enabled { color: rgb(80, 80, 80);}"
                        "QComboBox:enabled:hover, QComboBox:enabled:focus {color: rgb(51, 51, 51);}"
                        "QComboBox::drop-down {width: 20px; border: none;background: transparent;}"
                        "QComboBox::drop-down:hover {background: rgba(255, 255, 255, 30);}"
                        "QComboBox::down-arrow {image: url(:/image/arrowBottom.png);}"
                        "QComboBox::down-arrow:on { /**top: 1px;**/}"
                        "QComboBox QAbstractItemView {border: 1px solid rgb(111, 156, 207);background: white; outline: none;}"
                        "QComboBox QAbstractItemView::item { height: 25px; color: rgb(73, 73, 73);}"
                        "QComboBox QAbstractItemView::item:selected {background: rgb(232, 241, 250); color: rgb(2, 65, 132);}");
    ui->loopRadio->setStyleSheet("QRadioButton{background: transparent;color:white;} "
                                   "QRadioButton:indicator{width:20px;height:20px;}"
                                   "QRadioButton:indicator:unchecked{image:url(:/image/radio2.png)}"
                                   "QRadioButton:indicator:checked{image:url(:/image/radio3.png)}");
    ui->countRadio->setStyleSheet("QRadioButton{background: transparent;color:white;} "
                                   "QRadioButton:indicator{width:20px;height:20px;}"
                                   "QRadioButton:indicator:unchecked{image:url(:/image/radio2.png)}"
                                   "QRadioButton:indicator:checked{image:url(:/image/radio3.png)}");
    ui->timeRadio->setStyleSheet("QRadioButton{background: transparent;color:white;} "
                                   "QRadioButton:indicator{width:20px;height:20px;}"
                                   "QRadioButton:indicator:unchecked{image:url(:/image/radio2.png)}"
                                   "QRadioButton:indicator:checked{image:url(:/image/radio3.png)}");

    ui->amplifierOpen->setStyleSheet("QRadioButton{background: transparent;color:white;} "
                                   "QRadioButton:indicator{width:35px;height:35px;}"
                                   "QRadioButton:indicator:unchecked{image:url(:/image/radio1.png)}"
                                   "QRadioButton:indicator:checked{image:url(:/image/radioselect.png)}");

    ui->amplifierClose->setStyleSheet("QRadioButton{background: transparent;color:white;} "
                                   "QRadioButton:indicator{width:35px;height:35px;}"
                                   "QRadioButton:indicator:unchecked{image:url(:/image/radio1.png)}"
                                   "QRadioButton:indicator:checked{image:url(:/image/radioselect.png)}");

    ui->listWidget->setStyleSheet("QListWidget{color:white;}"
                               "QListWidget::Item:hover{background:transparent; }"
                               "QListWidget::item:selected{background:transparent;color:white;}"
                               "QListWidget::item:selected:!active{ background:transparent; }"
                               );
    ui->listWidget->setFocusPolicy(Qt::NoFocus);


    ui->openBtn->setStyleSheet("QPushButton:enabled {background: rgb(34, 55, 76);color: rgb(0, 141, 212);}");
    ui->num7Btn->setStyleSheet("QPushButton:enabled {background: rgb(34, 55, 76);color: white;}");
    ui->num8Btn->setStyleSheet("QPushButton:enabled {background: rgb(34, 55, 76);color: white;}");
    ui->num9Btn->setStyleSheet("QPushButton:enabled {background: rgb(34, 55, 76);color: white;}");
    ui->num4Btn->setStyleSheet("QPushButton:enabled {background: rgb(34, 55, 76);color: white;}");
    ui->num5Btn->setStyleSheet("QPushButton:enabled {background: rgb(34, 55, 76);color: white;}");
    ui->num6Btn->setStyleSheet("QPushButton:enabled {background: rgb(34, 55, 76);color: white;}");
    ui->num1Btn->setStyleSheet("QPushButton:enabled {background: rgb(34, 55, 76);color: white;}");
    ui->num2Btn->setStyleSheet("QPushButton:enabled {background: rgb(34, 55, 76);color: white;}");
    ui->num3Btn->setStyleSheet("QPushButton:enabled {background: rgb(34, 55, 76);color: white;}");
    ui->num0Btn->setStyleSheet("QPushButton:enabled {background: rgb(34, 55, 76);color: white;}");
    ui->negativeBtn->setStyleSheet("QPushButton:enabled {background: rgb(34, 55, 76);color: white;}");
    ui->delBtn->setStyleSheet("QPushButton:enabled {background: rgb(34, 55, 76);color: white;}");
    ui->num000Btn->setStyleSheet("QPushButton:enabled {background: rgb(34, 55, 76);color: white;}");
    ui->acBtn->setStyleSheet("QPushButton:enabled {background: rgb(34, 55, 76);color: white;}");
    ui->okBtn->setStyleSheet("QPushButton:enabled {background: rgb(34, 55, 76);color: white;}");

    ui->upBtn->setFixedSize(40, 40);
    ui->upBtn->setStyleSheet("QPushButton{border-image:url(:/image/up.png);}"
                              "QPushButton::hover{border-image:url(:/image/up1.png);}"
                              "QPushButton::pressed{border-image:url(:/image/up.png);}");

    ui->downBtn->setFixedSize(40, 40);
    ui->downBtn->setStyleSheet("QPushButton{border-image:url(:/image/down.png);}"
                              "QPushButton::hover{border-image:url(:/image/down1.png);}"
                              "QPushButton::pressed{border-image:url(:/image/down.png);}");

    ui->leftBtn->setFixedSize(40, 40);
    ui->leftBtn->setStyleSheet("QPushButton{border-image:url(:/image/left.png);}"
                              "QPushButton::hover{border-image:url(:/image/left1.png);}"
                              "QPushButton::pressed{border-image:url(:/image/left.png);}");

    ui->rightBtn->setFixedSize(40, 40);
    ui->rightBtn->setStyleSheet("QPushButton{border-image:url(:/image/right.png);}"
                              "QPushButton::hover{border-image:url(:/image/right1.png);}"
                              "QPushButton::pressed{border-image:url(:/image/right.png);}");

    maxIndex = 11;
    currentIndex = 1;

    m_index.insert(1, ui->fwidget1);
    m_index.insert(2, ui->fwidget2);
    m_index.insert(3, ui->fwidget3);
    m_index.insert(4, ui->fwidget4);
    m_index.insert(5, ui->fwidget5);
    m_index.insert(6, ui->fwidget6);
    m_index.insert(7, ui->fwidget7);
    m_index.insert(8, ui->fwidget8);
    m_index.insert(9, ui->dwidget1);
    m_index.insert(10, ui->dwidget2);
    m_index.insert(11, ui->dwidget3);

    ui->fwidget1->setTextColor(QColor(255, 255, 255));
    ui->fwidget2->setTextColor(QColor(255, 255, 255));
    ui->fwidget3->setTextColor(QColor(255, 255, 255));
    ui->fwidget4->setTextColor(QColor(255, 255, 255));
    ui->fwidget5->setTextColor(QColor(255, 255, 255));
    ui->fwidget6->setTextColor(QColor(255, 255, 255));
    ui->fwidget7->setTextColor(QColor(255, 255, 255));
    ui->fwidget8->setTextColor(QColor(255, 255, 255));
    ui->dwidget1->setTextColor(QColor(255, 255, 255));
    ui->dwidget2->setTextColor(QColor(255, 255, 255));
    ui->dwidget3->setTextColor(QColor(255, 255, 255));
    ui->dwidget1->setLineColor(QColor(255, 255, 0));
    ui->dwidget2->setLineColor(QColor(255, 255, 0));
    ui->dwidget3->setLineColor(QColor(255, 255, 0));

    QStringList listValue;

    for (int i = 0; i <= 9; i++) {
        listValue.append(QString::number(i));
    }

    QStringList listValue1;
    for(int i = 0; i <= 3; i++)
    {
        listValue1.append(QString::number(i));
    }

    ui->fwidget1->setListValue(listValue1);
    ui->fwidget1->setCurrentIndex(1);
    ui->fwidget2->setListValue(listValue);
    ui->fwidget2->setCurrentIndex(0);
    ui->fwidget3->setListValue(listValue);
    ui->fwidget3->setCurrentIndex(0);
    ui->fwidget4->setListValue(listValue);
    ui->fwidget4->setCurrentIndex(0);
    ui->fwidget5->setListValue(listValue);
    ui->fwidget5->setCurrentIndex(0);
    ui->fwidget6->setListValue(listValue);
    ui->fwidget6->setCurrentIndex(0);
    ui->fwidget7->setListValue(listValue);
    ui->fwidget7->setCurrentIndex(0);
    ui->fwidget8->setListValue(listValue);
    ui->fwidget8->setCurrentIndex(0);
    ui->dwidget1->setListValue(listValue);
    ui->dwidget1->setCurrentIndex(1);
    ui->dwidget2->setListValue(listValue);
    ui->dwidget2->setCurrentIndex(0);
    ui->dwidget3->setListValue(listValue);
    ui->dwidget3->setCurrentIndex(0);

    ui->fwidget1->setLineFlag(true);
    ui->fwidget1->update();

    connect(ui->fwidget1, SIGNAL(Clicked()), this, SLOT(freWidget1Click()));
    connect(ui->fwidget2, SIGNAL(Clicked()), this, SLOT(freWidget2Click()));
    connect(ui->fwidget3, SIGNAL(Clicked()), this, SLOT(freWidget3Click()));
    connect(ui->fwidget4, SIGNAL(Clicked()), this, SLOT(freWidget4Click()));
    connect(ui->fwidget5, SIGNAL(Clicked()), this, SLOT(freWidget5Click()));
    connect(ui->fwidget6, SIGNAL(Clicked()), this, SLOT(freWidget6Click()));
    connect(ui->fwidget7, SIGNAL(Clicked()), this, SLOT(freWidget7Click()));
    connect(ui->fwidget8, SIGNAL(Clicked()), this, SLOT(freWidget8Click()));
    connect(ui->dwidget1, SIGNAL(Clicked()), this, SLOT(freWidget9Click()));
    connect(ui->dwidget2, SIGNAL(Clicked()), this, SLOT(freWidget10Click()));
    connect(ui->dwidget3, SIGNAL(Clicked()), this, SLOT(freWidget11Click()));

    QPalette p;
    p.setColor(QPalette::Highlight, QColor(100, 184, 255));
    ui->roundBar1->setPalette(p);
    ui->roundBar1->setBarStyle(ProgressBarRound::BarStyle_Line);
    ui->roundBar1->setTextHeight(80);
    ui->roundBar1->setMyText(QString("已下载"));
    ui->roundBar1->setOutlinePenWidth(8);
    ui->roundBar1->setDataPenWidth(6);
    ui->roundBar1->setClockWise(false);
    ui->roundBar1->setValue(0);

    ui->openBtn->setStyleSheet("QPushButton{border-radius: 4px;border: none;width: 75px;height: 25px;}"
                                 "QPushButton:enabled { background: rgb(68, 69, 73);color: white;}"
                                 "QPushButton:!enabled {background: rgb(100, 100, 100); color:rgb(200, 200, 200);}"
                                 "QPushButton:enabled:hover{ background: rgb(85, 85, 85);}"
                                 "QPushButton:enabled:pressed{ background: rgb(80, 80, 80);}");

    ui->loopRadio->setChecked(true);
    on_loopRadio_clicked();
    ui->amplifierClose->setChecked(true);
    ui->delayEdit->setText("0");
}

void Widget::initStopData()
{
    QByteArray datagram;
    datagram.resize(64);
    for(int i = 0; i < 8; i++)
    {
        datagram[i] = 0x1A;
    }

    datagram[8] = 0xFF;
    datagram[9] = 0xFF;
    datagram[10] = 0x01;
    datagram[11] = 0x00;

    datagram[12] = 0xFF;
    datagram[13] = 0xFF;
    datagram[14] = 0x02;
    datagram[15] = 0x00;

    datagram[16] = 0xFF;
    datagram[17] = 0xFF;
    datagram[18] = 0x03;
    datagram[19] = 0x00;

    datagram[20] = 0xFF;
    datagram[21] = 0xFF;
    datagram[22] = 0x04;
    datagram[23] = 0x00;

    datagram[24] = 0xFF;
    datagram[25] = 0xFF;
    datagram[26] = 0x05;
    datagram[27] = 0x00;

    datagram[28] = 0xFF;
    datagram[29] = 0xFF;
    datagram[30] = 0x06;
    datagram[31] = 0x00;

    datagram[32] = 0x01;
    datagram[33] = 0x00;
    datagram[34] = 0x07;
    datagram[35] = 0x00;

    if(ui->amplifierClose->isChecked())
    {
        datagram[36] = 0x00;
    }
    else
    {
        datagram[36] = 0xFF;
    }
    datagram[37] = 0x00;
    datagram[38] = 0x08;
    datagram[39] = 0x00;

    datagram[40] = 0xFF;
    datagram[41] = 0xFF;
    datagram[42] = 0x09;
    datagram[43] = 0x00;

    datagram[44] = 0xFF;
    datagram[45] = 0xFF;
    datagram[46] = 0x0A;
    datagram[47] = 0x00;

    datagram[48] = 0xFF;
    datagram[49] = 0xFF;
    datagram[50] = 0x0B;
    datagram[51] = 0x00;

    datagram[52] = 0xFF;
    datagram[53] = 0xFF;
    datagram[54] = 0x0C;
    datagram[55] = 0x00;

    for(int i = 0; i < 8; i++)
    {
        datagram[i + 56] = 0xAA;
    }

    stopData.clear();
    stopData = datagram;
    stopData[32] = 0x00;
    stopData[36] = 0x00;
}

void Widget::initExcelData()
{
    QString line;
    QFile file("data.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }

    QTextStream in(&file);
    line = in.readLine();
    m_pressValue = line.toInt();
    line = in.readLine();
    QStringList list1 = line.split(',');
    QString key = list1.at(0);
    QString value = list1.at(1);
    freValue = value.toInt();
    while (!line.isNull())
    {
        line = in.readLine();
        if(!line.isNull())
        {
            list1.clear();
            list1 = line.split(',');
            key = list1.at(0);
            value = list1.at(1);
            m_excelData.insert(key.toInt(), value.toInt());
        }
    }
}

void Widget::initseData()
{
    seData.clear();
    seData.resize(28);

    for(int i = 0; i < 8; i++)
    {
        seData[i] = 0x0A;
    }

    seData[8] = 0x01;
    seData[9] = 0x00;
    seData[10] = 0x00;
    seData[11] = 0x00;

    seData[12] = 0x9F;
    seData[13] = 0x00;
    seData[14] = 0x00;
    seData[15] = 0x00;

    seData[16] = 0x03;
    seData[17] = 0x00;
    seData[18] = 0x00;
    seData[19] = 0x00;

    for(int i = 0; i < 8; i++)
    {
        seData[i + 20] = 0xAA;
    }
}

int Widget::getAmplitude(int fre)
{
    QMap<int, int>::iterator it;
    int value;
    for(it = m_excelData.begin(); it != m_excelData.end(); it++)
    {
        if(fre < (it.key() * 10000))
        {
            if(it != m_excelData.begin())
            {
                it--;
                value = it.value();
                 qDebug() << "value" << fre;
                return value;
            }
            else
            {
                value = it.value();
                 qDebug() << "value" << fre;
                return value;
            }
        }
    }

    value = m_excelData.last();
    return value;
   // it = m_excelData.keyEnd();
   // value = it.value();
}

void Widget::gpio_send()
{
    if(ui->loopRadio->isChecked())
    {
        QByteArray datagram;
        datagram.resize(64);
        for(int i = 0; i < 8; i++)
        {
            datagram[i] = 0x1A;
        }

        datagram[8] = 0xFF;
        datagram[9] = 0xFF;
        datagram[10] = 0x01;
        datagram[11] = 0x00;

        datagram[12] = 0xFF;
        datagram[13] = 0xFF;
        datagram[14] = 0x02;
        datagram[15] = 0x00;

        datagram[16] = 0xFF;
        datagram[17] = 0xFF;
        datagram[18] = 0x03;
        datagram[19] = 0x00;

        datagram[20] = 0xFF;
        datagram[21] = 0xFF;
        datagram[22] = 0x04;
        datagram[23] = 0x00;

        datagram[24] = 0xFF;
        datagram[25] = 0xFF;
        datagram[26] = 0x05;
        datagram[27] = 0x00;

        datagram[28] = 0xFF;
        datagram[29] = 0xFF;
        datagram[30] = 0x06;
        datagram[31] = 0x00;

        datagram[32] = 0x01;
        datagram[33] = 0x00;
        datagram[34] = 0x07;
        datagram[35] = 0x00;

        if(ui->amplifierClose->isChecked())
        {
            datagram[36] = 0x00;
        }
        else
        {
            datagram[36] = 0xFF;
        }
        datagram[37] = 0x00;
        datagram[38] = 0x08;
        datagram[39] = 0x00;

        datagram[40] = 0xFF;
        datagram[41] = 0xFF;
        datagram[42] = 0x09;
        datagram[43] = 0x00;

        datagram[44] = 0xFF;
        datagram[45] = 0xFF;
        datagram[46] = 0x0A;
        datagram[47] = 0x00;

        datagram[48] = 0xFF;
        datagram[49] = 0xFF;
        datagram[50] = 0x0B;
        datagram[51] = 0x00;

        datagram[52] = 0xFF;
        datagram[53] = 0xFF;
        datagram[54] = 0x0C;
        datagram[55] = 0x00;

        for(int i = 0; i < 8; i++)
        {
            datagram[i + 56] = 0xAA;
        }

        //qDebug() << "send loop1";
        setClient->write(datagram);
        //qDebug() << "send loop2";
        stopData.clear();
        stopData = datagram;
        stopData[32] = 0x00;
        stopData[36] = 0x00;
    }
    else if(ui->countRadio->isChecked())
    {
        QByteArray datagram;
        datagram.resize(64);
        for(int i = 0; i < 8; i++)
        {
            datagram[i] = 0x1A;
        }

        datagram[8] = 0xFF;
        datagram[9] = 0xFF;
        datagram[10] = 0x01;
        datagram[11] = 0x00;

        datagram[12] = 0xFF;
        datagram[13] = 0xFF;
        datagram[14] = 0x02;
        datagram[15] = 0x00;

        int count = ui->countEdit->text().toInt();
        QString str1 = QString::number(count, 16);
        if(str1.size() < 8)
        {
            QString tmp;
            int length = 8 - str1.size();
            for(int i = 0; i < length; i++)
            {
                tmp = tmp + "0";
            }
            str1 = tmp + str1;
        }
        QByteArray datagram1;
        datagram1 = hexStringtoByteArray(str1);
        datagram1.resize(4);

        datagram[16] = datagram1[1];
        datagram[17] = datagram1[0];
        datagram[18] = 0x03;
        datagram[19] = 0x00;


        datagram[20] = datagram1[3];
        datagram[21] = datagram1[2];
        datagram[22] = 0x04;
        datagram[23] = 0x00;


        int interval = ui->interval1Edit->text().toInt();
        QString str2 = QString::number(interval, 16);
        if(str2.size() < 8)
        {
            QString tmp;
            int length = 8 - str2.size();
            for(int i = 0; i < length; i++)
            {
                tmp = tmp + "0";
            }
            str2 = tmp + str2;
        }
        QByteArray datagram2;
        datagram2 = hexStringtoByteArray(str2);
        datagram2.resize(4);

        datagram[24] = datagram2[1];
        datagram[25] = datagram2[0];
        datagram[26] = 0x05;
        datagram[27] = 0x00;

        datagram[28] = datagram2[3];
        datagram[29] = datagram2[2];
        datagram[30] = 0x06;
        datagram[31] = 0x00;

        datagram[32] = 0x01;
        datagram[33] = 0x00;
        datagram[34] = 0x07;
        datagram[35] = 0x00;

        datagram[36] = 0xFF;
        datagram[37] = 0x00;
        datagram[38] = 0x08;
        datagram[39] = 0x00;

        datagram[40] = 0xFF;
        datagram[41] = 0xFF;
        datagram[42] = 0x09;
        datagram[43] = 0x00;

        datagram[44] = 0xFF;
        datagram[45] = 0xFF;
        datagram[46] = 0x0A;
        datagram[47] = 0x00;

        datagram[48] = 0xFF;
        datagram[49] = 0xFF;
        datagram[50] = 0x0B;
        datagram[51] = 0x00;

        datagram[52] = 0xFF;
        datagram[53] = 0xFF;
        datagram[54] = 0x0C;
        datagram[55] = 0x00;

        for(int i = 0; i < 8; i++)
        {
            datagram[i + 56] = 0xAA;
        }

        setClient->write(datagram);
    }
    else if(ui->timeRadio->isChecked())
    {
        QByteArray datagram;
        datagram.resize(64);
        for(int i = 0; i < 8; i++)
        {
            datagram[i] = 0x1A;
        }

        int time = ui->timeEdit->text().toInt();
        QString str1 = QString::number(time, 16);
        if(str1.size() < 8)
        {
            QString tmp;
            int length = 8 - str1.size();
            for(int i = 0; i < length; i++)
            {
                tmp = tmp + "0";
            }
            str1 = tmp + str1;
        }
        QByteArray datagram1;
        datagram1 = hexStringtoByteArray(str1);
        datagram1.resize(4);

        datagram[8] = datagram1[1];
        datagram[9] = datagram1[0];
        datagram[10] = 0x01;
        datagram[11] = 0x00;

        datagram[12] = datagram1[3];
        datagram[13] = datagram1[2];
        datagram[14] = 0x02;
        datagram[15] = 0x00;

        datagram[16] = 0xFF;
        datagram[17] = 0xFF;
        datagram[18] = 0x03;
        datagram[19] = 0x00;

        datagram[20] = 0xFF;
        datagram[21] = 0xFF;
        datagram[22] = 0x04;
        datagram[23] = 0x00;

        int interval = ui->interval2Edit->text().toInt();
        QString str2 = QString::number(interval, 16);
        if(str2.size() < 8)
        {
            QString tmp;
            int length = 8 - str2.size();
            for(int i = 0; i < length; i++)
            {
                tmp = tmp + "0";
            }
            str2 = tmp + str2;
        }
        QByteArray datagram2;
        datagram2 = hexStringtoByteArray(str2);
        datagram2.resize(4);

        datagram[24] = datagram2[1];
        datagram[25] = datagram2[0];
        datagram[26] = 0x05;
        datagram[27] = 0x00;

        datagram[28] = datagram2[3];
        datagram[29] = datagram2[2];
        datagram[30] = 0x06;
        datagram[31] = 0x00;

        datagram[32] = 0x01;
        datagram[33] = 0x00;
        datagram[34] = 0x07;
        datagram[35] = 0x00;

        datagram[36] = 0xFF;
        datagram[37] = 0x00;
        datagram[38] = 0x08;
        datagram[39] = 0x00;

        datagram[40] = 0xFF;
        datagram[41] = 0xFF;
        datagram[42] = 0x09;
        datagram[43] = 0x00;

        datagram[44] = 0xFF;
        datagram[45] = 0xFF;
        datagram[46] = 0x0A;
        datagram[47] = 0x00;

        datagram[48] = 0xFF;
        datagram[49] = 0xFF;
        datagram[50] = 0x0B;
        datagram[51] = 0x00;

        datagram[52] = 0xFF;
        datagram[53] = 0xFF;
        datagram[54] = 0x0C;
        datagram[55] = 0x00;

        for(int i = 0; i < 8; i++)
        {
            datagram[i + 56] = 0xAA;
        }

        setClient->write(datagram);
    }
}

void Widget::set_send()
{
    if(!setClient->isValid())
    {
        qDebug() << "not valid socket";
        return;
    }

    totalBytes = 0;
    waitFMC = true;
    waitSet = false;
    serialFlag = false;
    endFlag = false;

    QByteArray datagram;
    datagram.resize(52);
    for(int i = 0; i < 8; i++)
    {
        datagram[i] = 0x0A;
    }

    datagram[8] = 0x04;
    datagram[9] = 0x00;
    datagram[10] = 0x00;
    datagram[11] = 0x00;

    datagram[12] = 0x90;
    datagram[13] = 0x00;
    datagram[14] = 0x00;
    datagram[15] = 0x00;

    quint32 data1 = (quint32)ui->fwidget1->getCurrentIndex() * 1000 + (quint32)ui->fwidget2->getCurrentIndex() * 100 + (quint32)ui->fwidget3->getCurrentIndex() * 10 + (quint32)ui->fwidget4->getCurrentIndex();
    quint32 data2 = (quint32)ui->fwidget5->getCurrentIndex() * 1000 + (quint32)ui->fwidget6->getCurrentIndex() * 100 + (quint32)ui->fwidget7->getCurrentIndex() * 10 + (quint32)ui->fwidget8->getCurrentIndex();
    quint32 data = data1 * 10000 + data2;
    quint32 tmpData = data;
    if(preFre == 0)
    {
        sendCalibration = true;
        seData[16] = 0x03;
        preFre = data;
    }
    else if(abs(data - preFre) >= 2000000)
    {
        sendCalibration = true;
        seData[16] = 0x03;
        preFre = data;
    }
    else
    {
        seData[16] = 0x00;
        sendCalibration = true;
    }
  //  qDebug() << data1;
    if(data1 <= 800)
    {
        data = (data1 + 2000) * 10000 + data2;
        serialData.clear();
        serialData.resize(23);
        for(int i = 0; i < 8; i++)
        {
            serialData[i] = 0x6A;
        }
        serialData[8] = 0xAA;
        serialData[9] = 0x82;
        serialData[10] = 0x00;
        serialData[11] = 0x00;
        serialData[12] = 0x00;
        serialData[13] = 0x00;
        serialData[14] = 0x55;
        for(int i = 0; i < 8; i++)
        {
            serialData[15 + i] = 0xAA;
        }
    }
    else
    {
        serialData.clear();
        serialData.resize(23);
        for(int i = 0; i < 8; i++)
        {
            serialData[i] = 0x6A;
        }
        serialData[8] = 0xAA;
        serialData[9] = 0x82;
        serialData[10] = 0xFF;
        serialData[11] = 0xFF;
        serialData[12] = 0xFF;
        serialData[13] = 0xFF;
        serialData[14] = 0x55;
        for(int i = 0; i < 8; i++)
        {
            serialData[15 + i] = 0xAA;
        }
    }

    data += freValue;
    QString str1 = QString::number(data, 16);
    QByteArray datagram1;
    if(str1.size() < 8)
    {
        QString tmp;
        int length = 8 - str1.size();
        for(int i = 0; i < length; i++)
        {
            tmp = tmp + "0";
        }
        str1 = tmp + str1;
    }
    datagram1 = hexStringtoByteArray(str1);
    datagram1.resize(4);

    datagram[16] = datagram1[3];
    datagram[17] = datagram1[2];
    datagram[18] = datagram1[1];
    datagram[19] = datagram1[0];

    datagram[20] = 0x9B;
    datagram[21] = 0x00;
    datagram[22] = 0x00;
    datagram[23] = 0x00;
    datagram[24] = 0x0F;
    datagram[25] = 0x00;
    datagram[26] = 0x00;
    datagram[27] = 0x00;

    datagram[28] = 0x9C;
    datagram[29] = 0x00;
    datagram[30] = 0x00;
    datagram[31] = 0x00;
    datagram[32] = 0x04;
    datagram[33] = 0x00;
    datagram[34] = 0x00;
    datagram[35] = 0x00;

    datagram[36] = 0x97;
    datagram[37] = 0x00;
    datagram[38] = 0x00;
    datagram[39] = 0x00;

    QString str_value = QString::number(ui->dwidget1->getCurrentIndex()) + QString::number(ui->dwidget2->getCurrentIndex()) + "." + QString::number(ui->dwidget3->getCurrentIndex());
    double dou_value = str_value.toDouble() * 2;
    quint32 int_value = qRound(dou_value);
    quint32 fre_value = int_value;
    if(ui->amplifierClose->isChecked())//功放关闭
    {
        if(int_value <= 60)
        {
            int_value = getAmplitude(tmpData);
        }
        else
        {
            int_value = int_value - 60 + getAmplitude(tmpData);
        }

        QString str2 = QString::number(int_value, 16);
        QByteArray datagram2;
        if(str2.size() < 2)
        {
            QString tmp;
            int length = 2 - str2.size();
            for(int i = 0; i < length; i++)
            {
                 tmp = tmp + "0";
            }
                str2 = tmp + str2;
        }
        datagram2 = hexStringtoByteArray(str2);
        datagram2.resize(1);

        datagram[40] = datagram2[0];
        datagram[41] = datagram2[0];
        datagram[42] = 0x00;
        datagram[43] = 0x00;

        if(fre_value > 60)
        {
            fre_value = 60;
        }
        QString str3 = QString::number(fre_value, 16);
        QByteArray datagram3;
        if(str3.size() < 2)
        {
            QString tmp;
            int length = 2 - str3.size();
            for(int i = 0; i < length; i++)
            {
                 tmp = tmp + "0";
            }
                str3 = tmp + str3;
        }
        datagram3 = hexStringtoByteArray(str3);
        datagram3.resize(1);

        serialFreData.clear();
        serialFreData.resize(23);
        for(int i = 0; i < 8; i++)
        {
            serialFreData[i] = 0x6A;
        }
        serialFreData[8] = 0xAA;
        serialFreData[9] = 0x84;
        serialFreData[10] = datagram3[0];
        serialFreData[11] = datagram3[0];
        serialFreData[12] = datagram3[0];
        serialFreData[13] = datagram3[0];
        serialFreData[14] = 0x55;
        for(int i = 0; i < 8; i++)
        {
            serialFreData[15 + i] = 0xAA;
        }

    }
    else//功放打开
    {
        if(ui->label_4->text() == "-")
        {
            int_value = 37 * 2 + int_value;
        }
        else
        {
            int_value = 37 * 2 - int_value;
        }
        fre_value = int_value;
        if(int_value <= 60)
        {
            int_value = getAmplitude(tmpData);
        }
        else
        {
            int_value = int_value - 60 + getAmplitude(tmpData);
        }

        QString str2 = QString::number(int_value, 16);
        QByteArray datagram2;
        if(str2.size() < 2)
        {
            QString tmp;
            int length = 2 - str2.size();
            for(int i = 0; i < length; i++)
            {
                 tmp = tmp + "0";
            }
                str2 = tmp + str2;
        }
        datagram2 = hexStringtoByteArray(str2);
        datagram2.resize(1);

        datagram[40] = datagram2[0];
        datagram[41] = datagram2[0];
        datagram[42] = 0x00;
        datagram[43] = 0x00;

        if(fre_value > 60)
        {
            fre_value = 60;
        }
        QString str3 = QString::number(fre_value, 16);
        QByteArray datagram3;
        if(str3.size() < 2)
        {
            QString tmp;
            int length = 2 - str3.size();
            for(int i = 0; i < length; i++)
            {
                 tmp = tmp + "0";
            }
                str3 = tmp + str3;
        }
        datagram3 = hexStringtoByteArray(str3);
        datagram3.resize(1);

        serialFreData.clear();
        serialFreData.resize(23);
        for(int i = 0; i < 8; i++)
        {
            serialFreData[i] = 0x6A;
        }
        serialFreData[8] = 0xAA;
        serialFreData[9] = 0x84;
        serialFreData[10] = datagram3[0];
        serialFreData[11] = datagram3[0];
        serialFreData[12] = datagram3[0];
        serialFreData[13] = datagram3[0];
        serialFreData[14] = 0x55;
        for(int i = 0; i < 8; i++)
        {
            serialFreData[15 + i] = 0xAA;
        }
    }

    for(int i = 0; i < 8; i++)
    {
        datagram[i + 44] = 0xAA;
    }
    setClient->write(datagram);
}

void Widget::fre_send()
{
    if(!setClient->isValid())
    {
        qDebug() << "not valid socket";
        return;
    }

    freFlag = true;

    QByteArray datagram;
    datagram.resize(52);
    for(int i = 0; i < 8; i++)
    {
        datagram[i] = 0x0A;
    }

    datagram[8] = 0x04;
    datagram[9] = 0x00;
    datagram[10] = 0x00;
    datagram[11] = 0x00;

    datagram[12] = 0x90;
    datagram[13] = 0x00;
    datagram[14] = 0x00;
    datagram[15] = 0x00;

    quint32 data1 = (quint32)ui->fwidget1->getCurrentIndex() * 1000 + (quint32)ui->fwidget2->getCurrentIndex() * 100 + (quint32)ui->fwidget3->getCurrentIndex() * 10 + (quint32)ui->fwidget4->getCurrentIndex();
    quint32 data2 = (quint32)ui->fwidget5->getCurrentIndex() * 1000 + (quint32)ui->fwidget6->getCurrentIndex() * 100 + (quint32)ui->fwidget7->getCurrentIndex() * 10 + (quint32)ui->fwidget8->getCurrentIndex();
    quint32 data = data1 * 10000 + data2;
    quint32 tmpData = data;
    if(preFre == 0)
    {
        sendCalibration = true;
        seData[16] = 0x03;
        preFre = data;
    }
    else if(abs(data - preFre) >= 2000000)
    {
        sendCalibration = true;
        seData[16] = 0x03;
        preFre = data;
    }
    else
    {
        seData[16] = 0x00;
        sendCalibration = true;
    }
    if(data1 <= 800)
    {
        data = (data1 + 2000) * 10000 + data2;
        serialData.clear();
        serialData.resize(23);
        for(int i = 0; i < 8; i++)
        {
            serialData[i] = 0x6A;
        }
        serialData[8] = 0xAA;
        serialData[9] = 0x82;
        serialData[10] = 0x00;
        serialData[11] = 0x00;
        serialData[12] = 0x00;
        serialData[13] = 0x00;
        serialData[14] = 0x55;
        for(int i = 0; i < 8; i++)
        {
            serialData[15 + i] = 0xAA;
        }
    }
    else
    {
        serialData.clear();
        serialData.resize(23);
        for(int i = 0; i < 8; i++)
        {
            serialData[i] = 0x6A;
        }
        serialData[8] = 0xAA;
        serialData[9] = 0x82;
        serialData[10] = 0xFF;
        serialData[11] = 0xFF;
        serialData[12] = 0xFF;
        serialData[13] = 0xFF;
        serialData[14] = 0x55;
        for(int i = 0; i < 8; i++)
        {
            serialData[15 + i] = 0xAA;
        }
    }

    data += freValue;
    QString str1 = QString::number(data, 16);
    QByteArray datagram1;
    if(str1.size() < 8)
    {
        QString tmp;
        int length = 8 - str1.size();
        for(int i = 0; i < length; i++)
        {
            tmp = tmp + "0";
        }
        str1 = tmp + str1;
    }
    datagram1 = hexStringtoByteArray(str1);
    datagram1.resize(4);

    datagram[16] = datagram1[3];
    datagram[17] = datagram1[2];
    datagram[18] = datagram1[1];
    datagram[19] = datagram1[0];

    datagram[20] = 0x9B;
    datagram[21] = 0x00;
    datagram[22] = 0x00;
    datagram[23] = 0x00;
    datagram[24] = 0x0F;
    datagram[25] = 0x00;
    datagram[26] = 0x00;
    datagram[27] = 0x00;

    datagram[28] = 0x9C;
    datagram[29] = 0x00;
    datagram[30] = 0x00;
    datagram[31] = 0x00;
    datagram[32] = 0x04;
    datagram[33] = 0x00;
    datagram[34] = 0x00;
    datagram[35] = 0x00;

    datagram[36] = 0x97;
    datagram[37] = 0x00;
    datagram[38] = 0x00;
    datagram[39] = 0x00;

    QString str_value = QString::number(ui->dwidget1->getCurrentIndex()) + QString::number(ui->dwidget2->getCurrentIndex()) + "." + QString::number(ui->dwidget3->getCurrentIndex());
    double dou_value = str_value.toDouble() * 2;
    quint32 int_value = qRound(dou_value);
    quint32 fre_value = int_value;
    if(ui->amplifierClose->isChecked())//功放关闭
    {
        if(int_value <= 60)
        {
            int_value = getAmplitude(tmpData);
        }
        else
        {
            int_value = int_value - 60 + getAmplitude(tmpData);
        }

        QString str2 = QString::number(int_value, 16);
        QByteArray datagram2;
        if(str2.size() < 2)
        {
            QString tmp;
            int length = 2 - str2.size();
            for(int i = 0; i < length; i++)
            {
                 tmp = tmp + "0";
            }
                str2 = tmp + str2;
        }
        datagram2 = hexStringtoByteArray(str2);
        datagram2.resize(1);

        datagram[40] = datagram2[0];
        datagram[41] = datagram2[0];
        datagram[42] = 0x00;
        datagram[43] = 0x00;

        if(fre_value > 60)
        {
            fre_value = 60;
        }
        QString str3 = QString::number(fre_value, 16);
        QByteArray datagram3;
        if(str3.size() < 2)
        {
            QString tmp;
            int length = 2 - str3.size();
            for(int i = 0; i < length; i++)
            {
                 tmp = tmp + "0";
            }
                str3 = tmp + str3;
        }
        datagram3 = hexStringtoByteArray(str3);
        datagram3.resize(1);

        serialFreData.clear();
        serialFreData.resize(23);
        for(int i = 0; i < 8; i++)
        {
            serialFreData[i] = 0x6A;
        }
        serialFreData[8] = 0xAA;
        serialFreData[9] = 0x84;
        serialFreData[10] = datagram3[0];
        serialFreData[11] = datagram3[0];
        serialFreData[12] = datagram3[0];
        serialFreData[13] = datagram3[0];
        serialFreData[14] = 0x55;
        for(int i = 0; i < 8; i++)
        {
            serialFreData[15 + i] = 0xAA;
        }

    }
    else//功放打开
    {
        if(ui->label_4->text() == "-")
        {
            int_value = 37 * 2 + int_value;
        }
        else
        {
            int_value = 37 * 2 - int_value;
        }
        fre_value = int_value;
        if(int_value <= 60)
        {
            int_value = getAmplitude(tmpData);
        }
        else
        {
            int_value = int_value - 60 + getAmplitude(tmpData);
        }

        QString str2 = QString::number(int_value, 16);
        QByteArray datagram2;
        if(str2.size() < 2)
        {
            QString tmp;
            int length = 2 - str2.size();
            for(int i = 0; i < length; i++)
            {
                 tmp = tmp + "0";
            }
                str2 = tmp + str2;
        }
        datagram2 = hexStringtoByteArray(str2);
        datagram2.resize(1);

        datagram[40] = datagram2[0];
        datagram[41] = datagram2[0];
        datagram[42] = 0x00;
        datagram[43] = 0x00;

        if(fre_value > 60)
        {
            fre_value = 60;
        }
        QString str3 = QString::number(fre_value, 16);
        QByteArray datagram3;
        if(str3.size() < 2)
        {
            QString tmp;
            int length = 2 - str3.size();
            for(int i = 0; i < length; i++)
            {
                 tmp = tmp + "0";
            }
                str3 = tmp + str3;
        }
        datagram3 = hexStringtoByteArray(str3);
        datagram3.resize(1);

        serialFreData.clear();
        serialFreData.resize(23);
        for(int i = 0; i < 8; i++)
        {
            serialFreData[i] = 0x6A;
        }
        serialFreData[8] = 0xAA;
        serialFreData[9] = 0x84;
        serialFreData[10] = datagram3[0];
        serialFreData[11] = datagram3[0];
        serialFreData[12] = datagram3[0];
        serialFreData[13] = datagram3[0];
        serialFreData[14] = 0x55;
        for(int i = 0; i < 8; i++)
        {
            serialFreData[15 + i] = 0xAA;
        }
    }

    for(int i = 0; i < 8; i++)
    {
        datagram[i + 44] = 0xAA;
    }
    setClient->write(datagram);
}

void Widget::serial_fre_send()
{
    if(setClient->isValid())
    {
        setClient->write(serialFreData);
    }
}

void Widget::calibrationFlag_send()
{
    if(setClient->isValid())
    {
        setClient->write(seData);
    }
}

void Widget::on_upBtn_clicked()
{
    myTumbler* wid = m_index[currentIndex];
    int index = wid->getCurrentIndex();
    if(currentIndex == 1)
    {
        if(index == 3)
        {
            wid->setCurrentIndex(index);
        }
        else
        {
            wid->setCurrentIndex(index + 1);
        }
    }
    else
    {
        if(index == 9)
        {
            wid->setCurrentIndex(index);
        }
        else
        {
            wid->setCurrentIndex(index + 1);
        }
    }
}

void Widget::on_leftBtn_clicked()
{
    if(currentIndex == 1)
    {
        currentIndex = 1;
    }
    else
    {
        currentIndex--;
    }

    myTumbler* wid = m_index[currentIndex];
    wid->setLineFlag(true);
    wid->update();
    wid = m_index[currentIndex + 1];
    wid->setLineFlag(false);
    wid->update();
}

void Widget::on_downBtn_clicked()
{
    myTumbler* wid = m_index[currentIndex];
    int index = wid->getCurrentIndex();
    if(index == 0)
    {
        wid->setCurrentIndex(index);
    }
    else
    {
        wid->setCurrentIndex(index - 1);
    }
}

void Widget::on_rightBtn_clicked()
{
    if(currentIndex == maxIndex)
    {
        currentIndex = maxIndex;
    }
    else
    {
        currentIndex++;
    }

    myTumbler* wid = m_index[currentIndex];
    wid->setLineFlag(true);
    wid->update();
    wid = m_index[currentIndex - 1];
    wid->setLineFlag(false);
    wid->update();
}

void Widget::on_pushButton_3_clicked()
{
    MySystemShutDown();
}

void Widget::on_num1Btn_clicked()
{
    QString value;
    QString number = "1";
    myTumbler* wid = m_index[currentIndex];
    switch(lineSelect)
    {

    case 0:
        if(ui->amplifierOpen->isChecked())
        {
            if(currentIndex == 11)
            {
                if((ui->dwidget1->getCurrentIndex() == 3) && (ui->dwidget2->getCurrentIndex() == 7))
                {
                    return;
                }
            }
            else if(currentIndex == 9)
            {
                ui->dwidget2->setMaxIndex(9);
            }
        }
        wid->setCurrentIndex(1);
        on_rightBtn_clicked();
        break;

    case 1:
        value = ui->countEdit->text();
        value = value + number;
        ui->countEdit->setText(value);
        ui->countEdit->setFocus();
        break;

    case 2:
        value = ui->interval1Edit->text();
        value = value + number;
        ui->interval1Edit->setText(value);
        ui->interval1Edit->setFocus();
        break;

    case 3:
        value = ui->timeEdit->text();
        value = value + number;
        ui->timeEdit->setText(value);
        ui->timeEdit->setFocus();
        break;

    case 4:
        value = ui->interval2Edit->text();
        value = value + number;
        ui->interval2Edit->setText(value);
        ui->interval2Edit->setFocus();
        break;

    case 5:
        value = ui->delayEdit->text();
        value = value + number;
        ui->delayEdit->setText(value);
        ui->delayEdit->setFocus();
        break;


    default:
        break;
    }
}

void Widget::on_num2Btn_clicked()
{
    QString value;
    QString number = "2";
    myTumbler* wid = m_index[currentIndex];
    switch(lineSelect)
    {

    case 0:
        if(ui->amplifierOpen->isChecked())
        {
            if(currentIndex == 11)
            {
                if((ui->dwidget1->getCurrentIndex() == 3) && (ui->dwidget2->getCurrentIndex() == 7))
                {
                    return;
                }
            }
            else if(currentIndex == 9)
            {
                ui->dwidget2->setMaxIndex(9);
            }
        }
        wid->setCurrentIndex(2);
        on_rightBtn_clicked();
        break;

    case 1:
        value = ui->countEdit->text();
        value = value + number;
        ui->countEdit->setText(value);
        ui->countEdit->setFocus();
        break;

    case 2:
        value = ui->interval1Edit->text();
        value = value + number;
        ui->interval1Edit->setText(value);
        ui->interval1Edit->setFocus();
        break;

    case 3:
        value = ui->timeEdit->text();
        value = value + number;
        ui->timeEdit->setText(value);
        ui->timeEdit->setFocus();
        break;

    case 4:
        value = ui->interval2Edit->text();
        value = value + number;
        ui->interval2Edit->setText(value);
        ui->interval2Edit->setFocus();
        break;

    case 5:
        value = ui->delayEdit->text();
        value = value + number;
        ui->delayEdit->setText(value);
        ui->delayEdit->setFocus();
        break;

    default:
        break;
    }
}

void Widget::on_num3Btn_clicked()
{
    QString value;
    QString number = "3";
    myTumbler* wid = m_index[currentIndex];
    switch(lineSelect)
    {

    case 0:
        if(ui->amplifierOpen->isChecked())
        {
            if(currentIndex == 11)
            {
                if((ui->dwidget1->getCurrentIndex() == 3) && (ui->dwidget2->getCurrentIndex() == 7))
                {
                    return;
                }
            }
            else if(currentIndex == 9)
            {
                ui->dwidget2->setMaxIndex(7);
            }
        }
        wid->setCurrentIndex(3);
        on_rightBtn_clicked();
        break;

    case 1:
        value = ui->countEdit->text();
        value = value + number;
        ui->countEdit->setText(value);
        ui->countEdit->setFocus();
        break;

    case 2:
        value = ui->interval1Edit->text();
        value = value + number;
        ui->interval1Edit->setText(value);
        ui->interval1Edit->setFocus();
        break;

    case 3:
        value = ui->timeEdit->text();
        value = value + number;
        ui->timeEdit->setText(value);
        ui->timeEdit->setFocus();
        break;

    case 4:
        value = ui->interval2Edit->text();
        value = value + number;
        ui->interval2Edit->setText(value);
        ui->interval2Edit->setFocus();
        break;

    case 5:
        value = ui->delayEdit->text();
        value = value + number;
        ui->delayEdit->setText(value);
        ui->delayEdit->setFocus();
        break;

    default:
        break;
    }
}

void Widget::on_num4Btn_clicked()
{
    QString value;
    QString number = "4";
    myTumbler* wid = m_index[currentIndex];
    switch(lineSelect)
    {

    case 0:
        if(ui->amplifierOpen->isChecked())
        {
            if(currentIndex == 11)
            {
                if((ui->dwidget1->getCurrentIndex() == 3) && (ui->dwidget2->getCurrentIndex() == 7))
                {
                    return;
                }
            }
        }
        if(currentIndex > 1)
        {
            if(currentIndex == 9)
            {
                if(ui->amplifierOpen->isChecked())
                {
                    on_rightBtn_clicked();
                }
                else
                {
                    wid->setCurrentIndex(4);
                    on_rightBtn_clicked();
                }
            }
            else
            {
                wid->setCurrentIndex(4);
                on_rightBtn_clicked();
            }
        }
        else
        {
            on_rightBtn_clicked();
        }
        break;

    case 1:
        value = ui->countEdit->text();
        value = value + number;
        ui->countEdit->setText(value);
        ui->countEdit->setFocus();
        break;

    case 2:
        value = ui->interval1Edit->text();
        value = value + number;
        ui->interval1Edit->setText(value);
        ui->interval1Edit->setFocus();
        break;

    case 3:
        value = ui->timeEdit->text();
        value = value + number;
        ui->timeEdit->setText(value);
        ui->timeEdit->setFocus();
        break;

    case 4:
        value = ui->interval2Edit->text();
        value = value + number;
        ui->interval2Edit->setText(value);
        ui->interval2Edit->setFocus();
        break;

    case 5:
        value = ui->delayEdit->text();
        value = value + number;
        ui->delayEdit->setText(value);
        ui->delayEdit->setFocus();
        break;

    default:
        break;
    }
}

void Widget::on_num5Btn_clicked()
{
    QString value;
    QString number = "5";
    myTumbler* wid = m_index[currentIndex];
    switch(lineSelect)
    {

    case 0:
        if(ui->amplifierOpen->isChecked())
        {
            if(currentIndex == 11)
            {
                if((ui->dwidget1->getCurrentIndex() == 3) && (ui->dwidget2->getCurrentIndex() == 7))
                {
                    return;
                }
            }
        }
        if(currentIndex > 1)
        {
            if(currentIndex == 9)
            {
                if(ui->amplifierOpen->isChecked())
                {
                    on_rightBtn_clicked();
                }
                else
                {
                    wid->setCurrentIndex(5);
                    on_rightBtn_clicked();
                }
            }
            else
            {
                wid->setCurrentIndex(5);
                on_rightBtn_clicked();
            }
        }
        else
        {
            on_rightBtn_clicked();
        }
        break;

    case 1:
        value = ui->countEdit->text();
        value = value + number;
        ui->countEdit->setText(value);
        ui->countEdit->setFocus();
        break;

    case 2:
        value = ui->interval1Edit->text();
        value = value + number;
        ui->interval1Edit->setText(value);
        ui->interval1Edit->setFocus();
        break;

    case 3:
        value = ui->timeEdit->text();
        value = value + number;
        ui->timeEdit->setText(value);
        ui->timeEdit->setFocus();
        break;

    case 4:
        value = ui->interval2Edit->text();
        value = value + number;
        ui->interval2Edit->setText(value);
        ui->interval2Edit->setFocus();
        break;

    case 5:
        value = ui->delayEdit->text();
        value = value + number;
        ui->delayEdit->setText(value);
        ui->delayEdit->setFocus();
        break;

    default:
        break;
    }
}

void Widget::on_num6Btn_clicked()
{
    QString value;
    QString number = "6";
    myTumbler* wid = m_index[currentIndex];
    switch(lineSelect)
    {

    case 0:
        if(ui->amplifierOpen->isChecked())
        {
            if(currentIndex == 11)
            {
                if((ui->dwidget1->getCurrentIndex() == 3) && (ui->dwidget2->getCurrentIndex() == 7))
                {
                    return;
                }
                else
                {
                    //wid->setCurrentIndex(9);
                    on_rightBtn_clicked();
                }
            }
        }
        if(currentIndex > 1)
        {
            if(currentIndex == 9)
            {
                if(ui->amplifierOpen->isChecked())
                {
                    on_rightBtn_clicked();
                }
            }
            else
            {
                wid->setCurrentIndex(6);
                on_rightBtn_clicked();
            }
        }
        else
        {
            on_rightBtn_clicked();
        }
        break;

    case 1:
        value = ui->countEdit->text();
        value = value + number;
        ui->countEdit->setText(value);
        ui->countEdit->setFocus();
        break;

    case 2:
        value = ui->interval1Edit->text();
        value = value + number;
        ui->interval1Edit->setText(value);
        ui->interval1Edit->setFocus();
        break;

    case 3:
        value = ui->timeEdit->text();
        value = value + number;
        ui->timeEdit->setText(value);
        ui->timeEdit->setFocus();
        break;

    case 4:
        value = ui->interval2Edit->text();
        value = value + number;
        ui->interval2Edit->setText(value);
        ui->interval2Edit->setFocus();
        break;

    case 5:
        value = ui->delayEdit->text();
        value = value + number;
        ui->delayEdit->setText(value);
        ui->delayEdit->setFocus();
        break;

    default:
        break;
    }
}

void Widget::on_num7Btn_clicked()
{
    QString value;
    QString number = "7";
    myTumbler* wid = m_index[currentIndex];
    switch(lineSelect)
    {

    case 0:
        if(ui->amplifierOpen->isChecked())
        {
            if(currentIndex == 11)
            {
                if((ui->dwidget1->getCurrentIndex() == 3) && (ui->dwidget2->getCurrentIndex() == 7))
                {
                    return;
                }
            }
        }
        if(currentIndex > 1)
        {
            if(currentIndex == 9)
            {
                if(ui->amplifierOpen->isChecked())
                {
                    on_rightBtn_clicked();
                }
                else
                {
                    //wid->setCurrentIndex(9);
                    on_rightBtn_clicked();
                }
            }
            else
            {
                wid->setCurrentIndex(7);
                on_rightBtn_clicked();
            }
        }
        else
        {
            on_rightBtn_clicked();
        }
        break;

    case 1:
        value = ui->countEdit->text();
        value = value + number;
        ui->countEdit->setText(value);
        ui->countEdit->setFocus();
        break;

    case 2:
        value = ui->interval1Edit->text();
        value = value + number;
        ui->interval1Edit->setText(value);
        ui->interval1Edit->setFocus();
        break;

    case 3:
        value = ui->timeEdit->text();
        value = value + number;
        ui->timeEdit->setText(value);
        ui->timeEdit->setFocus();
        break;

    case 4:
        value = ui->interval2Edit->text();
        value = value + number;
        ui->interval2Edit->setText(value);
        ui->interval2Edit->setFocus();
        break;

    case 5:
        value = ui->delayEdit->text();
        value = value + number;
        ui->delayEdit->setText(value);
        ui->delayEdit->setFocus();
        break;

    default:
        break;
    }
}

void Widget::on_num8Btn_clicked()
{
    QString value;
    QString number = "8";
    myTumbler* wid = m_index[currentIndex];
    switch(lineSelect)
    {

    case 0:
        if(ui->amplifierOpen->isChecked())
        {
            if(currentIndex == 11)
            {
                if((ui->dwidget1->getCurrentIndex() == 3) && (ui->dwidget2->getCurrentIndex() == 7))
                {
                    return;
                }
            }
        }
        if(currentIndex > 1)
        {
            if(currentIndex == 9)
            {
                if(ui->amplifierOpen->isChecked())
                {
                    on_rightBtn_clicked();
                }
                else
                {
                    //wid->setCurrentIndex(9);
                    on_rightBtn_clicked();
                }
            }
            else if(currentIndex == 10)
            {
                if(ui->amplifierOpen->isChecked())
                {
                    if(ui->dwidget1->getCurrentIndex() == 3)
                    {
                         on_rightBtn_clicked();
                    }
                }
            }
            else
            {
                wid->setCurrentIndex(8);
                on_rightBtn_clicked();
            }
        }
        else
        {
            on_rightBtn_clicked();
        }
        break;

    case 1:
        value = ui->countEdit->text();
        value = value + number;
        ui->countEdit->setText(value);
        ui->countEdit->setFocus();
        break;

    case 2:
        value = ui->interval1Edit->text();
        value = value + number;
        ui->interval1Edit->setText(value);
        ui->interval1Edit->setFocus();
        break;

    case 3:
        value = ui->timeEdit->text();
        value = value + number;
        ui->timeEdit->setText(value);
        ui->timeEdit->setFocus();
        break;

    case 4:
        value = ui->interval2Edit->text();
        value = value + number;
        ui->interval2Edit->setText(value);
        ui->interval2Edit->setFocus();
        break;

    case 5:
        value = ui->delayEdit->text();
        value = value + number;
        ui->delayEdit->setText(value);
        ui->delayEdit->setFocus();
        break;

    default:
        break;
    }
}

void Widget::on_num9Btn_clicked()
{
    QString value;
    QString number = "9";
    myTumbler* wid = m_index[currentIndex];
    switch(lineSelect)
    {

    case 0:
        if(ui->amplifierOpen->isChecked())
        {
            if(currentIndex == 11)
            {
                if((ui->dwidget1->getCurrentIndex() == 3) && (ui->dwidget2->getCurrentIndex() == 7))
                {
                    return;
                }
            }
        }
        if(currentIndex > 1)
        {
            if(currentIndex == 9)
            {
                if(ui->amplifierOpen->isChecked())
                {
                    on_rightBtn_clicked();
                }
                else
                {
                    //wid->setCurrentIndex(9);
                    on_rightBtn_clicked();
                }
            }
            else if(currentIndex == 10)
            {
                if(ui->amplifierOpen->isChecked())
                {
                    if(ui->dwidget1->getCurrentIndex() == 3)
                    {
                         on_rightBtn_clicked();
                    }
                }
            }
            else
            {
                wid->setCurrentIndex(9);
                on_rightBtn_clicked();
            }
        }
        else
        {
            on_rightBtn_clicked();
        }
        break;

    case 1:
        value = ui->countEdit->text();
        value = value + number;
        ui->countEdit->setText(value);
        ui->countEdit->setFocus();
        break;

    case 2:
        value = ui->interval1Edit->text();
        value = value + number;
        ui->interval1Edit->setText(value);
        ui->interval1Edit->setFocus();
        break;

    case 3:
        value = ui->timeEdit->text();
        value = value + number;
        ui->timeEdit->setText(value);
        ui->timeEdit->setFocus();
        break;

    case 4:
        value = ui->interval2Edit->text();
        value = value + number;
        ui->interval2Edit->setText(value);
        ui->interval2Edit->setFocus();
        break;

    case 5:
        value = ui->delayEdit->text();
        value = value + number;
        ui->delayEdit->setText(value);
        ui->delayEdit->setFocus();
        break;

    default:
        break;
    }
}

void Widget::on_negativeBtn_clicked()
{
    QString value;
    bool ok = false;
    QString number = "-";
    QString data;
    switch(lineSelect)
    {

    case 0:
        data = ui->label_4->text();
        if(data == "-")
        {
            ui->label_4->setText("");
        }
        else
        {
            ui->label_4->setText("-");
        }
        break;

    case 1:
        value = ui->countEdit->text();
        ok = value.contains('-');
        if(!ok)
        {
            value = value.insert(0, number);
            ui->countEdit->setText(value);
            ui->countEdit->setFocus();
        }
        break;

    case 2:
        value = ui->interval1Edit->text();
        ok = value.contains('-');
        if(!ok)
        {
            value = value.insert(0, number);
            ui->interval1Edit->setText(value);
            ui->interval1Edit->setFocus();
        }
        break;

    case 3:
        value = ui->timeEdit->text();
        ok = value.contains('-');
        {
            value = value.insert(0, number);
            ui->timeEdit->setText(value);
            ui->timeEdit->setFocus();
        }
        break;

    case 4:
        value = ui->interval2Edit->text();
        ok = value.contains('-');
        {
            value = value.insert(0, number);
            ui->interval2Edit->setText(value);
            ui->interval2Edit->setFocus();
        }
        break;

    case 5:
        value = ui->delayEdit->text();
        ok = value.contains('-');
        {
            value = value.insert(0, number);
            ui->delayEdit->setText(value);
            ui->delayEdit->setFocus();
        }
        break;

    default:
        break;
    }
}

void Widget::on_num0Btn_clicked()
{
    QString value;
    QString number = "0";
    myTumbler* wid = m_index[currentIndex];
    switch(lineSelect)
    {

    case 0:
        if(currentIndex == 9)
        {
            ui->dwidget2->setMaxIndex(9);
        }
        wid->setCurrentIndex(0);
        on_rightBtn_clicked();
        break;

    case 1:
        value = ui->countEdit->text();
        value = value + number;
        ui->countEdit->setText(value);
        ui->countEdit->setFocus();
        break;

    case 2:
        value = ui->interval1Edit->text();
        value = value + number;
        ui->interval1Edit->setText(value);
        ui->interval1Edit->setFocus();
        break;

    case 3:
        value = ui->timeEdit->text();
        value = value + number;
        ui->timeEdit->setText(value);
        ui->timeEdit->setFocus();
        break;

    case 4:
        value = ui->interval2Edit->text();
        value = value + number;
        ui->interval2Edit->setText(value);
        ui->interval2Edit->setFocus();
        break;

    case 5:
        value = ui->delayEdit->text();
        value = value + number;
        ui->delayEdit->setText(value);
        ui->delayEdit->setFocus();
        break;

    default:
        break;
    }
}

void Widget::on_num000Btn_clicked()
{
    QString value;
    QString number = "000";
    switch(lineSelect)
    {

    case 0:
        break;

    case 1:
        value = ui->countEdit->text();
        value = value + number;
        ui->countEdit->setText(value);
        ui->countEdit->setFocus();
        break;

    case 2:
        value = ui->interval1Edit->text();
        value = value + number;
        ui->interval1Edit->setText(value);
        ui->interval1Edit->setFocus();
        break;

    case 3:
        value = ui->timeEdit->text();
        value = value + number;
        ui->timeEdit->setText(value);
        ui->timeEdit->setFocus();
        break;

    case 4:
        value = ui->interval2Edit->text();
        value = value + number;
        ui->interval2Edit->setText(value);
        ui->interval2Edit->setFocus();
        break;

    case 5:
        value = ui->delayEdit->text();
        value = value + number;
        ui->delayEdit->setText(value);
        ui->delayEdit->setFocus();
        break;

    default:
        break;
    }
}

void Widget::on_delBtn_clicked()
{
    QString value;
    switch(lineSelect)
    {

    case 0:
        break;

    case 1:
        value = ui->countEdit->text();
        value = value.left(value.size() - 1);
        ui->countEdit->setText(value);
        break;

    case 2:
        value = ui->interval1Edit->text();
        value = value.left(value.size() - 1);
        ui->interval1Edit->setText(value);
        break;

    case 3:
        value = ui->timeEdit->text();
        value = value.left(value.size() - 1);
        ui->timeEdit->setText(value);
        break;

    case 4:
        value = ui->interval2Edit->text();
        value = value.left(value.size() - 1);
        ui->interval2Edit->setText(value);
        break;

    case 5:
        value = ui->delayEdit->text();
        value = value.left(value.size() - 1);
        ui->delayEdit->setText(value);
        break;

    default:
        break;
    }
}

void Widget::on_acBtn_clicked()
{
    switch(lineSelect)
    {

    case 0:
        if(currentIndex < 9)
        {
            ui->fwidget1->setCurrentIndex(0);
            ui->fwidget2->setCurrentIndex(0);
            ui->fwidget3->setCurrentIndex(0);
            ui->fwidget4->setCurrentIndex(0);
            ui->fwidget5->setCurrentIndex(0);
            ui->fwidget6->setCurrentIndex(0);
            ui->fwidget7->setCurrentIndex(0);
            ui->fwidget8->setCurrentIndex(0);
        }
        else
        {
            ui->dwidget1->setCurrentIndex(0);
            ui->dwidget2->setCurrentIndex(0);
            ui->dwidget3->setCurrentIndex(0);
        }
        break;

    case 1:
        ui->countEdit->clear();
        ui->countEdit->setFocus();
        break;

    case 2:
        ui->interval1Edit->clear();
        ui->interval1Edit->setFocus();
        break;

    case 3:
        ui->timeEdit->clear();
        ui->timeEdit->setFocus();
        break;

    case 4:
        ui->interval2Edit->clear();
        ui->interval2Edit->setFocus();
        break;

    case 5:
        ui->delayEdit->clear();
        ui->delayEdit->setFocus();
        break;

    default:
        break;
    }
}

void Widget::on_okBtn_clicked()
{
    switch(lineSelect)
    {

    case 0:

        if(!freFlag)
        {
            fre_send();
        }
        break;

    case 1:
        ui->countEdit->clearFocus();
        break;

    case 2:
        ui->interval1Edit->clearFocus();
        break;

    case 3:
        ui->timeEdit->clearFocus();
        break;

    case 4:
        ui->interval2Edit->clearFocus();
        break;

    case 5:
        ui->delayEdit->clearFocus();
        break;

    default:
        break;
    }
}

void Widget::on_loopRadio_clicked()
{
    ui->frame3->setStyleSheet("QFrame {background: rgb(49, 67, 81);}");
    ui->frame4->setStyleSheet("QFrame {background: rgb(22, 27, 36);}");
    ui->frame5->setStyleSheet("QFrame {background: rgb(22, 27, 36);}");
    ui->countRadio->setChecked(false);
    ui->timeRadio->setChecked(false);
    ui->countEdit->setEnabled(false);
    ui->interval1Edit->setEnabled(false);
    ui->timeEdit->setEnabled(false);
    ui->interval2Edit->setEnabled(false);
    lineSelect = 0;
}


void Widget::on_countRadio_clicked()
{
    ui->frame4->setStyleSheet("QFrame {background: rgb(49, 67, 81);}");
    ui->frame3->setStyleSheet("QFrame {background: rgb(22, 27, 36);}");
    ui->frame5->setStyleSheet("QFrame {background: rgb(22, 27, 36);}");
    ui->loopRadio->setChecked(false);
    ui->timeRadio->setChecked(false);
    ui->countEdit->setEnabled(true);
    ui->interval1Edit->setEnabled(true);
    ui->timeEdit->setEnabled(false);
    ui->interval2Edit->setEnabled(false);
}

void Widget::on_timeRadio_clicked()
{
    ui->frame5->setStyleSheet("QFrame {background: rgb(49, 67, 81);}");
    ui->frame4->setStyleSheet("QFrame {background: rgb(22, 27, 36);}");
    ui->frame3->setStyleSheet("QFrame {background: rgb(22, 27, 36);}");
    ui->loopRadio->setChecked(false);
    ui->countRadio->setChecked(false);
    ui->countEdit->setEnabled(false);
    ui->interval1Edit->setEnabled(false);
    ui->timeEdit->setEnabled(true);
    ui->interval2Edit->setEnabled(true);
}

void Widget::on_pushButton_clicked()
{
    if(!startOrStop)
    {
        if(ui->loopRadio->isChecked())
        {
            int time = ui->delayEdit->text().toInt();
            if(time != 0)
            {
                m_timerId = startTimer(time * 1000);
                ui->pushButton->setIcon(QIcon(":/image/close.png"));
                ui->label_14->setText(QString("停止"));
                startOrStop = true;
                QListWidgetItem *item = ui->listWidget->item(1);
                item->setText(QString("信号源已开启"));
                ui->widget_6->setEnabled(false);
                ui->widget_8->setEnabled(false);
                return;
            }
        }

        if(setClient->isValid())
        {
            setClient->close();
        }
        ui->pushButton->setIcon(QIcon(":/image/close.png"));
        ui->label_14->setText(QString("停止"));
        startOrStop = true;
        QListWidgetItem *item = ui->listWidget->item(1);
        item->setText(QString("信号源已开启"));
        setClient->connectToHost("192.168.0.10", 5010);//连接

        ui->widget_6->setEnabled(false);
        ui->widget_8->setEnabled(false);
    }
    else
    {
        ui->pushButton->setIcon(QIcon(":/image/start.png"));
        ui->label_14->setText(QString("启动"));
        QListWidgetItem *item = ui->listWidget->item(1);
        item->setText(QString("信号源已关闭"));
        endFlag = true;
        if(setClient->isValid())
        {
            setClient->write(stopData);
        }
        ui->roundBar1->setValue(0);
        startOrStop = false;
        ui->widget_6->setEnabled(true);
        ui->widget_8->setEnabled(true);
        if(ui->amplifierOpen->isChecked())
        {
            ui->amplifierClose->setChecked(true);
            on_amplifierClose_clicked(true);
        }
    }
}

void Widget::startTransfer()
{
   {
        quint32 sendSize;
        if(pageInfo == 0)
        {
            bytesToWrite = localFile->size();  //剩余数据的大小
            totalBytes = localFile->size();
            pageNumber = totalBytes / (8 * 1024 *1024);
            if((totalBytes % (8 * 1024 * 1024)) != 0)
            {
                pageNumber++;
            }
            qDebug() << pageNumber;
        }

        if(pageInfo == (pageNumber - 1))
        {
            sendSize = bytesToWrite;
        }
        else
        {
            sendSize = 8 * 1024 * 1024;
        }

        QDataStream out(&outBlock, QIODevice::WriteOnly);
        out.setByteOrder(QDataStream::LittleEndian);

        out.device()->seek(0);//设置文件指针从0开始
        out << quint64(0x3A3A3A3A3A3A3A3A) << totalBytes << quint32(pageNumber) << quint32(pageInfo + 1) << sendSize;
        tcpClient->write(outBlock);  //将读到的文件发送到套接字
        outBlock.resize(0);
        out.device()->seek(0);//设置文件指针从0开始
        while(sendSize)
        {
            outBlock = localFile->read(qMin(sendSize, loadSize));
            tcpClient->write(outBlock);
            sendSize -= outBlock.size();
            bytesToWrite -= outBlock.size();
            outBlock.resize(0);
            out.device()->seek(0);//设置文件指针从0开始
        }
        outBlock.resize(0);
        out.device()->seek(0);//设置文件指针从0开始
        out << quint64(0xAAAAAAAAAAAAAAAA);
        tcpClient->write(outBlock);  //将读到的文件发送到套接字
        pageInfo++;
    }
}

void Widget::updateClientProgress(qint64)
{
    if(totalBytes != 0)
    {
        //qDebug() << "set progress value";
        int progress = (unsigned int)(((totalBytes - bytesToWrite) * 100 / totalBytes));
       // qDebug() << progress;
        ui->roundBar1->setValue(progress);
    }
    else
    {
        return;
    }

    if(bytesToWrite == 0)
    {
        localFile->close();
        sendFlag = false;
        return;
    }
}

void Widget::receiveData()
{
    QByteArray data = tcpClient->readAll();
    qDebug() << hex << data;
    if((0x4A == data[0]) && (0x5A == data[12]))//接收到命令/数据反馈
    {
       if(sendFlag)
       {
            sendFlag = false;
            startTransfer();
            return;
       }
       if(pageInfo <= (pageNumber - 1))
       {
           startTransfer();
       }
       else if(pageInfo == pageNumber)
       {
           qDebug() << "data down over!";
           pageInfo = pageNumber = 0;
         //  tcpClient->close();
       }
    }
}

void Widget::receiveSetData()
{
    QByteArray data = setClient->readAll();
    qDebug() << hex << data;
    if((0x4A == data[0]) && (0x5A == data[12]))//接收到命令/数据反馈
    {
        switch (m_cmd)
        {
        case CMD_NORMAL:
            break;

        case CMD_INIT_PRESS:
            initAD9371();
            break;

        case CMD_INIT_AD9371:
            initPL();
            break;

        case CMD_INIT_PL:
            m_cmd = CMD_NORMAL;
            break;

        default:
            break;
        }

        if(!waitSet)
        {
               qDebug() << "send data";
               waitSet = true;
               //calibrationFlag = true;
               //calibrationFlag_send();
               serialFlag = true;
               serialSend();
               return;
        }
        if(calibrationFlag)
        {
            calibrationFlag = false;
            calibrationFlag_send();
            //serialSend();
            return;
        }
        if(calibrationFlag2)
        {
            calibrationFlag2 = false;
             calibrationFlag_send();
            //serial_fre_send();
            return;
        }
        if(serialFlag)
        {
            serialFlag = false;
            serialfreFlag = true;
            serial_fre_send();
            return;
        }
        if(serialFlag1)
        {
            serialFlag1 = false;
            if(sendCalibration)
            {
                calibrationFlag2 = true;
            }
            else
            {
                calibrationFlag2 = false;
            }
            serial_fre_send();
            return;
        }
        if(serialfreFlag)
        {
            serialfreFlag = false;
            qDebug() << "send serial data";
            if(sendCalibration)
            {
                calibrationFlag = true;
            }
            else
            {
                calibrationFlag = false;
            }
            gpio_send();
            return;
        }
        if(freFlag)
        {
            qDebug() << "receive fre return";
            freFlag = false;
            serialFlag1 = true;
            serialSend();
            return;
        }
        if(endFlag)
        {
            endFlag = false;
            setClient->write(stopData);
        }
    }
}

void Widget::startSet()
{
    set_send();
}

void Widget::sendData()
{
    sendFlag = true;
    tcpClient->write(stopData);
}

/*
 * 初始化压控震荡器
*/
void Widget::pressureControl()
{
    QByteArray datagram;
    datagram.resize(21);

    //帧头
    for(int i = 0; i < 8; i++)
    {
        datagram[i] = 0x1A;
    }

    //参数个数
    datagram[8] = 0x01;

    //参数
    QString str = QString::number(m_pressValue, 16);
    QByteArray datagram2;
    if(str.size() < 4)
    {
        QString tmp;
        int length = 4 - str.size();
        for(int i = 0; i < length; i++)
        {
             tmp = tmp + "0";
        }
            str = tmp + str;
    }
    datagram2 = hexStringtoByteArray(str);
    datagram2.resize(2);
    datagram[9] = 0x0C;
    datagram[10] = 0x00;
    datagram[11] = datagram2[1];
    datagram[12] = datagram2[0];

    //帧尾
    for(int i = 0; i < 8; i++)
    {
        datagram[i + 13] = 0xAA;
    }

    setClient->write(datagram);
    m_cmd = CMD_INIT_PRESS;
}

/*
 * 初始化AD9371+射频通道参数
*/
void Widget::initAD9371()
{
    QByteArray datagram;
    datagram.resize(76);
    //帧头
    for(int i = 0; i < 8; i++)
    {
        datagram[i] = 0x0A;
    }

    datagram[8] = 0x07;
    datagram[9] = 0x00;
    datagram[10] = 0x00;
    datagram[11] = 0x00;

    //频率
    datagram[12] = 0x90;
    datagram[13] = 0x00;
    datagram[14] = 0x00;
    datagram[15] = 0x00;

    quint32 fre = 10000000;
    fre += freValue;
    quint32 amp = getAmplitude(fre);//获取频率对应的幅度值
    QString str1 = QString::number(fre, 16);
    QByteArray datagram1;
    if(str1.size() < 8)
    {
        QString tmp;
        int length = 8 - str1.size();
        for(int i = 0; i < length; i++)
        {
            tmp = tmp + "0";
        }
        str1 = tmp + str1;
    }
    datagram1 = hexStringtoByteArray(str1);
    datagram1.resize(4);

    datagram[16] = datagram1[3];
    datagram[17] = datagram1[2];
    datagram[18] = datagram1[1];
    datagram[19] = datagram1[0];

    //幅度
    datagram[20] = 0x97;
    datagram[21] = 0x00;
    datagram[22] = 0x00;
    datagram[23] = 0x00;
    QString str2 = QString::number(amp, 16);
    QByteArray datagram2;
    if(str2.size() < 2)
    {
        QString tmp;
        int length = 2 - str2.size();
        for(int i = 0; i < length; i++)
        {
             tmp = tmp + "0";
        }
            str2 = tmp + str2;
    }
    datagram2 = hexStringtoByteArray(str2);
    datagram2.resize(1);

    datagram[24] = datagram2[0];
    datagram[25] = datagram2[0];
    datagram[26] = 0x00;
    datagram[27] = 0x00;

    //功放控制
    datagram[28] = 0xB0;
    datagram[29] = 0x00;
    datagram[30] = 0x00;
    datagram[31] = 0x00;
    datagram[32] = 0x00;
    datagram[33] = 0x00;
    datagram[34] = 0x00;
    datagram[35] = 0x00;

    //射频开关控制线
    datagram[36] = 0xB1;
    datagram[37] = 0x00;
    datagram[38] = 0x00;
    datagram[39] = 0x00;
    datagram[40] = 0x0A;
    datagram[41] = 0x00;
    datagram[42] = 0x00;
    datagram[43] = 0x00;

    //衰减器控制
    datagram[44] = 0xB2;
    datagram[45] = 0x00;
    datagram[46] = 0x00;
    datagram[47] = 0x00;
    datagram[48] = 0x03;
    datagram[49] = 0x00;
    datagram[50] = 0x03;
    datagram[51] = 0x00;

    //检波开关控制
    datagram[52] = 0xB3;
    datagram[53] = 0x00;
    datagram[54] = 0x00;
    datagram[55] = 0x00;
    datagram[56] = 0x00;
    datagram[57] = 0x00;
    datagram[58] = 0x00;
    datagram[59] = 0x00;

    //功放控制
    datagram[60] = 0xB0;
    datagram[61] = 0x00;
    datagram[62] = 0x00;
    datagram[63] = 0x00;
    datagram[64] = 0x00;
    datagram[65] = 0x00;
    datagram[66] = 0x00;
    datagram[67] = 0x00;

    //帧尾
    for(int i = 0; i < 8; i++)
    {
        datagram[i + 68] = 0xAA;
    }

    setClient->write(datagram);
    m_cmd = CMD_INIT_AD9371;
}

void Widget::initPL()
{
    QByteArray datagram;
    datagram.resize(64);
    //帧头
    for(int i = 0; i < 8; i++)
    {
        datagram[i] = 0x1A;
    }

    //参数个数
    datagram[8] = 0x0C;
    datagram[9] = 0x00;
    datagram[10] = 0x00;
    datagram[11] = 0x00;

    //参数
    datagram[12] = 0x01;
    datagram[13] = 0x00;
    datagram[14] = 0xFF;
    datagram[15] = 0xFF;

    datagram[16] = 0x02;
    datagram[17] = 0x00;
    datagram[18] = 0xFF;
    datagram[19] = 0xFF;

    datagram[20] = 0x03;
    datagram[21] = 0x00;
    datagram[22] = 0xFF;
    datagram[23] = 0xFF;

    datagram[24] = 0x04;
    datagram[25] = 0x00;
    datagram[26] = 0xFF;
    datagram[27] = 0xFF;

    datagram[28] = 0x05;
    datagram[29] = 0x00;
    datagram[30] = 0xFF;
    datagram[31] = 0xFF;

    datagram[32] = 0x06;
    datagram[33] = 0x00;
    datagram[34] = 0xFF;
    datagram[35] = 0xFF;

    datagram[36] = 0x07;
    datagram[37] = 0x00;
    datagram[38] = 0x00;
    datagram[39] = 0x00;

    datagram[40] = 0x08;
    datagram[41] = 0x00;
    datagram[42] = 0x00;
    datagram[43] = 0x20;

    datagram[44] = 0x09;
    datagram[45] = 0x00;
    datagram[46] = 0x00;
    datagram[47] = 0x00;

    datagram[48] = 0x0A;
    datagram[49] = 0x00;
    datagram[50] = 0x00;
    datagram[51] = 0x00;

    datagram[52] = 0x0B;
    datagram[53] = 0x00;
    datagram[54] = 0x01;
    datagram[55] = 0x00;

    //帧尾
    for(int i = 0; i < 8; i++)
    {
        datagram[i + 56] = 0xAA;
    }

    setClient->write(datagram);
    m_cmd = CMD_INIT_PL;
}

void Widget::on_openBtn_clicked()
{
    this->fileName.clear();
    this->fileName = QFileDialog::getOpenFileName(this, "请选择一个波形文件", "D:/data/");
    if(this->fileName == NULL)
    {
        qDebug() << "not select file";
        return;
    }
    QFileInfo info(fileName);
    QString name = info.fileName();

    if(name.contains('_'))
    {
        QStringList list1 = name.split('_');
        QListWidgetItem *item = ui->listWidget->item(3);
        item->setText(list1.at(0));

        QStringList list2 = list1.at(1).split('.');
        item = ui->listWidget->item(4);
        item->setText("码速率" + list2.at(0));
    }
    else
    {
        QStringList list1 = name.split('.');
        QListWidgetItem *item = ui->listWidget->item(3);
        item->setText(list1.at(0));

        item = ui->listWidget->item(4);
        item->setText(" ");
    }

    if(startOrStop)
    {
        ui->pushButton->setIcon(QIcon(":/image/start.png"));
        ui->label_14->setText(QString("启动"));
        QListWidgetItem *item = ui->listWidget->item(1);
        item->setText(QString("信号源已关闭"));
        endFlag = true;
        ui->roundBar1->setValue(0);
        startOrStop = false;
        ui->widget_6->setEnabled(true);
        ui->widget_8->setEnabled(true);
        if(ui->amplifierOpen->isChecked())
        {
            ui->amplifierClose->setChecked(true);
            on_amplifierClose_clicked(true);
        }
    }

    localFile = new QFile(fileName);
    if(!localFile->open(QFile::ReadOnly))
    {
        qDebug() << "open file error";
        return;
    }
    bytesWritten = 0;//初始化已发送字节为0
    pageInfo = 0;
    loadSize = 4*1024;
    totalBytes = 0;
    bytesToWrite = 0;
    ui->roundBar1->setValue(0);
    if(tcpClient->isValid())
    {
        tcpClient->close();
    }
    tcpClient->connectToHost("192.168.0.10", 5010);//连接
}

void Widget::serialSend()
{
    //serialFlag = true;
    if(setClient->isValid())
    {
        setClient->write(serialData);
    }
}

void Widget::fwidgetClick()
{
    lineSelect = 0;
    if(currentIndex != 1)
    {
        myTumbler* wid = m_index[1];
        wid->setLineFlag(true);
        wid->update();
        wid = m_index[currentIndex];
        wid->setLineFlag(false);
        wid->update();
    }
    currentIndex = 1;

    ui->countEdit->clearFocus();
    ui->interval1Edit->clearFocus();
    ui->timeEdit->clearFocus();
    ui->interval2Edit->clearFocus();
}

void Widget::dwidgetClick()
{
    lineSelect = 0;
    if(currentIndex != 9)
    {
        myTumbler* wid = m_index[9];
        wid->setLineFlag(true);
        wid->update();
        wid = m_index[currentIndex];
        wid->setLineFlag(false);
        wid->update();
    }
    currentIndex = 9;

    ui->countEdit->clearFocus();
    ui->interval1Edit->clearFocus();
    ui->timeEdit->clearFocus();
    ui->interval2Edit->clearFocus();
}


void Widget::on_amplifierClose_clicked(bool checked)
{
    if(checked)
    {
        QListWidgetItem *item = ui->listWidget->item(2);
        item->setText(QString("功放(关)"));
        if(ui->label_4->text() != "-")
        {
            ui->label_4->setText("-");

            ui->dwidget2->setMaxIndex(9);
            ui->dwidget1->setMaxIndex(9);

            QString str_value = QString::number(ui->dwidget1->getCurrentIndex()) + QString::number(ui->dwidget2->getCurrentIndex()) + QString::number(ui->dwidget3->getCurrentIndex());
            int value = str_value.toInt();
            value = 370 - value;

            ui->dwidget1->setCurrentIndex(value / 100 % 10);
            ui->dwidget2->setCurrentIndex(value / 10 % 10);
            ui->dwidget3->setCurrentIndex(value % 10);
        }
        else
        {
            ui->dwidget2->setMaxIndex(9);
            ui->dwidget1->setMaxIndex(9);

            QString str_value = QString::number(ui->dwidget1->getCurrentIndex()) + QString::number(ui->dwidget2->getCurrentIndex()) + QString::number(ui->dwidget3->getCurrentIndex());
            int value = str_value.toInt();
            value = 370 + value;

            ui->dwidget1->setCurrentIndex(value / 100 % 10);
            ui->dwidget2->setCurrentIndex(value / 10 % 10);
            ui->dwidget3->setCurrentIndex(value % 10);
        }
     }
}

void Widget::on_amplifierOpen_clicked(bool checked)
{
    if(checked)
    {
        QString result = AmplifierWidget::showMyMessageBox(this);
        if(result == "ok")
        {
            QListWidgetItem *item = ui->listWidget->item(2);
            item->setText(QString("功放(开)"));
            QString str_value = QString::number(ui->dwidget1->getCurrentIndex()) + QString::number(ui->dwidget2->getCurrentIndex()) + QString::number(ui->dwidget3->getCurrentIndex());
            int value = str_value.toInt();
            value = 370 - value;
            if(value < 0)
            {
                value = -value;
                ui->label_4->setText("-");
            }
            else
            {
                ui->label_4->setText("");
            }

            ui->dwidget1->setCurrentIndex(value / 100 % 10);
            ui->dwidget2->setCurrentIndex(value / 10 % 10);
            ui->dwidget3->setCurrentIndex(value % 10);

            ui->dwidget1->setMaxIndex(3);
        }
        else
        {
            ui->amplifierOpen->setChecked(false);
            ui->amplifierClose->setChecked(true);
        }
    }
}

void Widget::freWidget1Click()
{
    lineSelect = 0;
    myTumbler* wid = m_index[currentIndex];
    wid->setLineFlag(false);
    wid->update();
    wid = m_index[1];
    wid->setLineFlag(true);
    wid->update();
    currentIndex = 1;
}

void Widget::freWidget2Click()
{
    lineSelect = 0;
    myTumbler* wid = m_index[currentIndex];
    wid->setLineFlag(false);
    wid->update();
    wid = m_index[2];
    wid->setLineFlag(true);
    wid->update();
    currentIndex = 2;
}

void Widget::freWidget3Click()
{
    lineSelect = 0;
    myTumbler* wid = m_index[currentIndex];
    wid->setLineFlag(false);
    wid->update();
    wid = m_index[3];
    wid->setLineFlag(true);
    wid->update();
    currentIndex = 3;
}

void Widget::freWidget4Click()
{
    lineSelect = 0;
    myTumbler* wid = m_index[currentIndex];
    wid->setLineFlag(false);
    wid->update();
    wid = m_index[4];
    wid->setLineFlag(true);
    wid->update();
    currentIndex = 4;
}

void Widget::freWidget5Click()
{
    lineSelect = 0;
    myTumbler* wid = m_index[currentIndex];
    wid->setLineFlag(false);
    wid->update();
    wid = m_index[5];
    wid->setLineFlag(true);
    wid->update();
    currentIndex = 5;
}

void Widget::freWidget6Click()
{
    lineSelect = 0;
    myTumbler* wid = m_index[currentIndex];
    wid->setLineFlag(false);
    wid->update();
    wid = m_index[6];
    wid->setLineFlag(true);
    wid->update();
    currentIndex = 6;
}

void Widget::freWidget7Click()
{
    lineSelect = 0;
    myTumbler* wid = m_index[currentIndex];
    wid->setLineFlag(false);
    wid->update();
    wid = m_index[7];
    wid->setLineFlag(true);
    wid->update();
    currentIndex = 7;
}

void Widget::freWidget8Click()
{
    lineSelect = 0;
    myTumbler* wid = m_index[currentIndex];
    wid->setLineFlag(false);
    wid->update();
    wid = m_index[8];
    wid->setLineFlag(true);
    wid->update();
    currentIndex = 8;
}

void Widget::freWidget9Click()
{
    lineSelect = 0;
    myTumbler* wid = m_index[currentIndex];
    wid->setLineFlag(false);
    wid->update();
    wid = m_index[9];
    wid->setLineFlag(true);
    wid->update();
    currentIndex = 9;
}

void Widget::freWidget10Click()
{
    lineSelect = 0;
    myTumbler* wid = m_index[currentIndex];
    wid->setLineFlag(false);
    wid->update();
    wid = m_index[10];
    wid->setLineFlag(true);
    wid->update();
    currentIndex = 10;
}

void Widget::freWidget11Click()
{
    lineSelect = 0;
    myTumbler* wid = m_index[currentIndex];
    wid->setLineFlag(false);
    wid->update();
    wid = m_index[11];
    wid->setLineFlag(true);
    wid->update();
    currentIndex = 11;
}

void Widget::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Escape)
    {
        close();
    }
}

void Widget::setAutoStart(bool is_auto_start)
{
    QString application_name = QApplication::applicationName();
    QSettings *settings = new QSettings(REG_RUN, QSettings::NativeFormat);
    if(is_auto_start)
    {
        QString application_path = QApplication::applicationFilePath();
        settings->setValue(application_name, application_path.replace("/", "\\"));
    }
    else
    {
        settings->remove(application_name);
    }
    delete settings;
}

bool Widget::MySystemShutDown()
 {
     HANDLE hToken;
     TOKEN_PRIVILEGES tkp;

     //获取进程标志
     if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
          return false;

     //获取关机特权的LUID
     LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME,    &tkp.Privileges[0].Luid);
     tkp.PrivilegeCount = 1;
     tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

     //获取这个进程的关机特权
     AdjustTokenPrivileges(hToken, false, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
     if (GetLastError() != ERROR_SUCCESS) return false;

     // 强制关闭计算机
     if ( !ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, 0))
           return false;
     return true;
/*
    // 强制重启计算机
     if ( !ExitWindowsEx(EWX_REBOOT| EWX_FORCE, 0))
          return false;
     return true;
*/
}

void Widget::initDevice()
{
    if (!m_bInit)
    {
        m_bInit = true;
        pressureControl();
    }
}

