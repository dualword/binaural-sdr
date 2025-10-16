/*
 * Copyright (C) 2025 Alexander Busorgin
 * This file is part of Binaural-SDR (https://github.com/dualword/binaural-sdr)
 * License: GPL-3 (GPL-3.0-only)
 *
 * Binaural-SDR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Binaural-SDR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Binaural-SDR.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef LEDBUTTON_H
#define LEDBUTTON_H

#include <QObject>
#include <QPainter>
#include <QToolButton>

class LEDButton : public QToolButton {
    Q_OBJECT

public:
    LEDButton(QWidget *p = nullptr) : QToolButton(p){};
    ~LEDButton(){};
    void setLedColor(QColor c) { mColor = c; update();}

protected:
    void paintEvent(QPaintEvent *event){
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        QPointF c(width()/2, height()/2);
        int w = width() * 0.9;
        int h = height() * 0.9;
        int m = (height() * 0.1) - 1;
        qreal r = w * 3/5;

        QRadialGradient gradient( c, r);
        gradient.setColorAt(0.0, Qt::white);
        gradient.setColorAt(0.21, mColor);
        gradient.setColorAt(1.0, Qt::black);
        painter.setBrush(gradient);
        painter.setPen( Qt::NoPen );
        painter.drawEllipse( m, m, h, h);

    }

private:
    QColor mColor = Qt::green;
};

#endif // LEDBUTTON_H
