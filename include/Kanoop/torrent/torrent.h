#ifndef TORRENT_H
#define TORRENT_H

#include <Kanoop/torrent/kanooptorrent.h>
#include <Kanoop/torrent/magnetlink.h>
#include <Kanoop/utility/loggingbaseclass.h>
#include <QObject>

class LIBKANOOPTORRENT_EXPORT Torrent : public QObject,
                                         public LoggingBaseClass
{
    Q_OBJECT
public:
    enum State {
        Idle,
        FetchingMetadata,
        Checking,
        Downloading,
        Seeding,
        Paused,
        Error,
    };
    Q_ENUM(State)

    Torrent(QObject* parent = nullptr);
    ~Torrent();

    // --- Control ---
    void start();
    void stop();
    void pause();
    void resume();

    State state() const { return _state; }

    // --- Info ---
    QString name() const;
    QByteArray infoHash() const { return _infoHash; }
    QString infoHashHex() const { return QString::fromLatin1(_infoHash.toHex()); }

    // --- Download progress ---
    double progress() const;
    qint64 bytesDownloaded() const;
    qint64 totalSize() const;
    int connectedPeers() const;
    qint64 downloadRate() const;

    // --- Upload stats ---
    qint64 uploadRate() const;
    qint64 totalUploaded() const;
    double ratio() const;

    // --- Piece info ---
    int totalPieces() const;
    int downloadedPieces() const;
    int pieceSize() const;

    // --- ETA ---
    int eta() const; // seconds remaining, -1 if unknown

    // --- Metadata ---
    bool hasMetadata() const { return _hasMetadata; }

    // --- File management (requires metadata) ---
    int fileCount() const;
    QString fileName(int index) const;
    QStringList fileNames() const;
    qint64 fileSize(int index) const;
    void setFilePriority(int index, int priority);    // 0=skip, 1=low, 4=normal, 7=highest
    void setAllFilePriorities(int priority);
    int findFileByName(const QString& partialName) const; // case-insensitive substring match

    // --- Paths ---
    QString downloadDirectory() const { return _downloadDirectory; }
    QString outputPath() const;
    void moveStorage(const QString& newPath);

    // --- Per-torrent options ---
    bool isSequentialDownload() const;
    void setSequentialDownload(bool enabled);
    bool isAutoManaged() const;
    void setAutoManaged(bool enabled);
    int downloadLimit() const;
    void setDownloadLimit(int bytesPerSecond);
    int uploadLimit() const;
    void setUploadLimit(int bytesPerSecond);
    int maxConnections() const;
    void setMaxConnections(int value);
    int maxUploads() const;
    void setMaxUploads(int value);

    // --- Seed ratio limit ---
    double seedRatioLimit() const { return _seedRatioLimit; }
    void setSeedRatioLimit(double ratio);

    // --- Tracker management ---
    QStringList trackers() const;
    void addTracker(const QString& url);
    void removeTracker(const QString& url);
    void forceReannounce();

    // --- Resume data ---
    void saveResumeData();

    // --- Internal — used by TorrentClient ---
    void setHandle(void* handle); // lt::torrent_handle*
    void* handle() const { return _handle; }
    void setMagnetLink(const MagnetLink& link) { _magnetLink = link; }
    void setInfoHash(const QByteArray& hash) { _infoHash = hash; }
    void setDownloadDirectory(const QString& dir) { _downloadDirectory = dir; }
    void updateFromStatus(); // called by TorrentClient::processAlerts
    void checkSeedRatio();   // called after progress update

signals:
    void stateChanged(Torrent::State state);
    void progressUpdated(double ratio);
    void fileCompleted(int fileIndex, const QString& filePath);
    void downloadComplete();
    void metadataReceived();
    void error(const QString& message);

    // Tracker signals
    void trackerAnnounced(const QString& url);
    void trackerError(const QString& url, const QString& message);

    // Storage signals
    void storageMoved(const QString& newPath);
    void storageMoveError(const QString& error);

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
