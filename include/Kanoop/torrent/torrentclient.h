#ifndef TORRENTCLIENT_H
#define TORRENTCLIENT_H

#include <Kanoop/torrent/kanooptorrent.h>
#include <Kanoop/torrent/torrent.h>
#include <Kanoop/utility/loggingbaseclass.h>
#include <QNetworkProxy>
#include <QObject>

class QTimer;

class LIBKANOOPTORRENT_EXPORT TorrentClient : public QObject,
                                               public LoggingBaseClass
{
    Q_OBJECT
public:
    explicit TorrentClient(QObject* parent = nullptr);
    ~TorrentClient();

    // Add torrents
    Torrent* addTorrent(const MagnetLink& magnetLink, const QString& downloadDirectory);
    Torrent* addTorrent(const QString& torrentFilePath, const QString& downloadDirectory);
    Torrent* addTorrent(const QByteArray& torrentData, const QString& downloadDirectory);

    // Remove
    void removeTorrent(Torrent* torrent, bool deleteFiles = false);

    // Query
    QList<Torrent*> torrents() const { return _torrents; }
    Torrent* findTorrent(const QByteArray& infoHash) const;

    // Default download directory
    QString defaultDownloadDirectory() const { return _defaultDownloadDirectory; }
    void setDefaultDownloadDirectory(const QString& value) { _defaultDownloadDirectory = value; }

    // SOCKS5 proxy
    QNetworkProxy networkProxy() const { return _networkProxy; }
    void setNetworkProxy(const QNetworkProxy& value);

    // Global controls
    void startAll();
    void stopAll();

    // Must be called periodically (e.g. from a timer) to process libtorrent alerts
    void processAlerts();

signals:
    void torrentAdded(Torrent* torrent);
    void torrentRemoved(const QByteArray& infoHash);
    void torrentStateChanged(Torrent* torrent, Torrent::State state);
    void torrentComplete(Torrent* torrent);

private:
    void configureProxy();

    void* _session = nullptr; // lt::session*
    QList<Torrent*> _torrents;
    QNetworkProxy _networkProxy;
    QString _defaultDownloadDirectory;
    QTimer* _alertTimer = nullptr;
};

#endif // TORRENTCLIENT_H
