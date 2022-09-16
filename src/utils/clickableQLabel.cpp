#include "clickableQLabel.h"

ClickableQLabel::ClickableQLabel(QWidget* parent, Qt::WindowFlags f)
    : QLabel(parent, f) {
}

ClickableQLabel::ClickableQLabel(const QString &text, QWidget *parent, Qt::WindowFlags f)
    : QLabel(text, parent, f) {
}

ClickableQLabel::~ClickableQLabel() {}

void ClickableQLabel::mousePressEvent(QMouseEvent*) {
    emit clicked();
}
