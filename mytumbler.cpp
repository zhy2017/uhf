#include "mytumbler.h"
#include "qpainter.h"
#include "qevent.h"
#include "qdebug.h"

myTumbler::myTumbler(QWidget *parent) : QWidget(parent)
{
    currentIndex = 0;
    currentValue = "1";
    maxIndex = 9;

    for (int i = 1; i <= 12; i++) {
        listValue.append(QString::number(i));
    }

    foreground = QColor(140, 140, 140);
    background = QColor(22, 27, 36);
    lineColor = QColor(0, 255, 0);
    textColor = QColor(255, 255, 255);
    lineFlag = false;

    percent = 5;
    offset = 0;
    pressed = 0;
    pressedPos = 0;
    currentPos = 0;

    setFont(QFont("Arial", 6));
}

void myTumbler::mousePressEvent(QMouseEvent *e)
{
    pressed = true;
    int target = e->pos().y();

    pressedPos = target;
}

void myTumbler::mouseMoveEvent(QMouseEvent *e)
{
    int count = listValue.count();

    if (count <= 1)
    {
        return;
    }

    int pos = e->pos().y();
    int target = this->height();


    int index = listValue.indexOf(currentValue);

    if (pressed & lineFlag)
    {
        //数值到边界时,阻止继续往对应方向移动
        if ((index == 0 && pos >= pressedPos) || (index == count - 1 && pos <= pressedPos))
        {
            return;
        }

        offset = pos - pressedPos;

        //若移动速度过快时进行限制
        if (offset > target / percent) {
            offset = target / percent;
        } else if (offset < -target / percent) {
            offset = -target / percent;
        }

        static int oldIndex = -1;

        if (oldIndex != index) {
            emit currentIndexChanged(index);
            emit currentValueChanged(listValue.at(index));
            oldIndex = index;
        }

        update();
    }
}

void myTumbler::mouseReleaseEvent(QMouseEvent */*e*/)
{
    if (pressed & lineFlag)
    {
        pressed = false;
        //矫正到居中位置
        checkPosition();
    }
    emit Clicked();
}

void myTumbler::paintEvent(QPaintEvent *)
{
    //绘制准备工作,启用反锯齿
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    int count = listValue.count();

    if (count <= 1)
    {
        return;
    }

    int target = this->height();

    int index = listValue.indexOf(currentValue);

    //当右移偏移量大于比例且当前值不是第一个则索引-1
    if (offset >= target / percent && index > 0)
    {
        pressedPos += target / percent;
        offset -= target / percent;
        index -= 1;
    }

    //当左移偏移量小于比例且当前值不是末一个则索引+1
    if (offset <= -target / percent && index < count - 1) {
        pressedPos -= target / percent;
        offset += target / percent;
        index += 1;
    }

    if(currentIndex <= maxIndex)
    {
        currentIndex = index;
        currentValue = listValue.at(index);
    }

    //绘制背景
    drawBg(&painter);

    //绘制中间值
    painter.setPen(textColor);
    drawText(&painter, index, offset);
    painter.setPen(foreground);

    if(lineFlag)
    {
        drawLine(&painter);
    }
}

void myTumbler::drawBg(QPainter *painter)
{
    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(background);
    painter->drawRect(rect());
    painter->restore();
}

void myTumbler::drawLine(QPainter *painter)
{
    int width = this->width();
    int height = this->height();

    painter->save();
    painter->setBrush(Qt::NoBrush);

    QPen pen = painter->pen();
    pen.setWidth(4);
    pen.setColor(lineColor);
    pen.setCapStyle(Qt::RoundCap);
    painter->setPen(pen);

   // QString strValue = listValue.at(currentIndex);
    int textWidth = painter->fontMetrics().height();
    textWidth = textWidth + 10;

    painter->drawLine(width / 2, 0, width / 2, height / 2 - textWidth);
    painter->drawLine(width / 2, height / 2 + textWidth, width / 2, height);

    painter->restore();
}

void myTumbler::drawText(QPainter *painter, int index, int offset)
{
    painter->save();

    int width = this->width();
    int height = this->height();
    QString strValue = listValue.at(index);

    int target = height;

    QFont font = painter->font();
    font.setPixelSize((target - qAbs(offset)) / 4);
    painter->setFont(font);


    int textHeight = painter->fontMetrics().height();
    int initY = height / 2 + offset - textHeight / 2;
    painter->drawText(QRect(0, initY, width, textHeight), Qt::AlignCenter, strValue);

    //计算最后中间值停留的起始坐标,以便鼠标松开时矫正居中
    if (index == currentIndex)
    {
        currentPos = initY;
    }

    painter->restore();
}

void myTumbler::checkPosition()
{
    //上下滑动样式,往上滑动时,offset为负数,当前值所在Y轴坐标小于高度的一半,则将当前值设置为下一个值
    //上下滑动样式,往下滑动时,offset为正数,当前值所在Y轴坐标大于高度的一半,则将当前值设置为上一个值
    if (offset < 0)
    {
        //if (currentPos < target / 2)
      //  {
            offset = 0;
            setCurrentIndex(currentIndex + 1);
       // }
    }
    else
    {
       // if (currentPos > target / 2)
       // {
            offset = 0;
            setCurrentIndex(currentIndex - 1);
       // }
    }
}

QStringList myTumbler::getListValue() const
{
    return this->listValue;
}

int myTumbler::getCurrentIndex() const
{
    return this->currentIndex;
}

int myTumbler::getMaxIndex() const
{
    return maxIndex;
}

QString myTumbler::getCurrentValue() const
{
    return this->currentValue;
}

QColor myTumbler::getForeground() const
{
    return this->foreground;
}

QColor myTumbler::getBackground() const
{
    return this->background;
}

QColor myTumbler::getTextColor() const
{
    return this->textColor;
}

QSize myTumbler::sizeHint() const
{
    return QSize(50, 150);
}

QSize myTumbler::minimumSizeHint() const
{
    return QSize(10, 10);
}

void myTumbler::setListValue(const QStringList &listValue)
{
    if (listValue.count() > 0) {
        this->listValue = listValue;
        setCurrentIndex(0);
        setCurrentValue(listValue.at(0));
        update();
    }
}

void myTumbler::setCurrentIndex(int currentIndex)
{
    if (currentIndex >= 0) {
        this->currentIndex = currentIndex;
        this->currentValue = listValue.at(currentIndex);
        emit currentIndexChanged(currentIndex);
        update();
    }
}

void myTumbler::setMaxIndex(int maxIndex)
{
    this->maxIndex = maxIndex;
}

void myTumbler::setCurrentValue(const QString &currentValue)
{
    if (listValue.contains(currentValue)) {
        this->currentValue = currentValue;
        this->currentIndex = listValue.indexOf(currentValue);
        emit currentValueChanged(currentValue);
        update();
    }
}

void myTumbler::setLineFlag(bool value)
{
    lineFlag = value;
}

void myTumbler::setForeground(const QColor &foreground)
{
    if (this->foreground != foreground){
        this->foreground = foreground;
        update();
    }
}

void myTumbler::setBackground(const QColor &background)
{
    if (this->background != background){
        this->background = background;
        update();
    }
}

void myTumbler::setTextColor(const QColor &textColor)
{
    if (this->textColor != textColor){
        this->textColor = textColor;
        update();
    }
}

void myTumbler::setLineColor(const QColor &lineColor)
{
    if(this->lineColor != lineColor)
    {
        this->lineColor = lineColor;
        update();
    }
}

