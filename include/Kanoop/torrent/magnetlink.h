#ifndef MAGNETLINK_H
#define MAGNETLINK_H

#include <Kanoop/torrent/kanooptorrent.h>
#include <QByteArray>
#include <QString>
#include <QStringList>

/**
 * @brief Parses and constructs BitTorrent magnet URIs.
 *
 * MagnetLink handles both hex (40-char) and Base32-encoded info hashes,
 * extracts tracker URLs and display names, and can reconstruct a
 * well-formed magnet URI string.
 *
 * @code
 *   MagnetLink link("magnet:?xt=urn:btih:...");
 *   if (link.isValid()) {
 *       qDebug() << link.displayName() << link.infoHashHex();
 *   }
 * @endcode
 */
class LIBKANOOPTORRENT_EXPORT MagnetLink
{
public:
    /** @brief Construct an invalid (empty) magnet link. */
    MagnetLink();

    /**
     * @brief Parse a magnet URI string.
     *
     * Supports the @c xt (exact topic / info hash), @c dn (display name),
     * @c tr (tracker), and @c xl (exact length) parameters.
     * @param uri A @c "magnet:?..." URI string.
     */
    explicit MagnetLink(const QString& uri);

    /** @brief Whether the URI was parsed successfully and contains a valid info hash. */
    bool isValid() const { return _valid; }

    /** @brief Raw 20-byte SHA-1 info hash. */
    QByteArray infoHash() const { return _infoHash; }
    /** @brief Info hash as a lowercase hex string (40 characters). */
    QString infoHashHex() const { return QString::fromLatin1(_infoHash.toHex()); }

    /** @brief Human-readable name extracted from the @c dn parameter. */
    QString displayName() const { return _displayName; }
    /** @brief Set or override the display name. */
    void setDisplayName(const QString& value) { _displayName = value; }

    /** @brief Tracker URLs extracted from @c tr parameters. */
    QStringList trackers() const { return _trackers; }
    /** @brief Append a tracker URL. */
    void addTracker(const QString& url) { _trackers.append(url); }

    /** @brief Exact content length from the @c xl parameter, or 0 if not present. */
    qint64 exactLength() const { return _exactLength; }

    /**
     * @brief Reconstruct a magnet URI string from the current state.
     * @return A @c "magnet:?..." URI string.
     */
    QString toUri() const;

    /**
     * @brief Create a MagnetLink from a hex-encoded info hash.
     *
     * Automatically appends a set of well-known public trackers.
     * @param hexHash 40-character hex-encoded SHA-1 hash.
     * @return A valid MagnetLink, or an invalid one if @p hexHash is malformed.
     */
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
