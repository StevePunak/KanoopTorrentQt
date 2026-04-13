#ifndef TORRENT_H
#define TORRENT_H

#include <Kanoop/torrent/kanooptorrent.h>
#include <Kanoop/torrent/magnetlink.h>
#include <Kanoop/torrent/peerinfo.h>
#include <Kanoop/utility/loggingbaseclass.h>
#include <QObject>

/**
 * @brief Represents a single torrent within a TorrentClient session.
 *
 * Torrent objects are created by TorrentClient::addTorrent() and provide
 * a Qt-friendly interface over a libtorrent torrent handle. All state
 * updates are driven by TorrentClient::processAlerts() and delivered
 * as Qt signals.
 *
 * @note Do not construct Torrent objects directly; use TorrentClient::addTorrent().
 */
class LIBKANOOPTORRENT_EXPORT Torrent : public QObject,
                                         public LoggingBaseClass
{
    Q_OBJECT
public:
    /**
     * @brief Lifecycle state of a torrent.
     */
    enum State {
        Idle,               ///< Torrent is stopped or not yet started.
        FetchingMetadata,   ///< Downloading torrent metadata (magnet link resolution).
        Checking,           ///< Verifying existing data on disk.
        Downloading,        ///< Actively downloading pieces.
        Seeding,            ///< Download complete; uploading to peers.
        Paused,             ///< Explicitly paused by the user.
        Error,              ///< An unrecoverable error occurred.
    };
    Q_ENUM(State)

    Torrent(QObject* parent = nullptr);
    ~Torrent();

    // ── Control ─────────────────────────────────────────────────────────

    /** @brief Resume downloading/seeding. */
    void start();
    /** @brief Stop the torrent and reset state to Idle. */
    void stop();
    /** @brief Pause the torrent, keeping its state recoverable via resume(). */
    void pause();
    /** @brief Resume a paused torrent. */
    void resume();

    /** @brief Current lifecycle state. */
    State state() const { return _state; }

    // ── Info ────────────────────────────────────────────────────────────

    /**
     * @brief Human-readable torrent name.
     *
     * Returns the name from torrent metadata if available, otherwise
     * falls back to the magnet link display name or a truncated info hash.
     */
    QString name() const;

    /** @brief Raw 20-byte SHA-1 info hash. */
    QByteArray infoHash() const { return _infoHash; }
    /** @brief Info hash as a lowercase hex string (40 characters). */
    QString infoHashHex() const { return QString::fromLatin1(_infoHash.toHex()); }

    // ── Download progress ───────────────────────────────────────────────

    /** @brief Download progress as a ratio from 0.0 to 1.0. */
    double progress() const;
    /** @brief Total bytes downloaded so far. */
    qint64 bytesDownloaded() const;
    /** @brief Total size of all selected files in bytes. */
    qint64 totalSize() const;
    /** @brief Number of currently connected peers. */
    int connectedPeers() const;
    /** @brief Current download rate in bytes/sec. */
    qint64 downloadRate() const;

    // ── Upload stats ────────────────────────────────────────────────────

    /** @brief Current upload rate in bytes/sec. */
    qint64 uploadRate() const;
    /** @brief Total bytes uploaded over the lifetime of this torrent. */
    qint64 totalUploaded() const;
    /**
     * @brief Share ratio (uploaded / downloaded).
     * @return 0.0 if nothing has been downloaded yet.
     */
    double ratio() const;

    // ── Piece info ──────────────────────────────────────────────────────

    /** @brief Total number of pieces in this torrent. Requires metadata. */
    int totalPieces() const;
    /** @brief Number of pieces that have been downloaded and verified. */
    int downloadedPieces() const;
    /** @brief Size of each piece in bytes. Requires metadata. */
    int pieceSize() const;

    // ── ETA ─────────────────────────────────────────────────────────────

    /**
     * @brief Estimated seconds remaining until download completes.
     * @return Seconds remaining, or -1 if the rate is zero or unknown.
     */
    int eta() const;

    // ── Metadata ────────────────────────────────────────────────────────

    /**
     * @brief Whether torrent metadata has been received.
     *
     * File management methods require metadata. For magnet links, metadata
     * is fetched from peers after the torrent is added.
     */
    bool hasMetadata() const { return _hasMetadata; }

    // ── File management ─────────────────────────────────────────────────

    /** @brief Number of files in the torrent. Requires metadata. */
    int fileCount() const;

    /**
     * @brief Relative path of a file within the torrent.
     * @param index Zero-based file index.
     * @return Relative file path, or empty string if out of range.
     */
    QString fileName(int index) const;

    /** @brief List of all file paths in the torrent. */
    QStringList fileNames() const;

    /**
     * @brief Size of a file in bytes.
     * @param index Zero-based file index.
     * @return File size, or 0 if out of range or no metadata.
     */
    qint64 fileSize(int index) const;

    /**
     * @brief Set the download priority for a single file.
     * @param index Zero-based file index.
     * @param priority 0 = skip, 1 = low, 4 = normal, 7 = highest.
     */
    void setFilePriority(int index, int priority);

    /**
     * @brief Set the same download priority for every file.
     * @param priority 0 = skip, 1 = low, 4 = normal, 7 = highest.
     */
    void setAllFilePriorities(int priority);

    /**
     * @brief Find a file by partial name (case-insensitive substring match).
     * @param partialName Substring to search for.
     * @return Zero-based file index, or -1 if not found.
     */
    int findFileByName(const QString& partialName) const;

    /**
     * @brief Get the current download priority for a file.
     * @param index Zero-based file index.
     * @return Priority value (0 = skip, 1 = low, 4 = normal, 7 = highest), or -1 if invalid.
     */
    int filePriority(int index) const;

    /**
     * @brief Get per-file download progress as a list of byte counts.
     * @return A list with one entry per file containing bytes downloaded so far.
     *         Empty list if no metadata is available.
     */
    QList<qint64> fileProgress() const;

    /**
     * @brief Rename a file within the torrent.
     *
     * The rename is asynchronous. On completion, fileRenamed() is emitted;
     * on failure, fileRenameError() is emitted.
     * @param index Zero-based file index.
     * @param newName New relative path for the file.
     */
    void renameFile(int index, const QString& newName);

    // ── Paths ───────────────────────────────────────────────────────────

    /** @brief The directory where this torrent's data is being saved. */
    QString downloadDirectory() const { return _downloadDirectory; }

    /**
     * @brief Full output path (save_path + torrent name).
     *
     * For single-file torrents this is the file path; for multi-file
     * torrents this is the containing directory.
     */
    QString outputPath() const;

    /**
     * @brief Move the torrent's data to a new directory.
     *
     * The move is asynchronous. On completion, storageMoved() is emitted;
     * on failure, storageMoveError() is emitted.
     * @param newPath Destination directory.
     */
    void moveStorage(const QString& newPath);

    // ── Per-torrent options ─────────────────────────────────────────────

    /** @brief Whether pieces are requested in sequential order. */
    bool isSequentialDownload() const;
    /**
     * @brief Enable or disable sequential piece downloading.
     *
     * Useful for streaming: the first pieces arrive before later ones,
     * so playback can begin before the download finishes.
     */
    void setSequentialDownload(bool enabled);

    /** @brief Whether libtorrent auto-manages this torrent's queue position. */
    bool isAutoManaged() const;
    /** @brief Enable or disable libtorrent's auto-managed mode. */
    void setAutoManaged(bool enabled);

    /** @brief Per-torrent download rate limit in bytes/sec. 0 = unlimited. */
    int downloadLimit() const;
    /** @brief Set the per-torrent download rate limit. Pass 0 for unlimited. */
    void setDownloadLimit(int bytesPerSecond);

    /** @brief Per-torrent upload rate limit in bytes/sec. 0 = unlimited. */
    int uploadLimit() const;
    /** @brief Set the per-torrent upload rate limit. Pass 0 for unlimited. */
    void setUploadLimit(int bytesPerSecond);

    /** @brief Maximum number of peer connections for this torrent. */
    int maxConnections() const;
    /** @brief Set the maximum peer connections for this torrent. */
    void setMaxConnections(int value);

    /** @brief Maximum number of upload slots for this torrent. */
    int maxUploads() const;
    /** @brief Set the maximum upload slots for this torrent. */
    void setMaxUploads(int value);

    // ── Seed ratio limit ────────────────────────────────────────────────

    /**
     * @brief Target share ratio after which the torrent is auto-paused.
     * @return 0.0 means no limit (seed indefinitely).
     */
    double seedRatioLimit() const { return _seedRatioLimit; }

    /**
     * @brief Set a share-ratio target; the torrent pauses when it is reached.
     * @param ratio Target ratio (e.g. 1.0 = upload as much as downloaded). 0 = no limit.
     */
    void setSeedRatioLimit(double ratio);

    // ── Tracker management ──────────────────────────────────────────────

    /** @brief List of tracker URLs currently attached to this torrent. */
    QStringList trackers() const;

    /**
     * @brief Append a tracker URL.
     * @param url The tracker announce URL.
     */
    void addTracker(const QString& url);

    /**
     * @brief Remove a tracker by URL.
     * @param url The tracker announce URL to remove.
     */
    void removeTracker(const QString& url);

    /** @brief Force an immediate re-announce to all trackers. */
    void forceReannounce();

    // ── Peer management ─────────────────────────────────────────────────

    /**
     * @brief Snapshot of all currently connected peers.
     *
     * Each PeerInfo captures identity, transfer rates, progress, and flags
     * at the time of the call. The list is not updated automatically;
     * call again to refresh.
     * @return List of PeerInfo value objects.
     */
    QList<PeerInfo> peers() const;

    /**
     * @brief Ban a peer by IP address.
     *
     * Adds the address to the session IP filter with a deny rule and
     * disconnects the peer if currently connected. The ban persists
     * for the lifetime of the session.
     * @param address The peer's IP address to ban.
     */
    void banPeer(const QHostAddress& address);

    // ── Resume data ─────────────────────────────────────────────────────

    /**
     * @brief Request an asynchronous resume-data save.
     *
     * The result arrives via TorrentClient::resumeDataSaved() or
     * TorrentClient::resumeDataFailed().
     */
    void saveResumeData();

    // ── Internal — used by TorrentClient ────────────────────────────────

    /** @internal Set the underlying libtorrent handle (lt::torrent_handle*). */
    void setHandle(void* handle);
    /** @internal Get the underlying libtorrent handle. */
    void* handle() const { return _handle; }
    /** @internal Associate a magnet link with this torrent. */
    void setMagnetLink(const MagnetLink& link) { _magnetLink = link; }
    /** @internal Set the raw 20-byte info hash. */
    void setInfoHash(const QByteArray& hash) { _infoHash = hash; }
    /** @internal Set the download directory. */
    void setDownloadDirectory(const QString& dir) { _downloadDirectory = dir; }
    /** @internal Called by TorrentClient::processAlerts() to update state from libtorrent status. */
    void updateFromStatus();
    /** @internal Called after status update to auto-pause if seed ratio is reached. */
    void checkSeedRatio();

signals:
    /** @brief Emitted when the torrent transitions to a new State. */
    void stateChanged(Torrent::State state);
    /** @brief Emitted periodically during download with the current progress ratio (0.0–1.0). */
    void progressUpdated(double ratio);
    /** @brief Emitted when an individual file finishes downloading. */
    void fileCompleted(int fileIndex, const QString& filePath);
    /** @brief Emitted once when the entire torrent download completes. */
    void downloadComplete();
    /** @brief Emitted when metadata is received (magnet links only). */
    void metadataReceived();
    /** @brief Emitted on torrent errors (tracker failures, I/O errors, etc.). */
    void error(const QString& message);

    /** @brief Emitted when a tracker announce succeeds. */
    void trackerAnnounced(const QString& url);
    /** @brief Emitted when a tracker announce fails. */
    void trackerError(const QString& url, const QString& message);

    /** @brief Emitted when moveStorage() completes successfully. */
    void storageMoved(const QString& newPath);
    /** @brief Emitted when moveStorage() fails. */
    void storageMoveError(const QString& error);

    /** @brief Emitted when renameFile() completes successfully. */
    void fileRenamed(int fileIndex, const QString& newName);
    /** @brief Emitted when renameFile() fails. */
    void fileRenameError(int fileIndex, const QString& error);

private:
    void setState(State newState);

    State _state            = Idle;
    MagnetLink _magnetLink;
    QByteArray _infoHash;
    QString _downloadDirectory;
    bool _hasMetadata       = false;
    bool _downloadEmitted   = false;
    double _seedRatioLimit  = 0.0; // 0 = no limit
    void* _handle           = nullptr; // lt::torrent_handle*
};

#endif // TORRENT_H
