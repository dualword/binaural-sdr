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
/*
 *  SoftFM - Software decoder for FM broadcast radio with RTL-SDR
 *
 *  Copyright (C) 2013, Joris van Rantwijk.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, see http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "Receiver.h"
#include "app/global.h"

/** Flag is set on SIGINT / SIGTERM. */
static atomic_bool stop_flag(false);

/** Simple linear gain adjustment. */
void adjust_gain(SampleVector& samples, double gain)
{
    for (unsigned int i = 0, n = samples.size(); i < n; i++) {
        samples[i] *= gain;
    }
}

/**
 * Read data from source device and put it in a buffer.
 *
 * This code runs in a separate thread.
 * The RTL-SDR library is not capable of buffering large amounts of data.
 * Running this in a background thread ensures that the time between calls
 * to RtlSdrSource::get_samples() is very short.
 */
void read_source_data(RtlSdrSource *rtlsdr, DataBuffer<IQSample> *buf)
{
    IQSampleVector iqsamples;
    while (!stop_flag.load()) {        
        if (!rtlsdr->get_samples(iqsamples)) {
            fprintf(stderr, "ERROR: RtlSdr: %s\n", rtlsdr->error().c_str());
            exit(1);
        }
        buf->push(move(iqsamples));
    }
    buf->push_end();
}

/**
 * Get data from output buffer and write to output stream.
 *
 * This code runs in a separate thread.
 */
void write_output_data(AudioOutput *output, DataBuffer<Sample> *buf,
                       unsigned int buf_minfill)
{
    while (!stop_flag.load()) {

        if (buf->queued_samples() == 0) {
            // The buffer is empty. Perhaps the output stream is consuming
            // samples faster than we can produce them. Wait until the buffer
            // is back at its nominal level to make sure this does not happen
            // too often.
            buf->wait_buffer_fill(buf_minfill);
        }

        if (buf->pull_end_reached()) {
            // Reached end of stream.
            break;
        }

        // Get samples from buffer and write to output.
        SampleVector samples = buf->pull();
        //fprintf(stderr, "\n SampleVector: %u \n", samples.size());
        output->write(samples);
        if (!(*output)) {
            fprintf(stderr, "ERROR: AudioOutput: %s\n", output->error().c_str());
        }
    }
}

/** Return Unix time stamp in seconds. */
double get_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + 1.0e-6 * tv.tv_usec;
}

Receiver::Receiver(QObject* p) : QObject(p) {
    stop_flag.store(false);
    freq = mApp->value("freq", 10000000).toDouble();
    agcmode = mApp->value("agc", true).toBool();
    stereo = mApp->value("stereo", true).toBool();
}

Receiver::~Receiver(){
    mApp->setValue("freq", (int)tuner_freq);
    mApp->setValue("agc", agcmode);
    mApp->setValue("stereo", stereo);
}

void Receiver::init(){
    vector<string> devnames = RtlSdrSource::get_device_names();
    QStringList list;
    for (const auto& s : devnames)
        list.append(s.c_str());
    emit newRadio(list);
}

