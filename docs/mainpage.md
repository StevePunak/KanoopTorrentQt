# KanoopTorrentQt {#mainpage}

A Qt6 wrapper library for libtorrent-rasterbar providing a clean, signal/slot-based API for BitTorrent operations.

## Core Classes

| Class | Description |
|-------|-------------|
| **TorrentClient** | Session manager: add/remove torrents, bandwidth and connection limits, proxy, encryption, DHT/LSD, IP filtering, resume data persistence |
| **Torrent** | Single torrent handle: control, progress, file management, peer info, tracker management, per-torrent options |
| **TorrentCreator** | Create `.torrent` metainfo files from local content |
| **TorrentSearcher** | Asynchronous torrent search against public APIs |
| **MagnetLink** | Parse and construct magnet URIs (hex and Base32 info hashes) |

## Value Classes

| Class | Description |
|-------|-------------|
| **SessionStats** | Aggregate session statistics snapshot (transfer totals, rates, DHT nodes) |
| **PeerInfo** | Connected peer snapshot (address, client, rates, progress, flags) |
| **TorrentSearchResult** | Search result metadata (name, size, seeders, leechers) |

## Quick Start

```cpp
#include <Kanoop/torrent/torrentclient.h>
#include <Kanoop/torrent/magnetlink.h>

auto* client = new TorrentClient(this);
client->setDefaultDownloadDirectory("/tmp/downloads");
client->setResumeDataDirectory("/tmp/resume");
client->setDownloadRateLimit(5 * 1024 * 1024); // 5 MB/s
client->loadResumeData(); // restore previous session

MagnetLink link("magnet:?xt=urn:btih:...");
Torrent* t = client->addTorrent(link);
t->setSequentialDownload(true);

connect(t, &Torrent::progressUpdated, [](double ratio) {
    qDebug() << "Progress:" << int(ratio * 100) << "%";
});
connect(t, &Torrent::downloadComplete, []() {
    qDebug() << "Done!";
});
```

## Creating Torrents

```cpp
#include <Kanoop/torrent/torrentcreator.h>

TorrentCreator creator;
creator.setTrackers({"udp://tracker.example.com:6969/announce"});
creator.setComment("My content");
creator.setPrivate(true);

if (creator.create("/path/to/content", "/tmp/output.torrent")) {
    qDebug() << creator.fileCount() << "files," << creator.pieceCount() << "pieces";
}
```

## Links

- [GitHub Repository](https://github.com/StevePunak/KanoopTorrentQt)
- [Class List](annotated.html)
- [Class Hierarchy](hierarchy.html)
- [File List](files.html)
