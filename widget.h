#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "mytumbler.h"
#include <QMap>

class QUdpSocket;
class SendTask;
class QTcpSocket;
class QFile;
class QSerialPort;

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();
    void formatString(QString &org, int n = 2, const QChar &ch = QChar( ));
    QByteArray hexStringtoByteArray(QString hex);
    virtual void timerEvent (QTimerEvent * event);

public slots:
    bool eventFilter(QObject *watched, QEvent *event);

private slots:
    void on_upBtn_clicked();

    void on_leftBtn_clicked();

    void on_downBtn_clicked();

    void on_rightBtn_clicked();

    void on_pushButton_3_clicked();

    void on_num1Btn_clicked();

    void on_num2Btn_clicked();

    void on_num3Btn_clicked();

    void on_num4Btn_clicked();

    void on_num5Btn_clicked();

    void on_num6Btn_clicked();

    void on_num7Btn_clicked();

    void on_num8Btn_clicked();

    void on_num9Btn_clicked();

    void on_negativeBtn_clicked();

    void on_num0Btn_clicked();

    void on_num000Btn_clicked();

    void on_delBtn_clicked();

    void on_acBtn_clicked();

    void on_okBtn_clicked();

    void on_loopRadio_clicked();

    void on_countRadio_clicked();

    void on_timeRadio_clicked();

    void on_pushButton_clicked();
    void startTransfer();//发送文件大小等信息
    void updateClientProgress(qint64);//发送数据，更新进度条
    void receiveData();
    void receiveSetData();
    void startSet();
    void sendData();
    void on_openBtn_clicked();
    void serialSend();
    void fwidgetClick();
    void dwidgetClick();

    void on_amplifierClose_clicked(bool checked);

    void on_amplifierOpen_clicked(bool checked);
    void freWidget1Click();
    void freWidget2Click();
    void freWidget3Click();
    void freWidget4Click();
    void freWidget5Click();
    void freWidget6Click();
    void freWidget7Click();
    void freWidget8Click();
    void freWidget9Click();
    void freWidget10Click();
    void freWidget11Click();

protected:
    void keyPressEvent(QKeyEvent *event);

private:
    Ui::Widget *ui;
    int lineSelect;//输入框选择
    void initForm();
    void initSerial();
    void initStopData();
    void initExcelData();
    void initseData();
    void setAutoStart(bool is_auto_start);
    int getAmplitude(int fre);
    bool MySystemShutDown();
    QTcpSocket *tcpClient;
    QTcpSocket *setClient;
    QFile *localFile;//要发送的文件
    quint32 totalBytes;//数据总大小
    qint64 bytesWritten;//已经发送数据大小
    qint64 bytesToWrite;//剩余数据大小
    quint32 loadSize;//每次发送数据大小
    QString fileName;//保存文件路径
    QByteArray outBlock;//数据缓冲区，即存放每次要发送的数据
    quint8  pageInfo;
    quint8 pageNumber;
    bool sendFlag;
    bool waitFMC;
    bool waitSet;
    bool waitDown;
    bool calibrationFlag;//校准
    bool calibrationFlag2;//OK键按下校准
    bool startOrStop;//控制启动停止状态
    bool serialFlag;//串口发送1
    bool serialFlag1;//ok键按下串口发送
    bool serialfreFlag;//串口发送2
    bool endFlag;
    bool freFlag;//用于启动后控制按下ok键频率幅度发送
    bool sendCalibration;//是否发送校准命令
    SendTask *m_sendTask;
    int maxIndex;
    int m_timerId;
    int currentIndex;
    QMap<int, myTumbler*> m_index;
    QByteArray serialData;
    QByteArray serialFreData;
    QByteArray stopData;
    QByteArray seData;//校准命令
    QMap<int, int>  m_excelData;
    quint32 preFre;//保存上一次的频率值
    void gpio_send();
    void set_send();
    void fre_send();
    void serial_fre_send();
    void calibrationFlag_send();

};

#endif // WIDGET_H
