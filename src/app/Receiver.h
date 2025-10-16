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

#ifndef RECEIVER_H
#define RECEIVER_H

#include <cstdlib>
#include <cstdio>
#include <climits>
#include <cmath>
#include <csignal>
#include <cstring>
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unistd.h>
#include <getopt.h>
#include <sys/time.h>

#include "SoftFM.h"
#include "RtlSdrSource.h"
#include "FmDecode.h"
#include "AudioOutput.h"

#include <QtCore>

using namespace std;

/** Buffer to move sample data between threads. */
template <class Element>
class DataBuffer
{
public:
    /** Constructor. */
    DataBuffer()
        : m_qlen(0)
        , m_end_marked(false)
    { }

    /** Add samples to the queue. */
    void push(vector<Element>&& samples)
    {
        if (!samples.empty()) {
            unique_lock<mutex> lock(m_mutex);
            m_qlen += samples.size();
            m_queue.push(move(samples));
            lock.unlock();
            m_cond.notify_all();
        }
    }

    /** Mark the end of the data stream. */
    void push_end()
    {
        unique_lock<mutex> lock(m_mutex);
        m_end_marked = true;
        lock.unlock();
        m_cond.notify_all();
    }

    /** Return number of samples in queue. */
    size_t queued_samples()
    {
        unique_lock<mutex> lock(m_mutex);
        return m_qlen;
    }

    /**
     * If the queue is non-empty, remove a block from the queue and
     * return the samples. If the end marker has been reached, return
     * an empty vector. If the queue is empty, wait until more data is pushed
     * or until the end marker is pushed.
     */
    vector<Element> pull()
    {
        vector<Element> ret;
        unique_lock<mutex> lock(m_mutex);
        while (m_queue.empty() && !m_end_marked)
            m_cond.wait(lock);
        if (!m_queue.empty()) {
            m_qlen -= m_queue.front().size();
            swap(ret, m_queue.front());
            m_queue.pop();
        }
        return ret;
    }

    /** Return true if the end has been reached at the Pull side. */
    bool pull_end_reached()
    {
        unique_lock<mutex> lock(m_mutex);
        return m_qlen == 0 && m_end_marked;
    }

    /** Wait until the buffer contains minfill samples or an end marker. */
    void wait_buffer_fill(size_t minfill)
    {
        unique_lock<mutex> lock(m_mutex);
        while (m_qlen < minfill && !m_end_marked)
            m_cond.wait(lock);
    }

private:
    size_t              m_qlen;
    bool                m_end_marked;
    queue<vector<Element>> m_queue;
    mutex               m_mutex;
    condition_variable  m_cond;
};

class Receiver: public QObject {
    Q_OBJECT

public:
    Receiver(QObject* p = 0);
    ~Receiver();

signals:
    void finished();
    void newFreq(int);
    void newStereo(bool);
    void newRadio(const QStringList&);
    void newLna(const QStringList&);

public slots:
    void init();
    void start();
    void stop();
    void device(int i){devidx = i;};
    void agc(bool b){agcmode = b;};
    void setStereo(bool b){stereo = b;};
    bool agc(){return agcmode;};
    bool getStereo(){ return stereo;};
    int getFreq(){ if(!rtlsdr) return 0; return rtlsdr->get_frequency(); };
    void setFreq(int d){
        if(!rtlsdr) return;
        rtlsdr->set_frequency(d);
        int f = rtlsdr->get_frequency();
        tuner_freq = f ;
        emit newFreq(f);
    };

private:
    double tuner_freq;
    double  freq    = -1;
    int     devidx  = -1;
    int     lnagain = INT_MIN;
    bool    agcmode = false;
    double  ifrate  = 1.0e6;
    int     pcmrate = 44100;
    bool    stereo  = true;
    enum OutputMode {MODE_ALSA };
    OutputMode outmode = MODE_ALSA;
    string  filename;
    string  alsadev = "default";
    string  ppsfilename;
    double  bufsecs = -1;
    unique_ptr<RtlSdrSource> rtlsdr;

};

#endif // RECEIVER_H
