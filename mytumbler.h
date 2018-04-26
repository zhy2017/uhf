#ifndef MYTUMBLER_H
#define MYTUMBLER_H

#include <QWidget>

class myTumbler : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ getCurrentIndex WRITE setCurrentIndex)
    Q_PROPERTY(QString currentValue READ getCurrentValue WRITE setCurrentValue)

    Q_PROPERTY(QColor foreground READ getForeground WRITE setForeground)
    Q_PROPERTY(QColor background READ getBackground WRITE setBackground)
    Q_PROPERTY(QColor textColor READ getTextColor WRITE setTextColor)

public:
    explicit myTumbler(QWidget *parent = 0);

protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void paintEvent(QPaintEvent *e);
    void drawBg(QPainter *painter);
    void drawLine(QPainter *painter);
    void drawText(QPainter *painter, int index, int offset);

private:
    QStringList listValue;          //值队列
    int currentIndex;               //当前索引
    int maxIndex;                   //最大值索引
    QString currentValue;           //当前值

    QColor foreground;              //前景色
    QColor background;              //背景色
    QColor lineColor;               //线条颜色
    QColor textColor;               //当前文本颜色

    int percent;                    //比例
    int offset;                     //偏离值
    bool pressed;                   //鼠标是否按下
    bool lineFlag;                  //是否画线
    int pressedPos;                 //按下处坐标
    int currentPos;                 //当前值对应起始坐标

private:
    void checkPosition();

public:
    QStringList getListValue()      const;
    int getCurrentIndex()           const;
    int getMaxIndex()               const;
    QString getCurrentValue()       const;

    QColor getForeground()          const;
    QColor getBackground()          const;
    QColor getTextColor()           const;

    QSize sizeHint()                const;
    QSize minimumSizeHint()         const;

public Q_SLOTS:
    //设置值队列
    void setListValue(const QStringList &listValue);
    //设置当前索引
    void setCurrentIndex(int currentIndex);
    //设置最大值索引
    void setMaxIndex(int maxIndex);
    //设置当前值
    void setCurrentValue(const QString &currentValue);

    //设置是否画线
    void setLineFlag(bool value);

    //设置前景色
    void setForeground(const QColor &foreground);
    //设置背景色
    void setBackground(const QColor &background);
    //设置文本颜色
    void setTextColor(const QColor &textColor);

    void setLineColor(const QColor &lineColor);

signals:
    void currentIndexChanged(int currentIndex);
    void currentValueChanged(QString currentValue);
    void Clicked();
};

#endif // MYTUMBLER_H
