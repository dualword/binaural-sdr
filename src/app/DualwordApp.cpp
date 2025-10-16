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

#include "DualwordApp.h"

DualwordApp::DualwordApp(int &argc, char **argv) : QApplication(argc, argv) {
    setApplicationName("Binaural-SDR");
    setOrganizationName("dualword");
    #ifdef _VER
        QApplication::setApplicationVersion(_VER);
    #endif

}

DualwordApp::~DualwordApp() {

}

void DualwordApp::start() {
    w = new MainWindow();
    w->init();
    w->show();
}

void DualwordApp::setValue(const QString &key, const QVariant& val){
    QSettings s;
    s.setValue(key, val);
}

QVariant DualwordApp::value(const QString &key, const QVariant& val){
    QSettings s;
    return s.value(key, val);
}

