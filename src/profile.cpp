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

#include "profile.h"
#include "toxcore.h"

Profile::Profile(const QString& path)
{

}

void Profile::createTox()
{
    // IPv6 needed for LAN discovery, but can crash some weird routers. On by default, can be disabled in options.
    bool enableIPv6 = Settings::getInstance().getEnableIPv6();
    bool forceTCP = Settings::getInstance().getForceTCP();
    ProxyType proxyType = Settings::getInstance().getProxyType();

    if (enableIPv6)
        qDebug() << "Core starting with IPv6 enabled";
    else
        qWarning() << "Core starting with IPv6 disabled. LAN discovery may not work properly.";

    ToxOptions toxOptions;
    toxOptions.tox_options->ipv6enabled = enableIPv6;
    toxOptions.tox_options->udp_disabled = forceTCP;

    // No proxy by default
    toxOptions.tox_options->proxy_type = TOX_PROXY_NONE;
    toxOptions.tox_options->proxy_host[0] = 0;
    toxOptions.tox_options->proxy_port = 0;

    if (proxyType != ProxyType::ptNone)
    {
        QString proxyAddr = Settings::getInstance().getProxyAddr();
        int proxyPort = Settings::getInstance().getProxyPort();

        if (proxyAddr.length() > 255)
        {
            qWarning() << "Core: proxy address" << proxyAddr << "is too long";
        }
        else if (proxyAddr != "" && proxyPort > 0)
        {
            qDebug() << "Core: using proxy" << proxyAddr << ":" << proxyPort;
            // protection against changings in TOX_PROXY_TYPE enum
            if (proxyType == ProxyType::ptSOCKS5)
                toxOptions.tox_options->proxy_type = TOX_PROXY_SOCKS5;
            else if (proxyType == ProxyType::ptHTTP)
                toxOptions.tox_options->proxy_type = TOX_PROXY_HTTP;
            uint16_t sz = CString::fromString(proxyAddr, (unsigned char*)toxOptions.tox_options->proxy_host);
            toxOptions.tox_options->proxy_host[sz] = 0;
            toxOptions.tox_options->proxy_port = proxyPort;
        }
    }

    tox = new ToxCore(toxOptions);
    if (tox == nullptr)
    {
        if (enableIPv6) // Fallback to IPv4
        {
            toxOptions.tox_options->ipv6enabled = false;
            tox = tox_new(&toxOptions);
            if (tox == nullptr)
            {
                if (toxOptions.tox_options->proxy_type != TOX_PROXY_NONE)
                {
                    qCritical() << "Core: bad proxy! no toxcore!";
                    emit badProxy();
                }
                else
                {
                    qCritical() << "Tox core failed to start";
                    emit failedToStart();
                }
                return;
            }
            else
                qWarning() << "Core failed to start with IPv6, falling back to IPv4. LAN discovery may not work properly.";
        }
        else if (toxOptions.tox_options->proxy_type != TOX_PROXY_NONE)
        {
            emit badProxy();
            return;
        }
        else
        {
            qCritical() << "Tox core failed to start";
            emit failedToStart();
            return;
        }
    }

    toxav = toxav_new(tox, TOXAV_MAX_CALLS);
    if (toxav == nullptr)
    {
        qCritical() << "Toxav core failed to start";
        emit failedToStart();
        return;
    }
}
