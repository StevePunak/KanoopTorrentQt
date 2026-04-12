#ifndef TORRENTCLIENT_H
#define TORRENTCLIENT_H

#include <Kanoop/torrent/kanooptorrent.h>
#include <Kanoop/torrent/torrent.h>
#include <Kanoop/torrent/sessionstats.h>
#include <Kanoop/utility/loggingbaseclass.h>
#include <QNetworkProxy>
#include <QObject>

class QTimer;

class LIBKANOOPTORRENT_EXPORT TorrentClient : public QObject,
                                               public LoggingBaseClass
{
    Q_OBJECT
public:
    enum EncryptionMode {
        EncryptionEnabled,      // Prefer encrypted, allow plaintext
        EncryptionForced,       // Require encryption
        EncryptionDisabled,     // Disable encryption entirely
    };
    Q_ENUM(EncryptionMode)

    explicit TorrentClient(QObject* parent = nullptr);
    ~TorrentClient();

    // --- Torrent management ---
    Torrent* addTorrent(const MagnetLink& magnetLink, const QString& downloadDirectory = QString());
    Torrent* addTorrent(const QString& torrentFilePath, const QString& downloadDirectory = QString());
    Torrent* addTorrent(const QByteArray& torrentData, const QString& downloadDirectory = QString());
    void removeTorrent(Torrent* torrent, bool deleteFiles = false);

    // --- Query ---
    QList<Torrent*> torrents() const { return _torrents; }
    Torrent* findTorrent(const QByteArray& infoHash) const;

    // --- Default download directory ---
    QString defaultDownloadDirectory() const { return _defaultDownloadDirectory; }
    void setDefaultDownloadDirectory(const QString& value) { _defaultDownloadDirectory = value; }

    // --- SOCKS5 proxy ---
    QNetworkProxy networkProxy() const { return _networkProxy; }
    void setNetworkProxy(const QNetworkProxy& value);

    // --- Bandwidth limits (0 = unlimited) ---
    int downloadRateLimit() const;
    void setDownloadRateLimit(int bytesPerSecond);
    int uploadRateLimit() const;
    void setUploadRateLimit(int bytesPerSecond);

    // --- Connection limits (0 = unlimited) ---
    int maxConnections() const;
    void setMaxConnections(int value);
    int maxUploads() const;
    void setMaxUploads(int value);

    // --- Listen interface ---
    QString listenInterfaces() const;
    void setListenInterfaces(const QString& value);

    // --- Protocol settings ---
    bool isDhtEnabled() const;
    void setDhtEnabled(bool enabled);
    bool isLsdEnabled() const;
    void setLsdEnabled(bool enabled);

    // --- Encryption ---
    EncryptionMode encryptionMode() const;
    void setEncryptionMode(EncryptionMode mode);

    // --- User agent ---
    QString userAgent() const;
    void setUserAgent(const QString& value);

    // --- Session statistics ---
    SessionStats sessionStats() const;
    void requestSessionStats();

    // --- Resume data ---
    QString resumeDataDirectory() const { return _resumeDataDirectory; }
    void setResumeDataDirectory(const QString& path);
    void saveAllResumeData();
    void loadResumeData();

    // --- Global controls ---
    void startAll();
    void stopAll();

    // --- Alert processing ---
    void processAlerts();

signals:
    // Torrent lifecycle
    void torrentAdded(Torrent* torrent);
    void torrentRemoved(const QByteArray& infoHash);
    void torrentStateChanged(Torrent* torrent, Torrent::State state);
    void torrentComplete(Torrent* torrent);

    // Session events
    void listenSucceeded(const QString& address, int port);
    void listenFailed(const QString& address, int port, const QString& error);
    void externalAddressDetected(const QString& address);
    void dhtBootstrapComplete();
    void sessionStatsReceived(const SessionStats& stats);

    // Resume data
    void resumeDataSaved(const QByteArray& infoHash);
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
