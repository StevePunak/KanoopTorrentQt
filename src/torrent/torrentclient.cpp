#include <Kanoop/torrent/torrentclient.h>
#include <Kanoop/torrent/torrentsearchresult.h>

#include <libtorrent/session.hpp>
#include <libtorrent/session_params.hpp>
#include <libtorrent/session_stats.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/settings_pack.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/write_resume_data.hpp>

#include <QDir>
#include <QFile>
#include <QTimer>

#define LT_SESSION static_cast<lt::session*>(_session)

// ─── Construction / Destruction ──────────────────────────────────────────────

TorrentClient::TorrentClient(QObject* parent) :
    QObject(parent),
    LoggingBaseClass("torrent-client")
{
    lt::settings_pack pack;
    pack.set_str(lt::settings_pack::user_agent, "KanoopTorrentQt/1.0");
    pack.set_int(lt::settings_pack::alert_mask,
        lt::alert_category::status | lt::alert_category::error |
        lt::alert_category::storage | lt::alert_category::peer |
        lt::alert_category::tracker | lt::alert_category::dht |
        lt::alert_category::port_mapping | lt::alert_category::stats);
    pack.set_bool(lt::settings_pack::enable_dht, true);
    pack.set_bool(lt::settings_pack::enable_lsd, true);

    _session = new lt::session(std::move(pack));

    _alertTimer = new QTimer(this);
    _alertTimer->setInterval(500);
    connect(_alertTimer, &QTimer::timeout, this, &TorrentClient::processAlerts);
    _alertTimer->start();

    logText(LVL_INFO, "libtorrent session created");
}

TorrentClient::~TorrentClient()
{
    _alertTimer->stop();
    stopAll();
    delete LT_SESSION;
}

// ─── Proxy ───────────────────────────────────────────────────────────────────

void TorrentClient::setNetworkProxy(const QNetworkProxy& value)
{
    _networkProxy = value;
    configureProxy();
}

void TorrentClient::configureProxy()
{
    if(!_session) { return; }

    lt::settings_pack pack;

    if(_networkProxy.type() == QNetworkProxy::Socks5Proxy) {
        if(!_networkProxy.user().isEmpty()) {
            pack.set_int(lt::settings_pack::proxy_type, lt::settings_pack::socks5_pw);
        }
        else {
            pack.set_int(lt::settings_pack::proxy_type, lt::settings_pack::socks5);
        }
        pack.set_str(lt::settings_pack::proxy_hostname,
                     _networkProxy.hostName().toStdString());
        pack.set_int(lt::settings_pack::proxy_port, _networkProxy.port());
        pack.set_str(lt::settings_pack::proxy_username,
                     _networkProxy.user().toStdString());
        pack.set_str(lt::settings_pack::proxy_password,
                     _networkProxy.password().toStdString());
        pack.set_bool(lt::settings_pack::proxy_peer_connections, true);
        pack.set_bool(lt::settings_pack::proxy_hostnames, true);

        logText(LVL_INFO, QString("SOCKS5 proxy configured: %1:%2 user='%3'")
            .arg(_networkProxy.hostName())
            .arg(_networkProxy.port())
            .arg(_networkProxy.user()));
    }
    else {
        pack.set_int(lt::settings_pack::proxy_type, lt::settings_pack::none);
    }

    LT_SESSION->apply_settings(std::move(pack));
}

// ─── Settings helpers ────────────────────────────────────────────────────────

void TorrentClient::applySetting(int name, int value)
{
    lt::settings_pack pack;
    pack.set_int(name, value);
    LT_SESSION->apply_settings(std::move(pack));
}

void TorrentClient::applySetting(int name, bool value)
{
    lt::settings_pack pack;
    pack.set_bool(name, value);
    LT_SESSION->apply_settings(std::move(pack));
}

void TorrentClient::applySetting(int name, const std::string& value)
{
    lt::settings_pack pack;
    pack.set_str(name, value);
    LT_SESSION->apply_settings(std::move(pack));
}

int TorrentClient::getSettingInt(int name) const
{
    return LT_SESSION->get_settings().get_int(name);
}

bool TorrentClient::getSettingBool(int name) const
{
    return LT_SESSION->get_settings().get_bool(name);
}

