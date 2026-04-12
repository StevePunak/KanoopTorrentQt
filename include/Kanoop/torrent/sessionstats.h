#ifndef SESSIONSTATS_H
#define SESSIONSTATS_H

#include <Kanoop/torrent/kanooptorrent.h>
#include <QtGlobal>

class LIBKANOOPTORRENT_EXPORT SessionStats
{
public:
    SessionStats() = default;

    // Transfer totals (bytes)
    qint64 totalDownloaded() const { return _totalDownloaded; }
    void setTotalDownloaded(qint64 value) { _totalDownloaded = value; }
    qint64 totalUploaded() const { return _totalUploaded; }
    void setTotalUploaded(qint64 value) { _totalUploaded = value; }

    // Transfer rates (bytes/sec)
    qint64 downloadRate() const { return _downloadRate; }
    void setDownloadRate(qint64 value) { _downloadRate = value; }
    qint64 uploadRate() const { return _uploadRate; }
    void setUploadRate(qint64 value) { _uploadRate = value; }

    // Peers
    int totalPeers() const { return _totalPeers; }
    void setTotalPeers(int value) { _totalPeers = value; }

    // DHT
    int dhtNodes() const { return _dhtNodes; }
    void setDhtNodes(int value) { _dhtNodes = value; }

    // Torrents
    int activeTorrents() const { return _activeTorrents; }
    void setActiveTorrents(int value) { _activeTorrents = value; }
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
