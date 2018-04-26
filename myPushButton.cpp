#include "mypushbutton.h"
#include <QPainter>
#include <QDebug>
#include <QFontMetrics>
#include <QToolTip>

myPushButton::myPushButton(QWidget*parent) : QPushButton(parent)
{
    setCursor(Qt::PointingHandCursor);
    setFlat(true);
    setStyleSheet("QPushButton{background:transparent;}");
}
myPushButton::myPushButton(const QString& str, QWidget *parent) : QPushButton(str, parent)
{
     setCursor(Qt::PointingHandCursor);
     setFlat(true);
     setStyleSheet("QPushButton{background:transparent;}");
}

volButton::volButton(const QString& normal, const QString& hover, const QString& pressed, QWidget*parent)
    : QPushButton(parent)//图片5个连一串
    , m_partnerslider(NULL)
    , m_isenter(false)
    , m_savevol(100)
    , m_isvolempty(100)
    , m_index(0)
{
    setCursor(Qt::PointingHandCursor);

    QPixmap pixTemp(normal);
    for(int i = 0; i < 5; i++)
    m_listnormal << pixTemp.copy(i * (pixTemp.width() / 5), 0, pixTemp.width() / 5, pixTemp.height());

    pixTemp.load(hover);
    for(int i = 0; i < 5; i++)
    m_listhover << pixTemp.copy(i * (pixTemp.width() / 5), 0, pixTemp.width() / 5, pixTemp.height());

    pixTemp.load(pressed);
    for(int i = 0; i < 5; i++)
    m_listpressed << pixTemp.copy(i * (pixTemp.width() / 5), 0, pixTemp.width() / 5, pixTemp.height());


    m_timer.setSingleShot(true);
    connect(&m_timer, SIGNAL(timeout()), this, SIGNAL(sig_hideVolWidget()));
}

void volButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    if(!m_isenter)
    {
        p.drawPixmap((width() - m_listnormal.at(0).width()) / 2, (height() - m_listnormal.at(0).height()) / 2, m_listnormal.at(m_index));
    }
    else
    {
        p.drawPixmap((width() - m_listhover.at(0).width()) / 2, (height() - m_listhover.at(0).height()) / 2, m_listhover.at(m_index));
    }
}
void volButton::mousePressEvent(QMouseEvent *e)
{
    if(e->button() == Qt::LeftButton)
    {
        QPushButton::mousePressEvent(e);
    }
}
void volButton::mouseReleaseEvent(QMouseEvent *e)
{
    if(e->button() == Qt::LeftButton)
    {

        if(this->contentsRect().contains(mapFromGlobal(QCursor::pos())))
        {
            if(m_isvolempty == 0)//如果没有音量就 设置一个
            {
                emit setMute(m_savevol);
            }
            else//如果有音量 设置音量为0;
            {
                m_savevol = m_partnerslider->value();
                emit setMute(0);
            }
        }
       QPushButton::mouseReleaseEvent(e);
    }
}

void volButton::setButtonPixmap(int value)
{
    m_isenter = false;
    if(value == 0)
    {
        m_index = 4;
    }
    if(value > 2 && value <= 30)
    {
       m_index = 1;
    }
    if(value > 30)
    {
       m_index = 2;
    }
    update();
    m_isvolempty = value;//判断值为0;
}
void volButton::enterEvent(QEvent *)
{
    m_isenter = true;
    m_timer.stop();
    update();
}
void volButton::leaveEvent(QEvent *)
{
    m_isenter = false;
    m_timer.start(500);
    update();
}

