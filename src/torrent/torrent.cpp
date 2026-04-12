#include <Kanoop/torrent/torrent.h>

#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/file_storage.hpp>

Torrent::Torrent(QObject* parent) :
    QObject(parent),
    LoggingBaseClass("torrent")
{
}

Torrent::~Torrent()
{
    // handle is owned by TorrentClient, deleted there
}

void Torrent::start()
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        h->resume();
        logText(LVL_INFO, QString("Starting torrent: %1").arg(name()));
    }
}

void Torrent::stop()
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        h->pause();
    }
    setState(Idle);
}

void Torrent::pause()
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        h->pause();
    }
    setState(Paused);
}

void Torrent::resume()
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        h->resume();
    }
}

QString Torrent::name() const
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        lt::torrent_status status = h->status();
        std::string n = status.name;
        if(!n.empty()) {
            return QString::fromStdString(n);
        }
    }

    if(!_magnetLink.displayName().isEmpty()) {
        return _magnetLink.displayName();
    }
    return infoHashHex().left(16) + "...";
}

double Torrent::progress() const
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        return static_cast<double>(h->status().progress);
    }
    return 0.0;
}

qint64 Torrent::bytesDownloaded() const
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        return h->status().total_done;
    }
    return 0;
}

qint64 Torrent::totalSize() const
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        return h->status().total;
    }
    return 0;
}

int Torrent::connectedPeers() const
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        return h->status().num_peers;
    }
    return 0;
}

qint64 Torrent::downloadRate() const
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        return h->status().download_rate;
    }
    return 0;
}

int Torrent::fileCount() const
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid() && h->torrent_file()) {
        return h->torrent_file()->num_files();
    }
    return 0;
}

QString Torrent::fileName(int index) const
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid() && h->torrent_file()) {
        const lt::file_storage& fs = h->torrent_file()->files();
        if(index >= 0 && index < fs.num_files()) {
            return QString::fromStdString(fs.file_path(lt::file_index_t{index}));
        }
    }
    return QString();
}

QStringList Torrent::fileNames() const
{
    QStringList names;
    int count = fileCount();
    for(int i = 0; i < count; ++i) {
        names.append(fileName(i));
    }
    return names;
}

void Torrent::setFilePriority(int index, int priority)
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        h->file_priority(lt::file_index_t{index}, lt::download_priority_t{static_cast<std::uint8_t>(priority)});
    }
}

void Torrent::setAllFilePriorities(int priority)
{
    int count = fileCount();
    for(int i = 0; i < count; ++i) {
        setFilePriority(i, priority);
    }
}

int Torrent::findFileByName(const QString& partialName) const
{
    QString lower = partialName.toLower();
    int count = fileCount();
    for(int i = 0; i < count; ++i) {
        if(fileName(i).toLower().contains(lower)) {
            return i;
        }
    }
    return -1;
}

QString Torrent::outputPath() const
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        lt::torrent_status status = h->status();
        return QString::fromStdString(status.save_path) + "/" + QString::fromStdString(status.name);
    }
    return QString();
}

void Torrent::setHandle(void* handle)
{
    _handle = handle;
}

void Torrent::updateFromStatus()
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(!h || !h->is_valid()) { return; }

    lt::torrent_status status = h->status();

    // Check metadata
    if(!_hasMetadata && status.has_metadata) {
        _hasMetadata = true;
        logText(LVL_INFO, QString("Metadata received: %1").arg(name()));
        emit metadataReceived();
    }

    // Map libtorrent state to our state
    State newState = _state;
    switch(status.state) {
    case lt::torrent_status::checking_files:
    case lt::torrent_status::checking_resume_data:
        newState = Checking;
        break;
    case lt::torrent_status::downloading_metadata:
        newState = FetchingMetadata;
        break;
    case lt::torrent_status::downloading:
        newState = Downloading;
        break;
    case lt::torrent_status::finished:
    case lt::torrent_status::seeding:
        newState = Seeding;
        break;
    default:
        break;
    }

    if((status.flags & lt::torrent_flags::paused) && newState != Idle) {
        newState = Paused;
    }

    setState(newState);

    // Emit progress
    if(_state == Downloading) {
        double prog = static_cast<double>(status.progress);
        logText(LVL_DEBUG, QString("%1: %2% ↓%3/s peers=%4")
            .arg(name())
            .arg(static_cast<int>(prog * 100))
            .arg(status.download_rate > 0
                ? QString::number(status.download_rate / 1024) + "KB"
                : "0")
            .arg(status.num_peers));
        emit progressUpdated(prog);
    }

    // Emit download complete (once)
    if(newState == Seeding && !_downloadEmitted) {
        _downloadEmitted = true;
        logText(LVL_INFO, QString("Download complete: %1").arg(name()));
        emit downloadComplete();
    }
}

void Torrent::setState(State newState)
{
    if(_state != newState) {
        _state = newState;
        emit stateChanged(_state);
    }
}
