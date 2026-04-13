#include <Kanoop/torrent/torrentcreator.h>

#include <libtorrent/create_torrent.hpp>
#include <libtorrent/file_storage.hpp>
#include <libtorrent/torrent_info.hpp>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>

TorrentCreator::TorrentCreator(QObject* parent) :
    QObject(parent),
    LoggingBaseClass("torrent-creator")
{
}

void TorrentCreator::setPieceSize(int bytes)
{
    _pieceSize = bytes;
}

void TorrentCreator::setTrackers(const QStringList& urls)
{
    _trackers = urls;
}

void TorrentCreator::addTrackerTier(const QStringList& urls)
{
    _trackerTiers.append(urls);
}

void TorrentCreator::setWebSeeds(const QStringList& urls)
{
    _webSeeds = urls;
}

void TorrentCreator::setComment(const QString& value)
{
    _comment = value;
}

void TorrentCreator::setCreator(const QString& value)
{
    _creator = value;
}

void TorrentCreator::setPrivate(bool enabled)
{
    _private = enabled;
}

bool TorrentCreator::create(const QString& sourcePath, const QString& outputPath)
{
    _errorString.clear();
    _totalSize = 0;
    _fileCount = 0;
    _pieceCount = 0;

    QFileInfo sourceInfo(sourcePath);
    if(!sourceInfo.exists()) {
        _errorString = QString("Source path does not exist: %1").arg(sourcePath);
        logText(LVL_ERROR, _errorString);
        return false;
    }

    lt::file_storage fs;

    if(sourceInfo.isDir()) {
        // Add all files in the directory recursively
        QString baseName = sourceInfo.fileName();
        QDir dir(sourcePath);
        QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

        // Use recursive helper via QDirIterator
        QDirIterator it(sourcePath, QDir::Files, QDirIterator::Subdirectories);
        while(it.hasNext()) {
            it.next();
            QString fullPath = it.filePath();
            QString relativePath = baseName + "/" + dir.relativeFilePath(fullPath);
            qint64 size = it.fileInfo().size();
            fs.add_file(relativePath.toStdString(), size);
        }
    }
    else {
        // Single file
        fs.add_file(sourceInfo.fileName().toStdString(), sourceInfo.size());
    }

    if(fs.num_files() == 0) {
        _errorString = "No files found in source path";
        logText(LVL_ERROR, _errorString);
        return false;
    }

    lt::create_torrent ct(fs, _pieceSize > 0 ? _pieceSize : 0);

    // Add trackers — each individual URL as its own tier
    int tier = 0;
    for(const QString& url : _trackers) {
        ct.add_tracker(url.toStdString(), tier++);
    }
    // Add grouped tiers
    for(const QStringList& tierUrls : _trackerTiers) {
        for(const QString& url : tierUrls) {
            ct.add_tracker(url.toStdString(), tier);
        }
        tier++;
    }

    // Web seeds
    for(const QString& url : _webSeeds) {
        ct.add_url_seed(url.toStdString());
    }

    // Metadata
    if(!_comment.isEmpty()) {
        ct.set_comment(_comment.toStdString().c_str());
    }
    if(!_creator.isEmpty()) {
        ct.set_creator(_creator.toStdString().c_str());
    }
    ct.set_priv(_private);

    // Hash pieces with progress reporting
    std::string parentPath;
    if(sourceInfo.isDir()) {
        parentPath = sourceInfo.absolutePath().toStdString();
    }
    else {
        parentPath = sourceInfo.absolutePath().toStdString();
    }

    int totalPcs = ct.num_pieces();
    _pieceCount = totalPcs;

    lt::error_code ec;
    lt::set_piece_hashes(ct, parentPath,
        [this, totalPcs](lt::piece_index_t p) {
            emit progressUpdated(static_cast<int>(p) + 1, totalPcs);
        },
        ec);

    if(ec) {
        _errorString = QString("Failed to hash pieces: %1").arg(QString::fromStdString(ec.message()));
        logText(LVL_ERROR, _errorString);
        return false;
    }

    // Generate the torrent
    lt::entry e = ct.generate();
    std::vector<char> buf;
    lt::bencode(std::back_inserter(buf), e);

    // Write to file
    QFile outFile(outputPath);
    if(!outFile.open(QIODevice::WriteOnly)) {
        _errorString = QString("Cannot write output file: %1").arg(outputPath);
        logText(LVL_ERROR, _errorString);
        return false;
    }
    outFile.write(buf.data(), static_cast<qint64>(buf.size()));
    outFile.close();

    _totalSize = fs.total_size();
    _fileCount = fs.num_files();

    logText(LVL_INFO, QString("Created torrent: %1 (%2 files, %3 pieces)")
        .arg(outputPath).arg(_fileCount).arg(_pieceCount));
    return true;
}
