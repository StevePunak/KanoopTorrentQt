#include <Kanoop/torrent/magnetlink.h>
#include <QUrl>
#include <QUrlQuery>

static const QStringList WellKnownTrackers = {
    // HTTP trackers (work through SOCKS5 proxy)
    "https://tracker.lilithraws.org:443/announce",
    "http://tracker.files.fm:6969/announce",
    "http://tracker.bt4g.com:2095/announce",
    "https://tracker.tamersunion.org:443/announce",
    "http://open.acgnxtracker.com:80/announce",
    // UDP trackers (direct only, skipped when proxy active)
    "udp://tracker.opentrackr.org:1337/announce",
    "udp://open.stealth.si:80/announce",
    "udp://tracker.openbittorrent.com:6969/announce",
    "udp://exodus.desync.com:6969/announce",
    "udp://tracker.torrent.eu.org:451/announce",
};

MagnetLink::MagnetLink()
{
}

MagnetLink::MagnetLink(const QString& uri)
{
    // magnet:?xt=urn:btih:<hash>&dn=<name>&tr=<tracker>...
    QUrl url(uri);
    if(url.scheme() != "magnet") {
        return;
    }

    QUrlQuery query(url.query());
    QList<QPair<QString, QString>> items = query.queryItems(QUrl::FullyDecoded);

    for(const QPair<QString, QString>& pair : items) {
        if(pair.first == "xt") {
            // xt=urn:btih:<hash>
            QString xt = pair.second;
            if(!xt.startsWith("urn:btih:", Qt::CaseInsensitive)) {
                continue;
            }
            QString hashStr = xt.mid(9); // skip "urn:btih:"

            if(hashStr.length() == 40) {
                // 40-char hex SHA1
                _infoHash = QByteArray::fromHex(hashStr.toLatin1());
            }
            else if(hashStr.length() == 32) {
                // 32-char base32 SHA1
                _infoHash = decodeBase32(hashStr.toUpper().toLatin1());
            }

            if(_infoHash.size() == 20) {
                _valid = true;
            }
        }
        else if(pair.first == "dn") {
            _displayName = pair.second;
        }
        else if(pair.first == "tr") {
            _trackers.append(pair.second);
        }
        else if(pair.first == "xl") {
            bool ok = false;
            qint64 len = pair.second.toLongLong(&ok);
            if(ok) {
                _exactLength = len;
            }
        }
    }
}

QString MagnetLink::toUri() const
{
    if(!_valid) {
        return QString();
    }

    QString uri = "magnet:?xt=urn:btih:" + infoHashHex();

    if(!_displayName.isEmpty()) {
        uri += "&dn=" + QUrl::toPercentEncoding(_displayName);
    }

    for(const QString& tracker : _trackers) {
        uri += "&tr=" + QUrl::toPercentEncoding(tracker);
    }

    if(_exactLength > 0) {
        uri += "&xl=" + QString::number(_exactLength);
    }

    return uri;
}

MagnetLink MagnetLink::fromInfoHash(const QString& hexHash)
{
    MagnetLink link;
    if(hexHash.length() != 40) {
        return link;
    }

    link._infoHash = QByteArray::fromHex(hexHash.toLatin1());
    if(link._infoHash.size() == 20) {
        link._valid = true;
        // Add well-known trackers so magnet links work without embedded trackers
        link._trackers = WellKnownTrackers;
    }
    return link;
}

QByteArray MagnetLink::decodeBase32(const QByteArray& encoded)
{
    // RFC 4648 base32 decode
    static const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

    QByteArray result;
    int buffer = 0;
    int bitsLeft = 0;

    for(int i = 0; i < encoded.size(); ++i) {
        char c = encoded.at(i);
        if(c == '=') {
            break; // padding
        }

        const char* found = strchr(alphabet, c);
        if(!found) {
            return QByteArray(); // invalid character
        }

        int value = static_cast<int>(found - alphabet);
        buffer = (buffer << 5) | value;
        bitsLeft += 5;

        if(bitsLeft >= 8) {
            bitsLeft -= 8;
            result.append(static_cast<char>((buffer >> bitsLeft) & 0xFF));
        }
    }

    return result;
}
