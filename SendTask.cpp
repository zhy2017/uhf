#include "SendTask.h"
#include <string.h>
#include <QByteArray>
#include <QDebug>
#include <QTcpSocket>
#include <QHostAddress>
#include <QFile>
#include <QDataStream>


SendTask::SendTask(QObject *parent)
    : QThread(parent)
{
   // tcpClient = new QTcpSocket();

    m_tackOverFlag = 0;
    m_sendFlag = 0;
    pageInfo = 0;
    pageNumber = 0;
    loadSize = 4 * 1024;  //每次发送数据的大小
    connectFlag = 0;
    syncFlag = 0;
    //connect(tcpClient, SIGNAL(error()), this, SLOT(handelError()));
    Create();
}

SendTask::~SendTask()
{
    if(tcpClient->isValid())
    {
        tcpClient->close();
    }
    quit();
    wait();
}

int SendTask::Create()
{
    //QThread::sleep(1);
    start(); // 运行线程
    return 0;
}

void SendTask::startSend(QString filename)
{
    if(!m_tackOverFlag)
    {
        localFile = new QFile(filename);
        bool result = localFile->open(QFile::ReadOnly);

        qDebug() << result;
        m_sendFlag = 1;
        m_tackOverFlag = 1;
        pageInfo = 0;
        pageNumber = 0;
        loadSize = 4 * 1024;  //每次发送数据的大小
        m_fileName = filename;
        m_progress = 0;
    }
}

unsigned int SendTask::GetProgress()
{
    if(totalSize == 0)
    {
        return 0;
    }
    m_progress = (unsigned int)(((totalSize - byteToWrite) / totalSize) * 100);
    return m_progress;
}

// 线程的主函数
void SendTask::run()
{
    tcpClient = new QTcpSocket();
    tcpClient->connectToHost(QHostAddress("192.168.0.110"), 1000);
    //connect(tcpClient, SIGNAL(connected()), this, SLOT(connectOk()));
    //connect(tcpClient, SIGNAL(disconnected()), this, SLOT(connectBreak()));
    tcpClient->write("11111");

    while(1)
    {
        if(!connectFlag && syncFlag)
        {
           QThread::sleep(1);
            qDebug() << "connect";
            tcpClient->connectToHost(QHostAddress("192.168.0.110"), 1000);
        }
        if(m_sendFlag)
        {
           // sendData();
            quint32 sendSize;
            qDebug() << "send data";
            if(pageInfo == 0)
            {
                byteToWrite = localFile->size();  //剩余数据的大小
                totalSize = localFile->size();
                pageNumber = totalSize / (8 * 1024 *1024);
                if((totalSize % (8 * 1024 * 1024)) != 0)
                {
                    pageNumber++;
                }
                qDebug() << pageNumber;
            }

            if(pageInfo == (pageNumber - 1))
            {
                sendSize = byteToWrite;
                m_tackOverFlag = 0;
            }
            else
            {
                sendSize = 1024 * 1024;
            }

            QDataStream out(&outBlock, QIODevice::WriteOnly);
            out.setByteOrder(QDataStream::LittleEndian);

            out.device()->seek(0);//设置文件指针从0开始
            out << quint64(0x3A3A3A3A3A3A3A3A) << totalSize << quint32(pageNumber) << quint32(pageInfo + 1) << sendSize;
            qDebug() << "send data1";
            qDebug() << outBlock;
            tcpClient->write(outBlock);  //将读到的文件发送到套接字
            qDebug() << "send data2";
            outBlock.resize(0);
            out.device()->seek(0);//设置文件指针从0开始
            while(sendSize)
            {
                outBlock = localFile->read(qMin(sendSize, loadSize));
                tcpClient->write(outBlock);
                sendSize -= outBlock.size();
                byteToWrite -= outBlock.size();
                outBlock.resize(0);
                out.device()->seek(0);//设置文件指针从0开始
            }
            outBlock.resize(0);
            out.device()->seek(0);//设置文件指针从0开始
            out << quint64(0xAAAAAAAAAAAAAAAA);
            tcpClient->write(outBlock);  //将读到的文件发送到套接字
            pageInfo++;
            m_sendFlag = 0;
        }
        QThread::msleep(100);
    }
}

void SendTask::sendData()
{
    quint32 sendSize;
    qDebug() << "send data";
    if(pageInfo == 0)
    {
        byteToWrite = localFile->size();  //剩余数据的大小
        totalSize = localFile->size();
        pageNumber = totalSize / (8 * 1024 *1024);
        if((totalSize % (8 * 1024 * 1024)) != 0)
        {
            pageNumber++;
        }
    }

    if(pageInfo == (pageNumber - 1))
    {
        sendSize = byteToWrite;
        m_tackOverFlag = 0;
    }
    else
    {
        sendSize = 1024 * 1024;
    }

    QDataStream out(&outBlock, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::LittleEndian);

    out.device()->seek(0);//设置文件指针从0开始
    out << quint64(0x3A3A3A3A3A3A3A3A) << totalSize << quint32(pageNumber) << quint32(pageInfo + 1) << sendSize;
    qDebug() << "send data1";
    qDebug() << outBlock;
    tcpClient->write(outBlock);  //将读到的文件发送到套接字
    qDebug() << "send data2";
    outBlock.resize(0);
    out.device()->seek(0);//设置文件指针从0开始
    while(sendSize)
    {
        outBlock = localFile->read(qMin(sendSize, loadSize));
        tcpClient->write(outBlock);
        sendSize -= outBlock.size();
        byteToWrite -= outBlock.size();
        outBlock.resize(0);
        out.device()->seek(0);//设置文件指针从0开始
    }
    outBlock.resize(0);
    out.device()->seek(0);//设置文件指针从0开始
    out << quint64(0xAAAAAAAAAAAAAAAA);
    tcpClient->write(outBlock);  //将读到的文件发送到套接字
    pageInfo++;
    m_sendFlag = 0;
}

void SendTask::receiveData()
{
    //QByteArray data = tcpClient->readAll();
  //  if((data[0] == 0x4A) && (data[12] == 0x5A))
 //   {
 //      if(pageInfo != 0)
 //      {
 //         m_sendFlag = 1;
 //      }
  //  }
}

void SendTask::connectOk()
{
   // connectFlag = 1;
    qDebug() << "connect ok";
    connect(tcpClient, SIGNAL(readyRead()), this, SLOT(receiveData()));
    qDebug() << "connect over";
    connectFlag = 1;
    syncFlag = 1;
}

void SendTask::connectBreak()
{
    qDebug() << "connect break";
    connectFlag = 0;
    syncFlag = 0;
}

void SendTask::handelError()
{
    qDebug() << "error";
}