std::string TorrentClient::getSettingString(int name) const
{
    return LT_SESSION->get_settings().get_str(name);
}

// ─── Bandwidth limits ────────────────────────────────────────────────────────

int TorrentClient::downloadRateLimit() const
{
    return getSettingInt(lt::settings_pack::download_rate_limit);
}

void TorrentClient::setDownloadRateLimit(int bytesPerSecond)
{
    applySetting(lt::settings_pack::download_rate_limit, bytesPerSecond);
    logText(LVL_INFO, QString("Download rate limit: %1 bytes/s").arg(bytesPerSecond));
}

int TorrentClient::uploadRateLimit() const
{
    return getSettingInt(lt::settings_pack::upload_rate_limit);
}

void TorrentClient::setUploadRateLimit(int bytesPerSecond)
{
    applySetting(lt::settings_pack::upload_rate_limit, bytesPerSecond);
    logText(LVL_INFO, QString("Upload rate limit: %1 bytes/s").arg(bytesPerSecond));
}

// ─── Connection limits ───────────────────────────────────────────────────────

int TorrentClient::maxConnections() const
{
    return getSettingInt(lt::settings_pack::connections_limit);
}

void TorrentClient::setMaxConnections(int value)
{
    applySetting(lt::settings_pack::connections_limit, value);
}

int TorrentClient::maxUploads() const
{
    return getSettingInt(lt::settings_pack::unchoke_slots_limit);
}

void TorrentClient::setMaxUploads(int value)
{
    applySetting(lt::settings_pack::unchoke_slots_limit, value);
}

// ─── Listen interface ────────────────────────────────────────────────────────

QString TorrentClient::listenInterfaces() const
{
    return QString::fromStdString(getSettingString(lt::settings_pack::listen_interfaces));
}

void TorrentClient::setListenInterfaces(const QString& value)
{
    applySetting(lt::settings_pack::listen_interfaces, value.toStdString());
    logText(LVL_INFO, QString("Listen interfaces: %1").arg(value));
}

// ─── Protocol settings ──────────────────────────────────────────────────────

bool TorrentClient::isDhtEnabled() const
{
    return getSettingBool(lt::settings_pack::enable_dht);
}

void TorrentClient::setDhtEnabled(bool enabled)
{
    applySetting(lt::settings_pack::enable_dht, enabled);
    logText(LVL_INFO, QString("DHT %1").arg(enabled ? "enabled" : "disabled"));
}

bool TorrentClient::isLsdEnabled() const
{
    return getSettingBool(lt::settings_pack::enable_lsd);
}

void TorrentClient::setLsdEnabled(bool enabled)
{
    applySetting(lt::settings_pack::enable_lsd, enabled);
}

// ─── Encryption ──────────────────────────────────────────────────────────────

TorrentClient::EncryptionMode TorrentClient::encryptionMode() const
{
    int inEnc = getSettingInt(lt::settings_pack::in_enc_policy);
    if(inEnc == lt::settings_pack::pe_forced) {
        return EncryptionForced;
    }
    if(inEnc == lt::settings_pack::pe_disabled) {
        return EncryptionDisabled;
    }
    return EncryptionEnabled;
}

void TorrentClient::setEncryptionMode(EncryptionMode mode)
{
    lt::settings_pack pack;
    switch(mode) {
    case EncryptionForced:
        pack.set_int(lt::settings_pack::in_enc_policy, lt::settings_pack::pe_forced);
        pack.set_int(lt::settings_pack::out_enc_policy, lt::settings_pack::pe_forced);
        break;
    case EncryptionDisabled:
        pack.set_int(lt::settings_pack::in_enc_policy, lt::settings_pack::pe_disabled);
        pack.set_int(lt::settings_pack::out_enc_policy, lt::settings_pack::pe_disabled);
        break;
    case EncryptionEnabled:
    default:
        pack.set_int(lt::settings_pack::in_enc_policy, lt::settings_pack::pe_enabled);
        pack.set_int(lt::settings_pack::out_enc_policy, lt::settings_pack::pe_enabled);
        break;
    }
    LT_SESSION->apply_settings(std::move(pack));
    logText(LVL_INFO, QString("Encryption mode: %1").arg(QMetaEnum::fromType<EncryptionMode>().valueToKey(mode)));
}

