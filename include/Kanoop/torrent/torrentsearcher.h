#ifndef TORRENTSEARCHER_H
#define TORRENTSEARCHER_H

#include <Kanoop/torrent/kanooptorrent.h>
#include <Kanoop/torrent/torrentsearchresult.h>
#include <Kanoop/utility/loggingbaseclass.h>
#include <QNetworkProxy>
#include <QObject>

/**
 * @brief Asynchronous torrent search against a public API.
 *
 * TorrentSearcher queries an HTTP torrent index (default: apibay.org)
 * and returns results as a list of TorrentSearchResult objects. Results
 * are filtered to the Audio category.
 *
 * @code
 *   auto* searcher = new TorrentSearcher(this);
 *   connect(searcher, &TorrentSearcher::searchComplete,
 *           this, [](const QList<TorrentSearchResult>& results) {
 *       for (const auto& r : results)
 *           qDebug() << r.name() << r.seeders();
 *   });
 *   searcher->search("pink floyd");
 * @endcode
 */
class LIBKANOOPTORRENT_EXPORT TorrentSearcher : public QObject,
                                                 public LoggingBaseClass
{
    Q_OBJECT
public:
    /**
     * @brief Construct a TorrentSearcher.
     * @param parent Optional QObject parent.
     */
    explicit TorrentSearcher(QObject* parent = nullptr);

    /**
     * @brief Start an asynchronous search.
     * @param query The search query string.
     *
     * Results arrive via searchComplete(); errors via searchFailed().
     */
    void search(const QString& query);

    /** @brief Base URL for the search API (default: @c "https://apibay.org"). */
    QString apiBaseUrl() const { return _apiBaseUrl; }
    /** @brief Set a custom search API base URL. */
    void setApiBaseUrl(const QString& value) { _apiBaseUrl = value; }

    /** @brief SOCKS5 proxy used for search HTTP requests. */
    QNetworkProxy networkProxy() const { return _networkProxy; }
    /** @brief Set a SOCKS5 proxy for search requests. */
    void setNetworkProxy(const QNetworkProxy& value) { _networkProxy = value; }

signals:
    /** @brief Emitted when a search completes successfully. May be empty if no results were found. */
    void searchComplete(const QList<TorrentSearchResult>& results);
    /** @brief Emitted when a search fails (network error, parse error, etc.). */
    void searchFailed(const QString& errorMessage);

private slots:
    void onHttpFinished();

private:
    QNetworkProxy _networkProxy;
    QString _apiBaseUrl;
};

#endif // TORRENTSEARCHER_H
