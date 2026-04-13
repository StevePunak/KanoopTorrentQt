#ifndef TORRENTCLIENT_H
#define TORRENTCLIENT_H

#include <Kanoop/torrent/kanooptorrent.h>
#include <Kanoop/torrent/torrent.h>
#include <Kanoop/torrent/sessionstats.h>
#include <Kanoop/utility/loggingbaseclass.h>
#include <QHostAddress>
#include <QNetworkProxy>
#include <QObject>

class QTimer;

/**
 * @brief Qt wrapper around a libtorrent session.
 *
 * TorrentClient owns the underlying libtorrent session and manages the
 * lifecycle of all Torrent objects added to it. It polls for libtorrent
 * alerts on a 500 ms timer and translates them into Qt signals.
 *
 * Typical usage:
 * @code
 *   auto* client = new TorrentClient(this);
 *   client->setDefaultDownloadDirectory("/tmp/downloads");
 *   client->setResumeDataDirectory("/tmp/resume");
 *   client->loadResumeData();
 *
 *   Torrent* t = client->addTorrent(MagnetLink(uri));
 *   connect(t, &Torrent::downloadComplete, this, &MyApp::onDone);
 * @endcode
 */
class LIBKANOOPTORRENT_EXPORT TorrentClient : public QObject,
                                               public LoggingBaseClass
{
    Q_OBJECT
public:
    /**
     * @brief Peer connection encryption policy.
     */
    enum EncryptionMode {
        EncryptionEnabled,      ///< Prefer encrypted connections, allow plaintext fallback.
        EncryptionForced,       ///< Require encryption; reject plaintext peers.
        EncryptionDisabled,     ///< Disable encryption entirely.
    };
    Q_ENUM(EncryptionMode)

    /**
     * @brief Construct a TorrentClient with DHT and LSD enabled by default.
     * @param parent Optional QObject parent.
     */
    explicit TorrentClient(QObject* parent = nullptr);
    ~TorrentClient();

    // ── Torrent management ──────────────────────────────────────────────

    /**
     * @brief Add a torrent from a magnet link.
     * @param magnetLink The parsed magnet link.
     * @param downloadDirectory Save path override. Uses defaultDownloadDirectory() if empty.
     * @return Pointer to the new Torrent, or @c nullptr if a duplicate or invalid.
     */
    Torrent* addTorrent(const MagnetLink& magnetLink, const QString& downloadDirectory = QString());

    /**
     * @brief Add a torrent from a @c .torrent file on disk.
     * @param torrentFilePath Path to the @c .torrent file.
     * @param downloadDirectory Save path override. Uses defaultDownloadDirectory() if empty.
     * @return Pointer to the new Torrent, or @c nullptr on failure.
     */
    Torrent* addTorrent(const QString& torrentFilePath, const QString& downloadDirectory = QString());

    /**
     * @brief Add a torrent from raw @c .torrent file bytes.
     * @param torrentData The bencoded torrent data.
     * @param downloadDirectory Save path override. Uses defaultDownloadDirectory() if empty.
     * @return Pointer to the new Torrent, or @c nullptr on failure.
     */
    Torrent* addTorrent(const QByteArray& torrentData, const QString& downloadDirectory = QString());

    /**
     * @brief Remove a torrent from the session.
     * @param torrent The torrent to remove. Becomes invalid after this call.
     * @param deleteFiles If @c true, also delete downloaded data from disk.
     */
    void removeTorrent(Torrent* torrent, bool deleteFiles = false);

    // ── Query ───────────────────────────────────────────────────────────

    /** @brief Return all managed torrents. */
    QList<Torrent*> torrents() const { return _torrents; }

    /**
     * @brief Find a torrent by its 20-byte SHA-1 info hash.
     * @param infoHash Raw 20-byte hash.
     * @return Matching Torrent pointer, or @c nullptr if not found.
     */
    Torrent* findTorrent(const QByteArray& infoHash) const;

    // ── Default download directory ──────────────────────────────────────

    /** @brief Default save path for new torrents when no override is given. */
    QString defaultDownloadDirectory() const { return _defaultDownloadDirectory; }
    /** @brief Set the default save path for new torrents. */
    void setDefaultDownloadDirectory(const QString& value) { _defaultDownloadDirectory = value; }

    // ── SOCKS5 proxy ────────────────────────────────────────────────────

    /** @brief Current SOCKS5 proxy configuration. */
    QNetworkProxy networkProxy() const { return _networkProxy; }

    /**
     * @brief Route all libtorrent traffic through a SOCKS5 proxy.
     *
     * Supports both plain SOCKS5 and SOCKS5 with username/password
     * authentication. Pass a default-constructed QNetworkProxy to disable.
     * @param value The proxy configuration.
     */
    void setNetworkProxy(const QNetworkProxy& value);

    // ── Bandwidth limits ────────────────────────────────────────────────

    /** @brief Global download rate limit in bytes/sec. 0 means unlimited. */
    int downloadRateLimit() const;
    /** @brief Set the global download rate limit in bytes/sec. Pass 0 for unlimited. */
    void setDownloadRateLimit(int bytesPerSecond);

    /** @brief Global upload rate limit in bytes/sec. 0 means unlimited. */
    int uploadRateLimit() const;
    /** @brief Set the global upload rate limit in bytes/sec. Pass 0 for unlimited. */
    void setUploadRateLimit(int bytesPerSecond);

    // ── Connection limits ───────────────────────────────────────────────

    /** @brief Maximum number of simultaneous connections across all torrents. 0 = unlimited. */
    int maxConnections() const;
    /** @brief Set the global connection limit. */
    void setMaxConnections(int value);

    /** @brief Maximum number of unchoke slots (upload slots). 0 = unlimited. */
    int maxUploads() const;
    /** @brief Set the maximum number of unchoke slots. */
    void setMaxUploads(int value);

    // ── Listen interface ────────────────────────────────────────────────

    /**
     * @brief Current listen interface string.
     *
     * Format: @c "address:port" pairs, comma-separated.
     * Example: @c "0.0.0.0:6881,[::]:6881"
     */
    QString listenInterfaces() const;

    /**
     * @brief Set the listen interfaces for incoming connections.
     * @param value Comma-separated @c "address:port" pairs.
     */
    void setListenInterfaces(const QString& value);

    // ── Protocol settings ───────────────────────────────────────────────

    /** @brief Whether the Distributed Hash Table (DHT) is enabled. */
    bool isDhtEnabled() const;
    /** @brief Enable or disable DHT. */
    void setDhtEnabled(bool enabled);

    /** @brief Whether Local Service Discovery (LSD) is enabled. */
    bool isLsdEnabled() const;
    /** @brief Enable or disable LSD. */
    void setLsdEnabled(bool enabled);

    // ── Encryption ──────────────────────────────────────────────────────

    /** @brief Current peer connection encryption policy. */
    EncryptionMode encryptionMode() const;
    /** @brief Set the peer connection encryption policy. */
    void setEncryptionMode(EncryptionMode mode);

    // ── User agent ──────────────────────────────────────────────────────

    /** @brief The user-agent string sent to trackers and peers. */
    QString userAgent() const;
    /** @brief Set the user-agent string. */
    void setUserAgent(const QString& value);

    // ── Session statistics ──────────────────────────────────────────────

    /**
     * @brief Compute an aggregate snapshot of session statistics.
     *
     * Iterates all managed torrents and sums their transfer totals,
     * rates, and peer counts. Also queries the DHT routing table size.
     * @return A SessionStats value object.
     */
    SessionStats sessionStats() const;

    /**
     * @brief Post an asynchronous session-stats request to libtorrent.
     *
     * When the stats are ready, the sessionStatsReceived() signal is emitted.
     */
    void requestSessionStats();

    // ── IP filtering ────────────────────────────────────────────────────

    /**
     * @brief Block a range of IP addresses.
     *
     * Peers connecting from addresses in the range will be rejected.
     * Call with the same start and end to block a single address.
     * @param first Start of the IP range (inclusive).
     * @param last End of the IP range (inclusive).
     */
    void addIpFilter(const QHostAddress& first, const QHostAddress& last);

    /**
     * @brief Remove a previously added IP filter range.
     * @param first Start of the IP range (inclusive).
     * @param last End of the IP range (inclusive).
     */
    void removeIpFilter(const QHostAddress& first, const QHostAddress& last);

    /**
     * @brief Load a P2P-format blocklist from a file.
     *
     * Supports the widely-used P2P plaintext format where each line is:
     * @c "description:startIP-endIP". Lines starting with @c # are ignored.
     * @param filePath Path to the blocklist file.
     * @return Number of ranges loaded, or -1 on file error.
     */
    int loadBlocklist(const QString& filePath);

    /** @brief Remove all IP filter rules, allowing all addresses. */
    void clearIpFilter();

    // ── Resume data ─────────────────────────────────────────────────────

    /** @brief Directory where @c .fastresume files are stored. */
    QString resumeDataDirectory() const { return _resumeDataDirectory; }

    /**
     * @brief Set the resume-data directory and create it if it doesn't exist.
     * @param path Filesystem path for @c .fastresume files.
     */
    void setResumeDataDirectory(const QString& path);

    /**
     * @brief Request resume-data saves for every managed torrent.
     *
     * Each torrent's resume data is written asynchronously. The
     * resumeDataSaved() or resumeDataFailed() signal fires per torrent
     * once the save completes.
     */
    void saveAllResumeData();

    /**
     * @brief Scan the resume-data directory and re-add all found torrents.
     *
     * Call this at startup after setResumeDataDirectory() to restore
     * the previous session state.
     */
    void loadResumeData();

    // ── Global controls ─────────────────────────────────────────────────

    /** @brief Resume all paused/stopped torrents. */
    void startAll();
    /** @brief Pause all active torrents. */
    void stopAll();

    // ── Alert processing ────────────────────────────────────────────────

    /**
     * @brief Process pending libtorrent alerts.
     *
     * Called automatically every 500 ms by an internal timer. Can also
     * be called manually for tighter polling. Translates libtorrent
     * alerts into the appropriate Qt signals on TorrentClient and Torrent.
     */
    void processAlerts();

signals:
    /** @brief Emitted after a torrent is successfully added to the session. */
    void torrentAdded(Torrent* torrent);
    /** @brief Emitted after a torrent is removed. @p infoHash identifies the removed torrent. */
    void torrentRemoved(const QByteArray& infoHash);
    /** @brief Emitted when any torrent transitions to a new state. */
    void torrentStateChanged(Torrent* torrent, Torrent::State state);
    /** @brief Emitted when a torrent finishes downloading and enters seeding. */
    void torrentComplete(Torrent* torrent);

    /** @brief Emitted when the session begins listening on an interface. */
    void listenSucceeded(const QString& address, int port);
    /** @brief Emitted when binding to a listen interface fails. */
    void listenFailed(const QString& address, int port, const QString& error);
    /** @brief Emitted when the session's external (public) IP is detected. */
    void externalAddressDetected(const QString& address);
    /** @brief Emitted when the DHT routing table is initially populated. */
    void dhtBootstrapComplete();
    /** @brief Emitted in response to requestSessionStats(). */
    void sessionStatsReceived(const SessionStats& stats);

    /** @brief Emitted when a torrent's resume data is successfully written to disk. */
    void resumeDataSaved(const QByteArray& infoHash);
    /** @brief Emitted when saving resume data for a torrent fails. */
    void resumeDataFailed(const QByteArray& infoHash, const QString& error);

private:
    void configureProxy();
    void applySetting(int name, int value);
    void applySetting(int name, bool value);
    void applySetting(int name, const std::string& value);
    int getSettingInt(int name) const;
    bool getSettingBool(int name) const;
    std::string getSettingString(int name) const;
    Torrent* findTorrentByHandle(void* alertHandle);

    void* _session = nullptr; // lt::session*
    QList<Torrent*> _torrents;
    QNetworkProxy _networkProxy;
    QString _defaultDownloadDirectory;
    QString _resumeDataDirectory;
    QTimer* _alertTimer = nullptr;
};

#endif // TORRENTCLIENT_H
