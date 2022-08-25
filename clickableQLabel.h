#ifndef CLICKABLEQLABEL_H
#define CLICKABLEQLABEL_H

#include <QLabel>
#include <QWidget>
#include <Qt>

class ClickableQLabel : public QLabel {
    Q_OBJECT

public:
    explicit ClickableQLabel(QWidget* parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
    explicit ClickableQLabel(const QString &text, QWidget *parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
    ~ClickableQLabel();

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent*);

};

#endif // CLICKABLEQLABEL_H
