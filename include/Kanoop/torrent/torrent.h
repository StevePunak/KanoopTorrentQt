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

    void start();
    void stop();
    void pause();
    void resume();

    State state() const { return _state; }

    // Info
    QString name() const;
    QByteArray infoHash() const { return _infoHash; }
    QString infoHashHex() const { return QString::fromLatin1(_infoHash.toHex()); }

    // Progress
    double progress() const;
    qint64 bytesDownloaded() const;
    qint64 totalSize() const;
    int connectedPeers() const;
    qint64 downloadRate() const;

    // Metadata
    bool hasMetadata() const { return _hasMetadata; }

    // File management (requires metadata)
    int fileCount() const;
    QString fileName(int index) const;
    QStringList fileNames() const;
    void setFilePriority(int index, int priority);    // 0=skip, 1=low, 4=normal, 7=highest
    void setAllFilePriorities(int priority);
    int findFileByName(const QString& partialName) const; // case-insensitive substring match

    QString downloadDirectory() const { return _downloadDirectory; }
    QString outputPath() const;

    // Internal — used by TorrentClient
    void setHandle(void* handle); // lt::torrent_handle*
    void* handle() const { return _handle; }
    void setMagnetLink(const MagnetLink& link) { _magnetLink = link; }
    void setInfoHash(const QByteArray& hash) { _infoHash = hash; }
    void setDownloadDirectory(const QString& dir) { _downloadDirectory = dir; }
    void updateFromStatus(); // called by TorrentClient::processAlerts

signals:
    void stateChanged(Torrent::State state);
    void progressUpdated(double ratio);
    void fileCompleted(int fileIndex, const QString& filePath);
    void downloadComplete();
    void metadataReceived();
    void error(const QString& message);

private:
    void setState(State newState);

    State _state = Idle;
    MagnetLink _magnetLink;
    QByteArray _infoHash;
    QString _downloadDirectory;
    bool _hasMetadata     = false;
    bool _downloadEmitted = false;
    void* _handle         = nullptr; // lt::torrent_handle*
};

#endif // TORRENT_H
