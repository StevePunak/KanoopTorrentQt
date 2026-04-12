#include <Kanoop/torrent/torrent.h>

#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/torrent_flags.hpp>
#include <libtorrent/file_storage.hpp>
#include <libtorrent/announce_entry.hpp>

// ─── Construction / Destruction ──────────────────────────────────────────────

Torrent::Torrent(QObject* parent) :
    QObject(parent),
    LoggingBaseClass("torrent")
{
}

Torrent::~Torrent()
{
    // handle is owned by TorrentClient, deleted there
}

// ─── Control ─────────────────────────────────────────────────────────────────

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

// ─── Info ────────────────────────────────────────────────────────────────────

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

// ─── Download progress ──────────────────────────────────────────────────────

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

// ─── Upload stats ────────────────────────────────────────────────────────────

qint64 Torrent::uploadRate() const
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        return h->status().upload_rate;
    }
    return 0;
}

qint64 Torrent::totalUploaded() const
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        return h->status().all_time_upload;
    }
    return 0;
}

double Torrent::ratio() const
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        lt::torrent_status s = h->status();
        if(s.all_time_download > 0) {
            return static_cast<double>(s.all_time_upload) / static_cast<double>(s.all_time_download);
        }
    }
    return 0.0;
}

// ─── Piece info ──────────────────────────────────────────────────────────────

int Torrent::totalPieces() const
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid() && h->torrent_file()) {
        return h->torrent_file()->num_pieces();
    }
    return 0;
}

int Torrent::downloadedPieces() const
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        return h->status().num_pieces;
    }
    return 0;
}

int Torrent::pieceSize() const
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid() && h->torrent_file()) {
        return h->torrent_file()->piece_length();
    }
    return 0;
}

// ─── ETA ─────────────────────────────────────────────────────────────────────

int Torrent::eta() const
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(!h || !h->is_valid()) { return -1; }

    lt::torrent_status s = h->status();
    if(s.download_rate <= 0) { return -1; }

    qint64 remaining = s.total - s.total_done;
    if(remaining <= 0) { return 0; }

    return static_cast<int>(remaining / s.download_rate);
}

// ─── File management ─────────────────────────────────────────────────────────

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

qint64 Torrent::fileSize(int index) const
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid() && h->torrent_file()) {
        const lt::file_storage& fs = h->torrent_file()->files();
        if(index >= 0 && index < fs.num_files()) {
            return fs.file_size(lt::file_index_t{index});
        }
    }
    return 0;
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

// ─── Paths ───────────────────────────────────────────────────────────────────

QString Torrent::outputPath() const
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        lt::torrent_status status = h->status();
        return QString::fromStdString(status.save_path) + "/" + QString::fromStdString(status.name);
    }
    return QString();
}

void Torrent::moveStorage(const QString& newPath)
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        h->move_storage(newPath.toStdString());
        logText(LVL_INFO, QString("Moving storage to: %1").arg(newPath));
    }
}

// ─── Per-torrent options ─────────────────────────────────────────────────────

bool Torrent::isSequentialDownload() const
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        return static_cast<bool>(h->flags() & lt::torrent_flags::sequential_download);
    }
    return false;
}

void Torrent::setSequentialDownload(bool enabled)
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        if(enabled) {
            h->set_flags(lt::torrent_flags::sequential_download);
        }
        else {
            h->unset_flags(lt::torrent_flags::sequential_download);
        }
    }
}

bool Torrent::isAutoManaged() const
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        return static_cast<bool>(h->flags() & lt::torrent_flags::auto_managed);
    }
    return false;
}

void Torrent::setAutoManaged(bool enabled)
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        if(enabled) {
            h->set_flags(lt::torrent_flags::auto_managed);
        }
        else {
            h->unset_flags(lt::torrent_flags::auto_managed);
        }
    }
}

int Torrent::downloadLimit() const
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        return h->download_limit();
    }
    return 0;
}

void Torrent::setDownloadLimit(int bytesPerSecond)
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        h->set_download_limit(bytesPerSecond);
    }
}

int Torrent::uploadLimit() const
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        return h->upload_limit();
    }
    return 0;
}

void Torrent::setUploadLimit(int bytesPerSecond)
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        h->set_upload_limit(bytesPerSecond);
    }
}

int Torrent::maxConnections() const
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        return h->max_connections();
    }
    return 0;
}

void Torrent::setMaxConnections(int value)
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        h->set_max_connections(value);
    }
}

int Torrent::maxUploads() const
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        return h->max_uploads();
    }
    return 0;
}

void Torrent::setMaxUploads(int value)
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        h->set_max_uploads(value);
    }
}

// ─── Seed ratio ──────────────────────────────────────────────────────────────

void Torrent::setSeedRatioLimit(double ratio)
{
    _seedRatioLimit = ratio;
}

void Torrent::checkSeedRatio()
{
    if(_seedRatioLimit <= 0.0) { return; }
    if(_state != Seeding) { return; }

    double currentRatio = ratio();
    if(currentRatio >= _seedRatioLimit) {
        logText(LVL_INFO, QString("Seed ratio %.2f reached limit %.2f, pausing: %1")
            .arg(currentRatio).arg(_seedRatioLimit).arg(name()));
        pause();
    }
}

// ─── Tracker management ─────────────────────────────────────────────────────

QStringList Torrent::trackers() const
{
    QStringList result;
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        std::vector<lt::announce_entry> entries = h->trackers();
        for(const auto& entry : entries) {
            result.append(QString::fromStdString(entry.url));
        }
    }
    return result;
}

void Torrent::addTracker(const QString& url)
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        lt::announce_entry entry(url.toStdString());
        h->add_tracker(entry);
        logText(LVL_DEBUG, QString("Added tracker: %1").arg(url));
    }
}

void Torrent::removeTracker(const QString& url)
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(!h || !h->is_valid()) { return; }

    std::vector<lt::announce_entry> entries = h->trackers();
    std::string target = url.toStdString();
    std::vector<lt::announce_entry> filtered;
    for(const auto& entry : entries) {
        if(entry.url != target) {
            filtered.push_back(entry);
        }
    }
    h->replace_trackers(filtered);
    logText(LVL_DEBUG, QString("Removed tracker: %1").arg(url));
}

void Torrent::forceReannounce()
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        h->force_reannounce();
    }
}

// ─── Resume data ─────────────────────────────────────────────────────────────

void Torrent::saveResumeData()
{
    auto* h = static_cast<lt::torrent_handle*>(_handle);
    if(h && h->is_valid()) {
        h->save_resume_data(lt::torrent_handle::save_info_dict);
    }
}

// ─── Internal ────────────────────────────────────────────────────────────────

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
