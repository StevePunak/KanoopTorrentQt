#ifndef TORRENTSEARCHRESULT_H
#define TORRENTSEARCHRESULT_H

#include <Kanoop/torrent/kanooptorrent.h>
#include <Kanoop/torrent/magnetlink.h>
#include <QDateTime>
#include <QJsonObject>
#include <QString>

class LIBKANOOPTORRENT_EXPORT TorrentSearchResult
{
public:
    TorrentSearchResult();

    QString name() const { return _name; }
    QString infoHash() const { return _infoHash; }
    qint64 size() const { return _size; }
    int seeders() const { return _seeders; }
    int leechers() const { return _leechers; }
    QDateTime addedDate() const { return _addedDate; }
    QString category() const { return _category; }
    QString uploaderName() const { return _uploaderName; }

    MagnetLink toMagnetLink() const;

    static TorrentSearchResult fromJson(const QJsonObject& json);
    static QString formatSize(qint64 bytes);

private:
    QString _name;
    QString _infoHash;
    qint64 _size = 0;
    int _seeders = 0;
    int _leechers = 0;
    QDateTime _addedDate;
    QString _category;
    QString _uploaderName;
};

#endif // TORRENTSEARCHRESULT_H
