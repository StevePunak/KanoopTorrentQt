#ifndef TORRENTSEARCHER_H
#define TORRENTSEARCHER_H

#include <Kanoop/torrent/kanooptorrent.h>
#include <Kanoop/torrent/torrentsearchresult.h>
#include <Kanoop/utility/loggingbaseclass.h>
#include <QNetworkProxy>
#include <QObject>

class LIBKANOOPTORRENT_EXPORT TorrentSearcher : public QObject,
                                                 public LoggingBaseClass
{
    Q_OBJECT
public:
    explicit TorrentSearcher(QObject* parent = nullptr);

    void search(const QString& query);

    QString apiBaseUrl() const { return _apiBaseUrl; }
    void setApiBaseUrl(const QString& value) { _apiBaseUrl = value; }

    QNetworkProxy networkProxy() const { return _networkProxy; }
    void setNetworkProxy(const QNetworkProxy& value) { _networkProxy = value; }

signals:
    void searchComplete(const QList<TorrentSearchResult>& results);
    void searchFailed(const QString& errorMessage);

private slots:
    void onHttpFinished();

private:
    QNetworkProxy _networkProxy;
    QString _apiBaseUrl;
};

#endif // TORRENTSEARCHER_H