void Receiver::start() {
    if(freq <= 0) freq = 10000000.0;

    vector<string> devnames = RtlSdrSource::get_device_names();
    if (devidx < 0 || (unsigned int)devidx >= devnames.size()) {
        return;
    }

    // Intentionally tune at a higher frequency to avoid DC offset.
    tuner_freq = freq; // + 0.25 * ifrate;
    rtlsdr.reset(new RtlSdrSource(devidx));

    // Configure RTL-SDR device and start streaming.
    rtlsdr->configure(ifrate, tuner_freq, lnagain, RtlSdrSource::default_block_length, agcmode);
    if (!rtlsdr) {
        fprintf(stderr, "ERROR: RtlSdr: %s\n", rtlsdr->error().c_str());
        return;
    }
    int f = rtlsdr->get_frequency();
    tuner_freq = f;
    emit newFreq(f);

    ifrate = rtlsdr->get_sample_rate();

    // Create source data queue.
    DataBuffer<IQSample> source_buffer;

    // Start reading from device in separate thread.
    std::thread source_thread(read_source_data, rtlsdr.get(), &source_buffer);

    // The baseband signal is empty above 100 kHz, so we can
    // downsample to ~ 200 kS/s without loss of information.
    // This will speed up later processing stages.
    unsigned int downsample = max(1, int(ifrate / 215.0e3));

    // Prevent aliasing at very low output sample rates.
    double bandwidth_pcm = min(FmDecoder::default_bandwidth_pcm, 0.45 * pcmrate);

    // Prepare decoder.
    FmDecoder fm(ifrate,                            // sample_rate_if
                 freq - tuner_freq,                 // tuning_offset
                 pcmrate,                           // sample_rate_pcm
                 stereo,                            // stereo
                 FmDecoder::default_deemphasis,     // deemphasis,
                 FmDecoder::default_bandwidth_if,   // bandwidth_if
                 FmDecoder::default_freq_dev,       // freq_dev
                 bandwidth_pcm,                     // bandwidth_pcm
                 downsample);                       // downsample

    // Calculate number of samples in audio buffer.
    unsigned int outputbuf_samples = 0;
    if (bufsecs < 0 && (outmode == MODE_ALSA)) {
        // Set default buffer to 1 second for interactive output streams.
        outputbuf_samples = pcmrate;
    } else if (bufsecs > 0) {
        // Calculate nr of samples for configured buffer length.
        outputbuf_samples = (unsigned int)(bufsecs * pcmrate);
    }
    if (outputbuf_samples > 0) {
        //fprintf(stderr, "output buffer:     %.1f seconds\n", outputbuf_samples / double(pcmrate));
    }

    // Prepare output writer.
    unique_ptr<AudioOutput> audio_output;
    switch (outmode) {
        case MODE_ALSA:
            audio_output.reset(new AlsaAudioOutput(alsadev, pcmrate, stereo));
            break;
    }

    if (!(*audio_output)) {
        fprintf(stderr, "ERROR: AudioOutput: %s\n", audio_output->error().c_str());
        return;
    }

    // If buffering enabled, start background output thread.
    DataBuffer<Sample> output_buffer;
    std::thread output_thread;
    if (outputbuf_samples > 0) {
        unsigned int nchannel = stereo ? 2 : 1;
        output_thread = std::thread(write_output_data,
                               audio_output.get(),
                               &output_buffer,
                               outputbuf_samples * nchannel);
    }

    SampleVector audiosamples;
    bool inbuf_length_warning = false;
    double audio_level = 0;
    bool got_stereo = false;
    double block_time = get_time();

    // Main loop.
    for (unsigned int block = 0; !stop_flag.load(); block++) {
        // Check for overflow of source buffer.
        if (!inbuf_length_warning &&
            source_buffer.queued_samples() > 10 * ifrate) {
            inbuf_length_warning = true;
        }

        // Pull next block from source buffer.
        IQSampleVector iqsamples = source_buffer.pull();
        if (iqsamples.empty())
            break;

        double prev_block_time = block_time;
        block_time = get_time();

        // Decode FM signal.
        fm.process(iqsamples, audiosamples);

        // Measure audio level.
        double audio_mean, audio_rms;
        samples_mean_rms(audiosamples, audio_mean, audio_rms);
        audio_level = 0.95 * audio_level + 0.05 * audio_rms;

        // Set nominal audio volume.
        adjust_gain(audiosamples, 0.5);

        // Show statistics.
//        fprintf(stderr,
//                "\rblk=%6d  freq=%8.4fMHz  IF=%+5.1fdB  BB=%+5.1fdB  audio=%+5.1fdB ",
//                block,
//                (tuner_freq + fm.get_tuning_offset()) * 1.0e-6,
//                20*log10(fm.get_if_level()),
//                20*log10(fm.get_baseband_level()) + 3.01,
//                20*log10(audio_level) + 3.01);
        if (outputbuf_samples > 0) {
            unsigned int nchannel = stereo ? 2 : 1;
            size_t buflen = output_buffer.queued_samples();
            //fprintf(stderr, " buf=%.1fs ", buflen / nchannel / double(pcmrate));
        }
        fflush(stderr);

        // Show stereo status.
        if (fm.stereo_detected() != got_stereo) {
            got_stereo = fm.stereo_detected();
            if (got_stereo){
                emit newStereo(true);
            }else{
                emit newStereo(false);
            }
        }

        // Throw away first block. It is noisy because IF filters
        // are still starting up.
        if (block > 0) {

            // Write samples to output.
            if (outputbuf_samples > 0) {
                // Buffered write.
                output_buffer.push(move(audiosamples));
            } else {
                // Direct write.
                audio_output->write(audiosamples);
            }
        }
    }
    //fprintf(stderr, "\n");

    source_thread.join();
    if (outputbuf_samples > 0) {
        output_buffer.push_end();
        output_thread.join();
    }
}

void Receiver::stop(){
    stop_flag.store(true);
    mApp->setValue("freq", (int)tuner_freq);
}
