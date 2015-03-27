/*
    Copyright (C) 2015 by Project Tox <https://tox.im>

    This file is part of qTox, a Qt-based graphical interface for Tox.

    This program is libre software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    See the COPYING file for more details.
*/

/* A C++11/Qt5 thin wrapper to the Tox toxav public API in toxav.h.
 * This is a *thin* wrapper, anyone using this is expected to have also
 * read toxcore's header, linked at the bottom of this comment.
 *
 * Having said that, these are some things to know about this wrapper:
 * Since the errors exposed by the core API are largely internal errors,
 * best used for debugging, or external errors that are preventable in clients
 * such that their presence is a bug in the client, it is a design choice that
 * the errors are ignored other than being logged to file, and instead the
 * caller will get an invalid return value (e.g. UINT32_MAX, QString::isNull())
 * In some cases the return value is an enum without an INVALID or similar
 * member of the enum; in such cases the return type is QVariant, which will be
 * QVariant::isNull() on error, or if not, then use QVariant::toInt() and cast
 * to the enum type.
 * Some changes have been made for consistency with Qt conventions. The changes
 * themselves should also be consistent across the API.
 * Note: in some cases, an empty string is valid, so be sure to use isNull()
 * and _not_ isEmpty().
 *
 * Functions are named the same, except in camelCase
 * self_get_nospam becomes getSelfNoSpam, for consistency with Qt conventions
 * Error enums are ignored except for logging purposes
 *   - Return values should instead be tested for success
 * uint8_t arrays for data are converted to/from QByteArrays, and
 * uint8_t arrays for text are converted to/from QStrings
 * Functions that fill an array whose size is provided by another function are wrapped into one,
 *     using the QByteArray/QString objects to handle sizing automatically
 *
 * Otherwise, the documentation from toxav.h applies for all functions here
 * https://github.com/irungentoo/toxcore/blob/master/toxav/toxav.h */

#ifndef QTOXAV_H
#define QTOXAV_H

#include <tox/toxav.h>

class ToxAv : public QObject
{
    Q_OBJECT

public:
    ToxAv::ToxAv(ToxCore& tox);
    ToxAv::ToxAv();

    uint32_t iterationInterval() const;
    void iterate();

    bool callControl(uint32_t friend_number, TOXAV_CALL_CONTROL control);
    bool setAudioBitRate(uint32_t friend_number, uint32_t audio_bit_rate);
    bool setVideoBitRate(uint32_t friend_number, uint32_t video_bit_rate);
    bool sendVideoFrame(uint32_t friend_number,
                        uint16_t width, uint16_t height,
                        const uint8_t* y, const uint8_t* u, const uint8_t* v);
    bool sendAudioFrame(uint32_t friend_number, const int16_t* pcm, size_t sample count,
                                                uint8_t channels, uint32_t sampling rate);

public slots:
    bool call(uint32_t friend_number, uint32_t audio_bit_rate, uint32_t video_bit_rate);
    bool answer(uint32_t friend_number, uint32_t audio_bit_rate, uint32_t video_bit_rate);

signals:
    void callRecevied(uint32_t friend_number, bool audio_enabled, bool video_enabled);
    void callStateChanged(uint32_t friend_number, uint32_t state);
    void videoFrameRequest(uint32_t friend_number);
    void audioFrameRequest(uint32_t friend_number);
    void videoFrameReceived(uint32_t friend_number, uint16_t width, uint16_t height, const uint8_t* planes[], const int32_t stride[]);

private:
    ToxAV* toxav;
};

#endif // QTOXAV_H