// ─── User agent ──────────────────────────────────────────────────────────────

QString TorrentClient::userAgent() const
{
    return QString::fromStdString(getSettingString(lt::settings_pack::user_agent));
}

void TorrentClient::setUserAgent(const QString& value)
{
    applySetting(lt::settings_pack::user_agent, value.toStdString());
}

// ─── Session statistics ──────────────────────────────────────────────────────

SessionStats TorrentClient::sessionStats() const
{
    SessionStats stats;

    qint64 totalDown = 0;
    qint64 totalUp   = 0;
    qint64 downRate  = 0;
    qint64 upRate    = 0;
    int totalPeers   = 0;
    int active       = 0;
    int paused       = 0;

    for(Torrent* t : _torrents) {
        auto* h = static_cast<lt::torrent_handle*>(t->handle());
        if(!h || !h->is_valid()) { continue; }

        lt::torrent_status s = h->status();
        totalDown += s.all_time_download;
        totalUp   += s.all_time_upload;
        downRate  += s.download_rate;
        upRate    += s.upload_rate;
        totalPeers += s.num_peers;

        if(s.flags & lt::torrent_flags::paused) {
            paused++;
        }
        else {
            active++;
        }
    }

    stats.setTotalDownloaded(totalDown);
    stats.setTotalUploaded(totalUp);
    stats.setDownloadRate(downRate);
    stats.setUploadRate(upRate);
    stats.setTotalPeers(totalPeers);
    stats.setActiveTorrents(active);
    stats.setPausedTorrents(paused);

    // DHT node count from session status
    lt::session_params params = LT_SESSION->session_state();
    stats.setDhtNodes(static_cast<int>(params.dht_state.nodes.size() +
                                        params.dht_state.nodes6.size()));

    return stats;
}

void TorrentClient::requestSessionStats()
{
    LT_SESSION->post_session_stats();
}

// ─── Resume data ─────────────────────────────────────────────────────────────

void TorrentClient::setResumeDataDirectory(const QString& path)
{
    _resumeDataDirectory = path;
    QDir().mkpath(path);
}

void TorrentClient::saveAllResumeData()
{
    for(Torrent* t : _torrents) {
        t->saveResumeData();
    }
}

void TorrentClient::loadResumeData()
{
    if(_resumeDataDirectory.isEmpty()) {
        logText(LVL_WARNING, "No resume data directory set");
        return;
    }

    QDir dir(_resumeDataDirectory);
    QStringList files = dir.entryList(QStringList() << "*.fastresume", QDir::Files);

    for(const QString& fileName : files) {
        QString filePath = dir.filePath(fileName);
        QFile file(filePath);
        if(!file.open(QIODevice::ReadOnly)) {
            logText(LVL_WARNING, QString("Cannot open resume file: %1").arg(filePath));
            continue;
        }

        QByteArray data = file.readAll();
        file.close();

        lt::error_code ec;
        lt::add_torrent_params params = lt::read_resume_data(
            lt::span<const char>(data.constData(), data.size()), ec);
        if(ec) {
            logText(LVL_WARNING, QString("Failed to parse resume data: %1 — %2")
                .arg(filePath, QString::fromStdString(ec.message())));
            continue;
        }

        auto hash = params.info_hashes.get_best();
        QByteArray infoHash(reinterpret_cast<const char*>(hash.data()), 20);

        if(findTorrent(infoHash)) {
            logText(LVL_DEBUG, QString("Skipping duplicate resume: %1").arg(fileName));
            continue;
        }

        lt::torrent_handle handle = LT_SESSION->add_torrent(params);

        Torrent* torrent = new Torrent(this);
        torrent->setInfoHash(infoHash);
        torrent->setDownloadDirectory(QString::fromStdString(params.save_path));
        torrent->setHandle(new lt::torrent_handle(handle));

        _torrents.append(torrent);
        logText(LVL_INFO, QString("Resumed torrent: %1").arg(torrent->name()));
        emit torrentAdded(torrent);
    }

    logText(LVL_INFO, QString("Loaded %1 resume file(s)").arg(files.size()));
}

