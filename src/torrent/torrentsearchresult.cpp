#include <Kanoop/torrent/torrentsearchresult.h>

TorrentSearchResult::TorrentSearchResult()
{
}

MagnetLink TorrentSearchResult::toMagnetLink() const
{
    MagnetLink link = MagnetLink::fromInfoHash(_infoHash);
    if(link.isValid() && !_name.isEmpty()) {
        link.setDisplayName(_name);
    }
    return link;
}

TorrentSearchResult TorrentSearchResult::fromJson(const QJsonObject& json)
{
    TorrentSearchResult result;
    result._name = json.value("name").toString();
    result._infoHash = json.value("info_hash").toString();
    result._size = json.value("size").toString().toLongLong();
    result._seeders = json.value("seeders").toString().toInt();
    result._leechers = json.value("leechers").toString().toInt();
    result._category = json.value("category").toString();
    result._uploaderName = json.value("username").toString();

    qint64 addedTimestamp = json.value("added").toString().toLongLong();
    if(addedTimestamp > 0) {
        result._addedDate = QDateTime::fromSecsSinceEpoch(addedTimestamp);
    }

    return result;
}

QString TorrentSearchResult::formatSize(qint64 bytes)
{
    if(bytes < 0) {
        return QString("0 B");
    }

    static const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    int unitIndex = 0;
    double size = static_cast<double>(bytes);

    while(size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }

    if(unitIndex == 0) {
        return QString("%1 B").arg(bytes);
    }
    return QString("%1 %2").arg(size, 0, 'f', 1).arg(units[unitIndex]);
}
