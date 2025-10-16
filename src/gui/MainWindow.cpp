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

#include "app/global.h"
#include "app/Receiver.h"

MainWindow::MainWindow(QWidget *p) : QMainWindow(p), rcv(new Receiver()){
	setupUi(this);
    setWindowTitle(QApplication::applicationName());
    setFixedSize(450, 160);
    setAttribute(Qt::WA_DeleteOnClose, true);
    restoreGeometry(mApp->value("geom").toByteArray());
    lcdNumber->display("");
    led->setLedColor(Qt::darkGray);
    chkAgc->setChecked(rcv->agc());
    chkStereo->setChecked(rcv->getStereo());

}

MainWindow::~MainWindow() {

}

void MainWindow::init(){
    connect(rcv.get(), &Receiver::newRadio, [this](const QStringList& list) {
        cmbRadio->addItems(list);

    });
    rcv->init();

    btnPower->setIcon(style()->standardIcon(QStyle::SP_MediaPlay, 0, this));
    connect(btnPower, &QPushButton::clicked, [this] {
        if(btnPower->toolTip() == "On"){
            btnPower->setToolTip("Off");
            btnPower->setIcon(style()->standardIcon(QStyle::SP_MediaStop, 0, this));
            rcv.reset(new Receiver());
            rcv->device(cmbRadio->currentIndex());
            rcv->agc(chkAgc->isChecked());
            rcv->setStereo(chkStereo->isChecked());
            QThread* th = new QThread();
            rcv.get()->moveToThread(th);
            connect( th, &QThread::started, rcv.get(), &Receiver::start);
            connect( rcv.get(), &Receiver::finished, th, &QThread::quit);
            connect( rcv.get(), &Receiver::finished, rcv.get(), &Receiver::deleteLater);
            connect( th, &QThread::finished, th, &QThread::deleteLater);
            th->start();
            lcdNumber->setRcv(rcv.get());
            connect(rcv.get(), &Receiver::newStereo, [this](bool b) {
                b?led->setLedColor(Qt::green):led->setLedColor(Qt::gray);
            });
            chkAgc->setEnabled(false);
            chkStereo->setEnabled(false);
        }else{
            btnPower->setToolTip("On");
            btnPower->setIcon(style()->standardIcon(QStyle::SP_MediaPlay, 0, this));
            rcv->stop();
            lcdNumber->display("");
            chkAgc->setEnabled(true);
            chkStereo->setEnabled(true);
            led->setLedColor(Qt::darkGray);
        }
    });

    btnAbout->setIcon(style()->standardIcon(QStyle::SP_MessageBoxInformation, 0, this));
    connect(btnAbout, &QPushButton::clicked, [this] {
        showAbout();
    });
    connect(btnPlus, &QPushButton::clicked, [this] {
        rcv->setFreq(rcv->getFreq() + 1);
    });
    connect(btnMinus, &QPushButton::clicked, [this] {
        rcv->setFreq(rcv->getFreq() - 1);
    });
    connect(btnUp, &QPushButton::clicked, [this] {
        rcv->setFreq(rcv->getFreq() + 25);
    });
    connect(btnDown, &QPushButton::clicked, [this] {
        rcv->setFreq(rcv->getFreq() - 25);
    });
}

void MainWindow::closeEvent(QCloseEvent *event) {
    mApp->setValue("geom", saveGeometry());
	event->accept();
}

void MainWindow::showAbout() {
	QString str;
	str.append(qApp->applicationName());
	str.append(" ").append(qApp->applicationVersion()).append("<br>");
    str.append("&copy; 2025 Alexander Busorgin <br/>");
    str.append("License: GPL-3 (GPL-3.0-only)<br/>");
    str.append("<a href='https://github.com/dualword/binaural-sdr'>https://github.com/dualword/binaural-sdr</a><br/>");
	QMessageBox::about(this, tr("About"), str );
}

bool MainWindow::askYesNo(QWidget* p, const QString& str){
	bool yes = false;
	QMessageBox::StandardButton r;
	r = QMessageBox::question(p, tr(""),str, QMessageBox::Yes | QMessageBox::No);
	if (r == QMessageBox::Yes) yes = true;
	return yes;
}

void MainWindow::resizeEvent(QResizeEvent *event){

}
