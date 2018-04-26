#ifndef SENDTASK_H
#define SENDTASK_H

#include <QThread>

class QTcpSocket;
class QFile;

class SendTask : public QThread
{
    Q_OBJECT

public:
    SendTask(QObject *parent);
    ~SendTask();

    // 创建与销毁线程
    int Create();
    void startSend(QString filename);

    unsigned int GetProgress();

private slots:
    void receiveData();
    void connectOk();
    void connectBreak();
    void handelError();

private:
    // 线程的入口函数
    void run();
    void CreateUdpSocket();


private:
    void sendData();
    QTcpSocket *tcpClient;
    QFile *localFile;
    QFile *saveFile;
    QString m_fileName;  //文件名

    QByteArray outBlock;  //分次传
    quint32 loadSize;  //每次发送数据的大小
    quint32 byteToWrite;  //剩余数据大小
    quint32 totalSize;  //文件总大小
    unsigned int m_progress;//发送百分比
    quint8  m_sendFlag;//发送标志位
    quint8 m_tackOverFlag;//任务完成标志
    quint8  pageInfo;
    quint8 pageNumber;
    quint8 connectFlag;//连接成功标志
    quint8 syncFlag;//同步标志
};

#endif
