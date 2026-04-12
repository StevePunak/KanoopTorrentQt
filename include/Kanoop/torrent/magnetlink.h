#ifndef MAGNETLINK_H
#define MAGNETLINK_H

#include <Kanoop/torrent/kanooptorrent.h>
#include <QByteArray>
#include <QString>
#include <QStringList>

class LIBKANOOPTORRENT_EXPORT MagnetLink
{
public:
    MagnetLink();
    explicit MagnetLink(const QString& uri);

    bool isValid() const { return _valid; }

    QByteArray infoHash() const { return _infoHash; }
    QString infoHashHex() const { return QString::fromLatin1(_infoHash.toHex()); }

    QString displayName() const { return _displayName; }
    void setDisplayName(const QString& value) { _displayName = value; }

    QStringList trackers() const { return _trackers; }
    void addTracker(const QString& url) { _trackers.append(url); }

    qint64 exactLength() const { return _exactLength; }

    QString toUri() const;

    static MagnetLink fromInfoHash(const QString& hexHash);

private:
    static QByteArray decodeBase32(const QByteArray& encoded);

    bool _valid = false;
    QByteArray _infoHash;
    QString _displayName;
    QStringList _trackers;
    qint64 _exactLength = 0;
};

#endif // MAGNETLINK_H
