# KanoopTorrentQt

Qt6 wrapper library for [libtorrent-rasterbar](https://libtorrent.org/), providing a clean signal/slot-based API for BitTorrent operations. Hides all libtorrent headers behind opaque pointers so consumers only need Qt.

**[API Documentation](https://StevePunak.github.io/KanoopTorrentQt/)** | [Class List](https://StevePunak.github.io/KanoopTorrentQt/annotated.html) | [Class Hierarchy](https://StevePunak.github.io/KanoopTorrentQt/hierarchy.html)

[![CI](https://github.com/StevePunak/KanoopTorrentQt/actions/workflows/ci.yaml/badge.svg)](https://github.com/StevePunak/KanoopTorrentQt/actions/workflows/ci.yaml)

## Features

- **Session management** -- bandwidth/connection limits, encryption, DHT/LSD, SOCKS5 proxy, listen interfaces
- **Torrent lifecycle** -- add from magnet links, `.torrent` files, or raw bytes; pause/resume/remove
- **Progress tracking** -- download/upload rates, ETA, piece counts, per-file progress
- **Peer management** -- peer list with client identification, flags, rates; IP banning
- **Tracker management** -- add/remove trackers, force re-announce
- **File management** -- per-file priorities, rename, move storage, file progress
- **Resume data** -- save/load `.fastresume` files for session persistence
- **Torrent creation** -- create `.torrent` files from local content with progress reporting
- **IP filtering** -- block IP ranges, load P2P-format blocklists
- **Torrent search** -- async search against public torrent APIs
- **Session statistics** -- aggregate transfer totals, rates, DHT node count
- **Seed ratio limits** -- auto-pause torrents when a target share ratio is reached

## Requirements

- C++17
- Qt 6.7.0+ (Core, Network)
- libtorrent-rasterbar 2.0+
- CMake 3.16+
- [KanoopCommonQt](https://github.com/StevePunak/KanoopCommonQt)
- [KanoopProtocolQt](https://github.com/StevePunak/KanoopProtocolQt)

## Building

### Standalone

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
cmake --build build --parallel
```

### As part of a superproject

```cmake
add_subdirectory(KanoopTorrentQt)
target_link_libraries(myapp PRIVATE KanoopTorrentQt)
```

## Quick Start

```cpp
#include <Kanoop/torrent/torrentclient.h>
#include <Kanoop/torrent/magnetlink.h>

// Create a session
auto* client = new TorrentClient(this);
client->setDefaultDownloadDirectory("/tmp/downloads");
client->setDownloadRateLimit(5 * 1024 * 1024);  // 5 MB/s

// Restore previous session
client->setResumeDataDirectory("/tmp/resume");
client->loadResumeData();

// Add a torrent
MagnetLink link("magnet:?xt=urn:btih:...");
Torrent* t = client->addTorrent(link);
t->setSequentialDownload(true);

connect(t, &Torrent::progressUpdated, [](double ratio) {
    qDebug() << int(ratio * 100) << "%";
});
connect(t, &Torrent::downloadComplete, [] {
    qDebug() << "Download finished!";
});

// Save state before exit
client->saveAllResumeData();
```

## Creating Torrents

```cpp
#include <Kanoop/torrent/torrentcreator.h>

TorrentCreator creator;
creator.setTrackers({"udp://tracker.example.com:6969/announce"});
creator.setComment("My album");
creator.setPrivate(true);

if (creator.create("/path/to/content", "/tmp/output.torrent")) {
    qDebug() << creator.fileCount() << "files,"
             << creator.pieceCount() << "pieces";
}
```

## Classes

| Class | Description |
|-------|-------------|
| **TorrentClient** | Session manager: owns the libtorrent session and all Torrent objects |
| **Torrent** | Single torrent: control, progress, files, peers, trackers, options |
| **TorrentCreator** | Create `.torrent` metainfo files from local content |
| **TorrentSearcher** | Async search against public torrent APIs |
| **MagnetLink** | Parse and construct magnet URIs |
| **SessionStats** | Aggregate session statistics snapshot |
| **PeerInfo** | Connected peer state snapshot |
| **TorrentSearchResult** | Search result metadata |

## API Documentation

Full Doxygen documentation is generated automatically on every CI build and published to GitHub Pages:

**https://StevePunak.github.io/KanoopTorrentQt/**

To generate locally:

```bash
# Via CMake (requires Doxygen + Graphviz)
cmake --build build --target doc_KanoopTorrentQt

# Or directly
doxygen Doxyfile
```

Open `docs/html/index.html` in a browser to browse the local copy.

## Project Structure

```
KanoopTorrentQt/
  include/Kanoop/torrent/   Public headers
  src/torrent/              Implementation files
  docs/                     Generated documentation (not committed)
  Doxyfile                  Doxygen configuration
  CMakeLists.txt            Build configuration
  .github/workflows/        CI pipeline
```

## License

[MIT](LICENSE)
