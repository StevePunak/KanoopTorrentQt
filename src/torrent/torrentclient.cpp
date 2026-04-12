#include <Kanoop/torrent/torrentclient.h>
#include <Kanoop/torrent/torrentsearchresult.h>

#include <libtorrent/session.hpp>
#include <libtorrent/session_params.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/settings_pack.hpp>

#include <QDir>
#include <QFile>
#include <QTimer>

#define LT_SESSION static_cast<lt::session*>(_session)

TorrentClient::TorrentClient(QObject* parent) :
    QObject(parent),
    LoggingBaseClass("torrent-client")
{
    lt::settings_pack pack;
    pack.set_str(lt::settings_pack::user_agent, "Kiosk/1.0");
    pack.set_int(lt::settings_pack::alert_mask,
        lt::alert_category::status | lt::alert_category::error |
        lt::alert_category::storage | lt::alert_category::peer |
        lt::alert_category::tracker | lt::alert_category::dht);
    pack.set_bool(lt::settings_pack::enable_dht, true);
    pack.set_bool(lt::settings_pack::enable_lsd, true);

    _session = new lt::session(std::move(pack));

    // Poll for libtorrent alerts every 500ms
    _alertTimer = new QTimer(this);
    _alertTimer->setInterval(500);
    connect(_alertTimer, &QTimer::timeout, this, &TorrentClient::processAlerts);
    _alertTimer->start();

    logText(LVL_INFO, "libtorrent session created (DHT + LSD enabled)");
}

TorrentClient::~TorrentClient()
{
    _alertTimer->stop();
    stopAll();
    delete LT_SESSION;
}

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

void TorrentClient::removeTorrent(Torrent* torrent, bool deleteFiles)
{
    if(!torrent || !_torrents.contains(torrent)) { return; }

    QByteArray hash = torrent->infoHash();

    auto* handle = static_cast<lt::torrent_handle*>(torrent->handle());
    if(handle && handle->is_valid()) {
        LT_SESSION->remove_torrent(*handle,
            deleteFiles ? lt::session::delete_files : lt::remove_flags_t{});
    }

    _torrents.removeOne(torrent);
    delete static_cast<lt::torrent_handle*>(torrent->handle());
    torrent->deleteLater();

    logText(LVL_INFO, QString("Removed torrent: %1").arg(QString::fromLatin1(hash.toHex()).left(16)));
    emit torrentRemoved(hash);
}

Torrent* TorrentClient::findTorrent(const QByteArray& infoHash) const
{
    for(Torrent* t : _torrents) {
        if(t->infoHash() == infoHash) { return t; }
    }
    return nullptr;
}

void TorrentClient::startAll()
{
    for(Torrent* t : _torrents) { t->start(); }
}

void TorrentClient::stopAll()
{
    for(Torrent* t : _torrents) { t->stop(); }
}

void TorrentClient::processAlerts()
{
    if(!_session) { return; }

    std::vector<lt::alert*> alerts;
    LT_SESSION->pop_alerts(&alerts);

    for(lt::alert* a : alerts) {
        logText(LVL_DEBUG, QString("lt alert: %1").arg(QString::fromStdString(a->message())));

        if(auto* ta = lt::alert_cast<lt::torrent_error_alert>(a)) {
            for(Torrent* t : _torrents) {
                auto* h = static_cast<lt::torrent_handle*>(t->handle());
                if(h && *h == ta->handle) {
                    logText(LVL_WARNING, QString("Torrent error: %1 — %2")
                        .arg(t->name(), QString::fromStdString(ta->message())));
                    emit t->error(QString::fromStdString(ta->message()));
                }
            }
        }

        if(auto* fa = lt::alert_cast<lt::file_completed_alert>(a)) {
            for(Torrent* t : _torrents) {
                auto* h = static_cast<lt::torrent_handle*>(t->handle());
                if(h && *h == fa->handle) {
                    int idx = static_cast<int>(fa->index);
                    QString filePath = t->fileName(idx);
                    logText(LVL_INFO, QString("File completed: %1 [%2]")
                        .arg(filePath).arg(idx));
                    emit t->fileCompleted(idx, filePath);
                }
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
    }
}
