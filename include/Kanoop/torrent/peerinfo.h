#ifndef PEERINFO_H
#define PEERINFO_H

#include <Kanoop/torrent/kanooptorrent.h>
#include <QHostAddress>
#include <QString>

/**
 * @brief Snapshot of a connected peer's state.
 *
 * PeerInfo is a lightweight value class populated by Torrent::peers().
 * Each instance captures a point-in-time view of one peer connection
 * including transfer rates, identification, and capability flags.
 */
class LIBKANOOPTORRENT_EXPORT PeerInfo
{
public:
    /**
     * @brief Capability and state flags for a peer connection.
     *
     * These flags can be combined with bitwise OR.
     */
    enum Flag {
        NoFlags      = 0x0000,
        Interesting  = 0x0001,  ///< We are interested in pieces this peer has.
        Choked       = 0x0002,  ///< We are choking this peer (not uploading to it).
        RemoteChoked = 0x0004,  ///< This peer is choking us (not uploading to us).
        Seed         = 0x0008,  ///< Peer has the complete torrent.
        Encrypted    = 0x0010,  ///< Connection is encrypted (RC4 or AES).
        Incoming     = 0x0020,  ///< Peer initiated the connection.
        Snubbed      = 0x0040,  ///< Peer has not sent data for a long time.
        UploadOnly   = 0x0080,  ///< Peer is in upload-only mode (e.g. super-seeding).
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    PeerInfo() = default;

    // ── Identity ────────────────────────────────────────────────────────

    /** @brief Peer's IP address. */
    QHostAddress address() const { return _address; }
    void setAddress(const QHostAddress& value) { _address = value; }

    /** @brief Peer's TCP port. */
    quint16 port() const { return _port; }
    void setPort(quint16 value) { _port = value; }

    /**
     * @brief Peer's client identification string (e.g. "qBittorrent 4.6.2").
     *
     * Derived from the peer's extension handshake or peer ID encoding.
     */
    QString client() const { return _client; }
    void setClient(const QString& value) { _client = value; }

    // ── Transfer ────────────────────────────────────────────────────────

    /** @brief Current download rate from this peer in bytes/sec. */
    qint64 downloadRate() const { return _downloadRate; }
    void setDownloadRate(qint64 value) { _downloadRate = value; }

    /** @brief Current upload rate to this peer in bytes/sec. */
    qint64 uploadRate() const { return _uploadRate; }
    void setUploadRate(qint64 value) { _uploadRate = value; }

    /** @brief Total bytes downloaded from this peer. */
    qint64 totalDownload() const { return _totalDownload; }
    void setTotalDownload(qint64 value) { _totalDownload = value; }

    /** @brief Total bytes uploaded to this peer. */
    qint64 totalUpload() const { return _totalUpload; }
    void setTotalUpload(qint64 value) { _totalUpload = value; }

    // ── Progress ────────────────────────────────────────────────────────

    /** @brief Peer's download progress as a ratio from 0.0 to 1.0. */
    double progress() const { return _progress; }
    void setProgress(double value) { _progress = value; }

    // ── Flags ───────────────────────────────────────────────────────────

    /** @brief Capability and state flags for this connection. */
    Flags flags() const { return _flags; }
    void setFlags(Flags value) { _flags = value; }

private:
    QHostAddress _address;
    quint16 _port         = 0;
    QString _client;
    qint64 _downloadRate  = 0;
    qint64 _uploadRate    = 0;
    qint64 _totalDownload = 0;
    qint64 _totalUpload   = 0;
    double _progress      = 0.0;
    Flags _flags          = NoFlags;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(PeerInfo::Flags)

#endif // PEERINFO_H
