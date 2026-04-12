#include <Kanoop/torrent/torrentsearcher.h>
#include <Kanoop/http/httpget.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

static const QByteArray BrowserUserAgent =
    "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36";

TorrentSearcher::TorrentSearcher(QObject* parent) :
    QObject(parent),
    LoggingBaseClass("torrent-search"),
    _apiBaseUrl("https://apibay.org")
{
}

void TorrentSearcher::search(const QString& query)
{
    if(query.trimmed().isEmpty()) {
        logText(LVL_WARNING, "Empty search query");
        emit searchFailed("Search query is empty");
        return;
    }

    // cat=100 = Audio category
    QString url = QString("%1/q.php?q=%2&cat=100")
        .arg(_apiBaseUrl, QUrl::toPercentEncoding(query.trimmed()));

    logText(LVL_DEBUG, QString("Searching: %1").arg(url));

    HttpGet* get = new HttpGet(url);
    get->appendHeader("User-Agent", BrowserUserAgent);
    if(_networkProxy.type() != QNetworkProxy::DefaultProxy) {
        get->setNetworkProxy(_networkProxy);
    }
    connect(get, &HttpGet::finished, this, &TorrentSearcher::onHttpFinished);
    get->start();
}

void TorrentSearcher::onHttpFinished()
{
    HttpGet* get = qobject_cast<HttpGet*>(sender());
    if(!get) {
        logText(LVL_ERROR, "onHttpFinished: sender is null");
        emit searchFailed("Internal error");
        return;
    }

    logText(LVL_DEBUG, QString("Response: HTTP %1, %2 bytes")
        .arg(get->statusCode()).arg(get->responseBody().size()));

    if(get->statusCode() != 200) {
        QString reason = QString("Search returned HTTP %1").arg(get->statusCode());
        logText(LVL_WARNING, reason);
        get->deleteLater();
        emit searchFailed(reason);
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(get->responseBody(), &parseError);
    get->deleteLater();

    if(parseError.error != QJsonParseError::NoError) {
        QString reason = QString("JSON parse error: %1").arg(parseError.errorString());
        logText(LVL_WARNING, reason);
        emit searchFailed(reason);
        return;
    }

    if(!doc.isArray()) {
        logText(LVL_WARNING, "Response is not a JSON array");
        emit searchFailed("Unexpected response format");
        return;
    }

    QJsonArray array = doc.array();
    QList<TorrentSearchResult> results;

    for(int i = 0; i < array.count(); ++i) {
        QJsonObject obj = array.at(i).toObject();

        // apibay returns [{"id":"0","name":"No results returned"}] for empty results
        if(obj.value("id").toString() == "0" && obj.value("name").toString() == "No results returned") {
            logText(LVL_DEBUG, "No results returned from API");
            emit searchComplete(results);
            return;
        }

        TorrentSearchResult result = TorrentSearchResult::fromJson(obj);
        if(!result.infoHash().isEmpty()) {
            results.append(result);
        }
    }

    logText(LVL_DEBUG, QString("Found %1 results").arg(results.count()));
    emit searchComplete(results);
}
