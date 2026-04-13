#ifndef TORRENTCREATOR_H
#define TORRENTCREATOR_H

#include <Kanoop/torrent/kanooptorrent.h>
#include <Kanoop/utility/loggingbaseclass.h>
#include <QObject>
#include <QStringList>

/**
 * @brief Creates @c .torrent metainfo files from local content.
 *
 * TorrentCreator wraps libtorrent's @c create_torrent API and produces
 * bencoded @c .torrent files that can be distributed to trackers or
 * loaded by TorrentClient::addTorrent().
 *
 * The creation runs synchronously in create() — for large file sets
 * consider calling from a worker thread or using QFuture.
 *
 * @code
 *   TorrentCreator creator;
 *   creator.setTrackers({"udp://tracker.example.com:6969/announce"});
 *   creator.setComment("My album");
 *   creator.setPrivate(true);
 *
 *   if (creator.create("/path/to/album", "/tmp/album.torrent")) {
 *       qDebug() << "Created" << creator.totalSize() << "bytes,"
 *                << creator.fileCount() << "files,"
 *                << creator.pieceCount() << "pieces";
 *   }
 * @endcode
 */
class LIBKANOOPTORRENT_EXPORT TorrentCreator : public QObject,
                                                public LoggingBaseClass
{
    Q_OBJECT
public:
    /**
     * @brief Construct a TorrentCreator.
     * @param parent Optional QObject parent.
     */
    explicit TorrentCreator(QObject* parent = nullptr);

    // ── Configuration ───────────────────────────────────────────────────

    /**
     * @brief Set the piece size in bytes.
     *
     * Must be a power of two (e.g. 16384, 32768, 65536, ...).
     * If not set or set to 0, libtorrent picks an optimal size based
     * on total content size.
     * @param bytes Piece size in bytes, or 0 for automatic.
     */
    void setPieceSize(int bytes);
    /** @brief Current piece size setting. 0 means automatic. */
    int pieceSize() const { return _pieceSize; }

    /**
     * @brief Set tracker announce URLs.
     *
     * Each string is a single announce URL. Multiple trackers are added
     * as separate tiers (one tier per URL). For grouped tiers, use
     * addTrackerTier().
     * @param urls List of tracker announce URLs.
     */
    void setTrackers(const QStringList& urls);
    /** @brief Current tracker list. */
    QStringList trackers() const { return _trackers; }

    /**
     * @brief Add a tracker tier (group of announce URLs).
     *
     * All URLs in the list share the same tier, and the client will
     * pick randomly among them.
     * @param urls List of announce URLs for this tier.
     */
    void addTrackerTier(const QStringList& urls);

    /**
     * @brief Set web seed URLs (BEP 19 / GetRight-style HTTP seeding).
     * @param urls List of HTTP/HTTPS base URLs.
     */
    void setWebSeeds(const QStringList& urls);
    /** @brief Current web seed list. */
    QStringList webSeeds() const { return _webSeeds; }

    /**
     * @brief Set the comment embedded in the torrent metainfo.
     * @param value Free-text comment string.
     */
    void setComment(const QString& value);
    /** @brief Current comment. */
    QString comment() const { return _comment; }

    /**
     * @brief Set the creator string embedded in the torrent metainfo.
     *
     * Defaults to @c "KanoopTorrentQt" if not set.
     * @param value Creator identification string.
     */
    void setCreator(const QString& value);
    /** @brief Current creator string. */
    QString creator() const { return _creator; }

    /**
     * @brief Mark the torrent as private (BEP 27).
     *
     * Private torrents disable DHT and PEX; peers can only be
     * obtained from the tracker.
     * @param enabled @c true for a private torrent.
     */
    void setPrivate(bool enabled);
    /** @brief Whether the torrent will be marked private. */
    bool isPrivate() const { return _private; }

    // ── Creation ────────────────────────────────────────────────────────

    /**
     * @brief Create a @c .torrent file from local content.
     *
     * @p sourcePath can be a single file or a directory. When a directory
     * is given, all files in it (recursively) are included.
     *
     * @param sourcePath Path to the file or directory to torrent.
     * @param outputPath Where to write the @c .torrent file.
     * @return @c true on success, @c false on failure (check errorString()).
     */
    bool create(const QString& sourcePath, const QString& outputPath);

    // ── Result info (valid after successful create()) ───────────────────

    /** @brief Total content size in bytes. Valid after a successful create(). */
    qint64 totalSize() const { return _totalSize; }
    /** @brief Number of files in the torrent. Valid after a successful create(). */
    int fileCount() const { return _fileCount; }
    /** @brief Number of pieces in the torrent. Valid after a successful create(). */
    int pieceCount() const { return _pieceCount; }
    /** @brief Last error message, or empty if no error. */
    QString errorString() const { return _errorString; }

signals:
    /**
     * @brief Emitted during piece hashing to report progress.
     * @param piecesHashed Number of pieces hashed so far.
     * @param totalPieces Total number of pieces.
     */
    void progressUpdated(int piecesHashed, int totalPieces);

private:
    int _pieceSize           = 0;
    QStringList _trackers;
    QList<QStringList> _trackerTiers;
    QStringList _webSeeds;
    QString _comment;
    QString _creator         = "KanoopTorrentQt";
    bool _private            = false;

    // Result state
    qint64 _totalSize        = 0;
    int _fileCount           = 0;
    int _pieceCount          = 0;
    QString _errorString;
};

#endif // TORRENTCREATOR_H