// ─── Add torrents ────────────────────────────────────────────────────────────

Torrent* TorrentClient::addTorrent(const MagnetLink& magnetLink, const QString& downloadDirectory)
{
    if(findTorrent(magnetLink.infoHash())) {
        logText(LVL_WARNING, QString("Torrent already exists: %1").arg(magnetLink.infoHashHex()));
        return nullptr;
    }

    QString dir = downloadDirectory.isEmpty() ? _defaultDownloadDirectory : downloadDirectory;
    QDir().mkpath(dir);

    std::string uri = magnetLink.toUri().toStdString();
    logText(LVL_DEBUG, QString("Magnet URI: %1").arg(QString::fromStdString(uri)));

    lt::add_torrent_params params = lt::parse_magnet_uri(uri);
    params.save_path = dir.toStdString();

    lt::torrent_handle handle = LT_SESSION->add_torrent(params);

    Torrent* torrent = new Torrent(this);
    torrent->setMagnetLink(magnetLink);
    torrent->setInfoHash(magnetLink.infoHash());
    torrent->setDownloadDirectory(dir);
    torrent->setHandle(new lt::torrent_handle(handle));

    _torrents.append(torrent);

    logText(LVL_INFO, QString("Added torrent: %1").arg(torrent->name()));
    emit torrentAdded(torrent);
    return torrent;
}

Torrent* TorrentClient::addTorrent(const QString& torrentFilePath, const QString& downloadDirectory)
{
    QFile file(torrentFilePath);
    if(!file.open(QIODevice::ReadOnly)) {
        logText(LVL_ERROR, QString("Cannot open torrent file: %1").arg(torrentFilePath));
        return nullptr;
    }
    return addTorrent(file.readAll(), downloadDirectory);
}

Torrent* TorrentClient::addTorrent(const QByteArray& torrentData, const QString& downloadDirectory)
{
    lt::error_code ec;
    lt::torrent_info ti(torrentData.constData(), static_cast<int>(torrentData.size()), ec);
    if(ec) {
        logText(LVL_ERROR, QString("Failed to parse torrent: %1")
            .arg(QString::fromStdString(ec.message())));
        return nullptr;
    }

    auto hash = ti.info_hashes().get_best();
    QByteArray infoHash(reinterpret_cast<const char*>(hash.data()), 20);

    if(findTorrent(infoHash)) {
        logText(LVL_WARNING, "Torrent already exists");
        return nullptr;
    }

    QString dir = downloadDirectory.isEmpty() ? _defaultDownloadDirectory : downloadDirectory;
    QDir().mkpath(dir);

    lt::add_torrent_params params;
    params.ti = std::make_shared<lt::torrent_info>(ti);
    params.save_path = dir.toStdString();

    lt::torrent_handle handle = LT_SESSION->add_torrent(params);

    Torrent* torrent = new Torrent(this);
    torrent->setInfoHash(infoHash);
    torrent->setDownloadDirectory(dir);
    torrent->setHandle(new lt::torrent_handle(handle));

    _torrents.append(torrent);

    logText(LVL_INFO, QString("Added torrent: %1 (%2)")
        .arg(torrent->name())
        .arg(TorrentSearchResult::formatSize(ti.total_size())));
    emit torrentAdded(torrent);
    return torrent;
}

// ─── Remove torrents ─────────────────────────────────────────────────────────

void TorrentClient::removeTorrent(Torrent* torrent, bool deleteFiles)
{
    if(!torrent || !_torrents.contains(torrent)) { return; }

    QByteArray hash = torrent->infoHash();

    auto* handle = static_cast<lt::torrent_handle*>(torrent->handle());
    if(handle && handle->is_valid()) {
        LT_SESSION->remove_torrent(*handle,
            deleteFiles ? lt::session::delete_files : lt::remove_flags_t{});
    }

    // Remove resume data file if it exists
    if(!_resumeDataDirectory.isEmpty()) {
        QString resumePath = _resumeDataDirectory + "/" + torrent->infoHashHex() + ".fastresume";
        QFile::remove(resumePath);
    }

    _torrents.removeOne(torrent);
    delete static_cast<lt::torrent_handle*>(torrent->handle());
    torrent->deleteLater();

    logText(LVL_INFO, QString("Removed torrent: %1").arg(QString::fromLatin1(hash.toHex()).left(16)));
    emit torrentRemoved(hash);
}

