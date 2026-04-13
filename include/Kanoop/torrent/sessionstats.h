#ifndef SESSIONSTATS_H
#define SESSIONSTATS_H

#include <Kanoop/torrent/kanooptorrent.h>
#include <QtGlobal>

/**
 * @brief Aggregate statistics snapshot for a TorrentClient session.
 *
 * SessionStats is a lightweight value class that captures a point-in-time
 * view of session-wide transfer totals, rates, peer counts, and DHT state.
 * Obtain one via TorrentClient::sessionStats() or the
 * TorrentClient::sessionStatsReceived() signal.
 */
class LIBKANOOPTORRENT_EXPORT SessionStats
{
public:
    SessionStats() = default;

    // ── Transfer totals ─────────────────────────────────────────────────

    /** @brief Total bytes downloaded across all torrents since session start. */
    qint64 totalDownloaded() const { return _totalDownloaded; }
    void setTotalDownloaded(qint64 value) { _totalDownloaded = value; }

    /** @brief Total bytes uploaded across all torrents since session start. */
    qint64 totalUploaded() const { return _totalUploaded; }
    void setTotalUploaded(qint64 value) { _totalUploaded = value; }

    // ── Transfer rates ──────────────────────────────────────────────────

    /** @brief Combined download rate in bytes/sec across all torrents. */
    qint64 downloadRate() const { return _downloadRate; }
    void setDownloadRate(qint64 value) { _downloadRate = value; }

    /** @brief Combined upload rate in bytes/sec across all torrents. */
    qint64 uploadRate() const { return _uploadRate; }
    void setUploadRate(qint64 value) { _uploadRate = value; }

    // ── Peers ───────────────────────────────────────────────────────────

    /** @brief Total number of connected peers across all torrents. */
    int totalPeers() const { return _totalPeers; }
    void setTotalPeers(int value) { _totalPeers = value; }

    // ── DHT ─────────────────────────────────────────────────────────────

    /** @brief Number of nodes in the DHT routing table (IPv4 + IPv6). */
    int dhtNodes() const { return _dhtNodes; }
    void setDhtNodes(int value) { _dhtNodes = value; }

    // ── Torrent counts ──────────────────────────────────────────────────

    /** @brief Number of torrents currently downloading or seeding. */
    int activeTorrents() const { return _activeTorrents; }
    void setActiveTorrents(int value) { _activeTorrents = value; }

    /** @brief Number of paused torrents. */
    int pausedTorrents() const { return _pausedTorrents; }
    void setPausedTorrents(int value) { _pausedTorrents = value; }

private:
    qint64 _totalDownloaded = 0;
    qint64 _totalUploaded   = 0;
    qint64 _downloadRate    = 0;
    qint64 _uploadRate      = 0;
    int _totalPeers         = 0;
    int _dhtNodes           = 0;
    int _activeTorrents     = 0;
    int _pausedTorrents     = 0;
};

#endif // SESSIONSTATS_H
