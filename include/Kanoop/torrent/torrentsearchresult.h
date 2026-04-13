#ifndef TORRENTSEARCHRESULT_H
#define TORRENTSEARCHRESULT_H

#include <Kanoop/torrent/kanooptorrent.h>
#include <Kanoop/torrent/magnetlink.h>
#include <QDateTime>
#include <QJsonObject>
#include <QString>

/**
 * @brief Metadata for a single torrent search result.
 *
 * Populated by TorrentSearcher from API responses. Use toMagnetLink()
 * to convert a result into a MagnetLink suitable for TorrentClient::addTorrent().
 */
class LIBKANOOPTORRENT_EXPORT TorrentSearchResult
{
public:
    TorrentSearchResult();

    /** @brief Torrent display name. */
    QString name() const { return _name; }
    /** @brief Hex-encoded info hash string. */
    QString infoHash() const { return _infoHash; }
    /** @brief Total content size in bytes. */
    qint64 size() const { return _size; }
    /** @brief Number of seeders reported by the tracker. */
    int seeders() const { return _seeders; }
    /** @brief Number of leechers reported by the tracker. */
    int leechers() const { return _leechers; }
    /** @brief Date/time the torrent was added to the index. */
    QDateTime addedDate() const { return _addedDate; }
    /** @brief Category string (e.g. "Audio", "Video"). */
    QString category() const { return _category; }
    /** @brief Name of the uploader. */
    QString uploaderName() const { return _uploaderName; }

    /**
     * @brief Convert this result to a MagnetLink.
     * @return A MagnetLink with the info hash and display name set.
     */
    MagnetLink toMagnetLink() const;

    /**
     * @brief Parse a search result from a JSON object.
     *
     * Expected fields: @c name, @c info_hash, @c size, @c seeders,
     * @c leechers, @c added, @c category, @c username.
     * @param json The JSON object to parse.
     * @return A populated TorrentSearchResult.
     */
    static TorrentSearchResult fromJson(const QJsonObject& json);

    /**
     * @brief Format a byte count as a human-readable string.
     * @param bytes Raw byte count.
     * @return Formatted string (e.g. "1.23 GB").
     */
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
