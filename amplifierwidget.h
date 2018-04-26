#ifndef AMPLIFIERWIDGET_H
#define AMPLIFIERWIDGET_H

#include <QWidget>
#include <QEventLoop>

namespace Ui {
class AmplifierWidget;
}

class AmplifierWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AmplifierWidget(QWidget *parent = 0);
    ~AmplifierWidget();

    QString static showMyMessageBox(QWidget* parent);

public:
    void paintEvent(QPaintEvent *);
    void closeEvent(QCloseEvent *event);

private slots:
    void on_cancelBtn_clicked();
    void on_okBtn_clicked();

private:
    Ui::AmplifierWidget *ui;
    QEventLoop* m_eventLoop;
    QString exec();
    QString m_resultStr;
    void initStyle();
};

#endif // AMPLIFIERWIDGET_H