// ─── Query ───────────────────────────────────────────────────────────────────

Torrent* TorrentClient::findTorrent(const QByteArray& infoHash) const
{
    for(Torrent* t : _torrents) {
        if(t->infoHash() == infoHash) { return t; }
    }
    return nullptr;
}

Torrent* TorrentClient::findTorrentByHandle(void* alertHandle)
{
    if(!alertHandle) { return nullptr; }
    for(Torrent* t : _torrents) {
        auto* h = static_cast<lt::torrent_handle*>(t->handle());
        if(h && *h == *static_cast<lt::torrent_handle*>(alertHandle)) {
            return t;
        }
    }
    return nullptr;
}

// ─── Global controls ─────────────────────────────────────────────────────────

void TorrentClient::startAll()
{
    for(Torrent* t : _torrents) { t->start(); }
}

void TorrentClient::stopAll()
{
    for(Torrent* t : _torrents) { t->stop(); }
}

// ─── Alert processing ────────────────────────────────────────────────────────

void TorrentClient::processAlerts()
{
    if(!_session) { return; }

    std::vector<lt::alert*> alerts;
    LT_SESSION->pop_alerts(&alerts);

    for(lt::alert* a : alerts) {
        logText(LVL_DEBUG, QString("lt alert: %1").arg(QString::fromStdString(a->message())));

        // --- Torrent error ---
        if(auto* ta = lt::alert_cast<lt::torrent_error_alert>(a)) {
            lt::torrent_handle h = ta->handle;
            Torrent* t = findTorrentByHandle(&h);
            if(t) {
                logText(LVL_WARNING, QString("Torrent error: %1 — %2")
                    .arg(t->name(), QString::fromStdString(ta->message())));
                emit t->error(QString::fromStdString(ta->message()));
            }
        }

        // --- File completed ---
        else if(auto* fa = lt::alert_cast<lt::file_completed_alert>(a)) {
            lt::torrent_handle h = fa->handle;
            Torrent* t = findTorrentByHandle(&h);
            if(t) {
                int idx = static_cast<int>(fa->index);
                QString filePath = t->fileName(idx);
                logText(LVL_INFO, QString("File completed: %1 [%2]")
                    .arg(filePath).arg(idx));
                emit t->fileCompleted(idx, filePath);
            }
        }

        // --- Tracker announce succeeded ---
        else if(auto* tra = lt::alert_cast<lt::tracker_reply_alert>(a)) {
            lt::torrent_handle h = tra->handle;
            Torrent* t = findTorrentByHandle(&h);
            if(t) {
                emit t->trackerAnnounced(QString::fromStdString(std::string(tra->tracker_url())));
            }
        }

        // --- Tracker error ---
        else if(auto* tre = lt::alert_cast<lt::tracker_error_alert>(a)) {
            lt::torrent_handle h = tre->handle;
            Torrent* t = findTorrentByHandle(&h);
            if(t) {
                QString url = QString::fromStdString(std::string(tre->tracker_url()));
                QString msg = QString::fromStdString(tre->message());
                logText(LVL_WARNING, QString("Tracker error: %1 — %2").arg(url, msg));
                emit t->trackerError(url, msg);
            }
        }

        // --- Metadata failed ---
        else if(auto* mfa = lt::alert_cast<lt::metadata_failed_alert>(a)) {
            lt::torrent_handle h = mfa->handle;
            Torrent* t = findTorrentByHandle(&h);
            if(t) {
                logText(LVL_WARNING, QString("Metadata fetch failed: %1").arg(t->name()));
                emit t->error(QString("Metadata fetch failed: %1")
                    .arg(QString::fromStdString(mfa->message())));
            }
        }

        // --- Storage moved ---
        else if(auto* sma = lt::alert_cast<lt::storage_moved_alert>(a)) {
            lt::torrent_handle h = sma->handle;
            Torrent* t = findTorrentByHandle(&h);
            if(t) {
                QString newPath = QString::fromStdString(std::string(sma->storage_path()));
                t->setDownloadDirectory(newPath);
                logText(LVL_INFO, QString("Storage moved: %1 → %2").arg(t->name(), newPath));
                emit t->storageMoved(newPath);
            }
        }

        // --- Storage move failed ---
        else if(auto* smf = lt::alert_cast<lt::storage_moved_failed_alert>(a)) {
            lt::torrent_handle h = smf->handle;
            Torrent* t = findTorrentByHandle(&h);
            if(t) {
                QString msg = QString::fromStdString(smf->message());
                logText(LVL_WARNING, QString("Storage move failed: %1 — %2").arg(t->name(), msg));
                emit t->storageMoveError(msg);
            }
        }

        // --- Listen succeeded ---
        else if(auto* lsa = lt::alert_cast<lt::listen_succeeded_alert>(a)) {
            QString addr = QString::fromStdString(lsa->address.to_string());
            int port = lsa->port;
            logText(LVL_INFO, QString("Listen succeeded: %1:%2").arg(addr).arg(port));
            emit listenSucceeded(addr, port);
        }

        // --- Listen failed ---
        else if(auto* lfa = lt::alert_cast<lt::listen_failed_alert>(a)) {
            QString addr = QString::fromStdString(lfa->address.to_string());
            int port = lfa->port;
            QString err = QString::fromStdString(lfa->error.message());
            logText(LVL_WARNING, QString("Listen failed: %1:%2 — %3").arg(addr).arg(port).arg(err));
            emit listenFailed(addr, port, err);
        }

        // --- External IP detected ---
        else if(auto* eip = lt::alert_cast<lt::external_ip_alert>(a)) {
            QString addr = QString::fromStdString(eip->external_address.to_string());
            logText(LVL_INFO, QString("External address detected: %1").arg(addr));
            emit externalAddressDetected(addr);
        }

        // --- DHT bootstrap ---
        else if(lt::alert_cast<lt::dht_bootstrap_alert>(a)) {
            logText(LVL_INFO, "DHT bootstrap complete");
            emit dhtBootstrapComplete();
        }

        // --- Session stats ---
        else if(lt::alert_cast<lt::session_stats_alert>(a)) {
            emit sessionStatsReceived(sessionStats());
        }

        // --- Save resume data ---
        else if(auto* srd = lt::alert_cast<lt::save_resume_data_alert>(a)) {
            lt::torrent_handle h = srd->handle;
            Torrent* t = findTorrentByHandle(&h);
            if(t && !_resumeDataDirectory.isEmpty()) {
                std::vector<char> buf = lt::write_resume_data_buf(srd->params);
                QString path = _resumeDataDirectory + "/" + t->infoHashHex() + ".fastresume";
                QFile file(path);
                if(file.open(QIODevice::WriteOnly)) {
                    file.write(buf.data(), static_cast<qint64>(buf.size()));
                    file.close();
                    logText(LVL_DEBUG, QString("Resume data saved: %1").arg(t->name()));
                    emit resumeDataSaved(t->infoHash());
                }
                else {
                    logText(LVL_WARNING, QString("Failed to write resume data: %1").arg(path));
                    emit resumeDataFailed(t->infoHash(), QString("Cannot write file: %1").arg(path));
                }
            }
        }

        // --- Save resume data failed ---
        else if(auto* srf = lt::alert_cast<lt::save_resume_data_failed_alert>(a)) {
            lt::torrent_handle h = srf->handle;
            Torrent* t = findTorrentByHandle(&h);
            if(t) {
                QString msg = QString::fromStdString(srf->message());
                logText(LVL_WARNING, QString("Resume data save failed: %1 — %2").arg(t->name(), msg));
                emit resumeDataFailed(t->infoHash(), msg);
            }
        }
    }

    // Update all torrent states
    for(Torrent* t : _torrents) {
        Torrent::State oldState = t->state();
        t->updateFromStatus();
        if(t->state() != oldState) {
            emit torrentStateChanged(t, t->state());
        }
        if(t->state() == Torrent::Seeding && oldState != Torrent::Seeding) {
            emit torrentComplete(t);
        }
        t->checkSeedRatio();
    }
}
