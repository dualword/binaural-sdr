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

#ifndef GUI_MAINWINDOW_H_
#define GUI_MAINWINDOW_H_

#include <QtWidgets>

#include "app/Receiver.h"

class LCDNumber : public QLCDNumber {
     Q_OBJECT

 public:
    LCDNumber(QWidget *p = nullptr) : QLCDNumber(p) {};

public slots:
    void setRcv(Receiver* rcv){
        mRcv = rcv;
        connect(mRcv, SIGNAL(newFreq(int)), SLOT(newFreq(int)));
        display(mRcv->getFreq());
    }
    void newFreq(int f){display(f);}

 protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            if(mRcv){
                bool ok;
                 double d = QInputDialog::getInt(this, tr("Enter new frequency"),
                                                    tr("Frequency (Hz):"), mRcv->getFreq(), 0, 1999999999, 3, &ok,
                                                    Qt::WindowFlags());
                     if (ok) {
                         mRcv->setFreq(d);
                         display(mRcv->getFreq());

                    }

            }
                 event->accept();
        }
    };

private:
    Receiver* mRcv = nullptr;

 };
#include "ui_MainWindow.h"

class MainWindow : public QMainWindow, private Ui::MainWindow {
  Q_OBJECT

public:
	MainWindow(QWidget *p = nullptr);
	virtual ~MainWindow();
	void init();
	bool askYesNo(QWidget*, const QString&);
    
protected:
    void closeEvent(QCloseEvent *);
    void resizeEvent(QResizeEvent *);

public slots:


private slots:
	void showAbout();

private:
    QScopedPointer<Receiver> rcv;

};

#endif /* GUI_MAINWINDOW_H_ */
