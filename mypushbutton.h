#ifndef MYPUSHBUTTON_H
#define MYPUSHBUTTON_H

#include <QObject>
#include <QWidget>
#include <QPushButton>
#include <QEvent>
#include <QList>
#include <QMouseEvent>
#include <QTimer>
#include <QSlider>

class myPushButton : public QPushButton
{
    Q_OBJECT
public:
    myPushButton(QWidget*parent=0);
    myPushButton(const QString& str,QWidget*parent=0);
};

class volButton:public QPushButton
{
    Q_OBJECT
public:
    volButton(const QString& normal, const QString& hover, const QString& pressed, QWidget*parent=0);
    void setPartnerSlider(QSlider *slider){ m_partnerslider = slider; }

    void startTimer(int interval){ m_timer.start(interval); }

    void stopTimer(){ m_timer.stop(); }
    QTimer m_timer;
public slots:

protected:
    void enterEvent(QEvent*);
    void leaveEvent(QEvent*);
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *e)override;
    void mouseReleaseEvent(QMouseEvent *e)override;

private:
    QSlider *m_partnerslider;
    bool m_isenter;
    int m_savevol;
    int m_isvolempty;
    int m_index;
    QVector<QPixmap> m_listnormal;
    QVector<QPixmap> m_listhover;
    QVector<QPixmap> m_listpressed;


public slots:
    void setButtonPixmap(int);//getFromSlider
signals:
    void setMute(int);
    void sig_hideVolWidget();
};



#endif // MYPUSHBUTTON_H
