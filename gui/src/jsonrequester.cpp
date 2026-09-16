#include "jsonrequester.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDebug>
#include <QRegularExpression>

JsonRequester::JsonRequester(QObject* parent) : QObject(parent), networkManager(new QNetworkAccessManager(this)) {
    connect(networkManager, &QNetworkAccessManager::finished, this, &JsonRequester::onRequestFinished);
}

QString JsonRequester::generateBearerAuthHeader(QString bearerToken) {
    return QString("Bearer %1").arg(bearerToken);
}

QString JsonRequester::generateBasicAuthHeader(QString username, QString password) {
    QString combined = QString("%1:%2").arg(username).arg(password);
    QString authHeader = "Basic " + combined.toUtf8().toBase64();
    return authHeader;
}

void JsonRequester::makePostRequest(const QString& url, const QString& authHeader, const QString contentType,
                                    const QString body, QString userAgent, const QHash<QString, QString>& additionalHeaders) {
    makeRequest(true, url, authHeader, contentType, body, userAgent, additionalHeaders);
}

void JsonRequester::makeGetRequest(const QString& url, const QString& authHeader, const QString contentType, QString userAgent, const QHash<QString, QString>& additionalHeaders) {
    makeRequest(false, url, authHeader, contentType, nullptr, userAgent, additionalHeaders);
}

#include "log_redaction.h"

void JsonRequester::makeRequest(bool post, const QString& url, const QString& authHeader, const QString contentType,
                                const QString body, QString userAgent, const QHash<QString, QString>& additionalHeaders) {
    // Log only method and sanitized URL (never log Request Body)
    QString sanitizedUrl = ludelo::log::sanitize_psn_url(url);
    qCInfo(chiakiGui) << "PSN" << (post ? "POST" : "GET") << "request:" << sanitizedUrl;

    QUrl q_url(url);
    QNetworkRequest request(q_url);
    if (!authHeader.isEmpty()) {
        request.setRawHeader("Authorization", authHeader.toUtf8());
    }
    request.setRawHeader("Content-Type", contentType.toUtf8());
    
    if (!userAgent.isEmpty()) {
        request.setRawHeader("User-Agent", userAgent.toUtf8());
    }
    
    for (auto it = additionalHeaders.constBegin(); it != additionalHeaders.constEnd(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }

    QNetworkReply* reply;
    if (post) {
        QByteArray postData = body.toUtf8();
        reply = networkManager->post(request, postData);
    } else {
        reply = networkManager->get(request);
    }

    currentReplies.insert(reply, url);
}

void JsonRequester::onRequestFinished(QNetworkReply* reply) {
    const QString url = currentReplies.value(reply);
    currentReplies.remove(reply);
    const QString sanitizedUrl = ludelo::log::sanitize_psn_url(url);
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() == QNetworkReply::NoError) {
        const QByteArray data = reply->readAll();
        
        // Log only method, sanitized URL, and status code (never log Response Body)
        qCInfo(chiakiGui) << "PSN response:" << sanitizedUrl << "- Status:" << statusCode;
        
        const QJsonDocument jsonDocument = QJsonDocument::fromJson(data);
        emit requestFinished(url, jsonDocument);
    } else {
        qCWarning(chiakiGui) << "PSN request error:" << sanitizedUrl << "- Status:" 
                             << statusCode << "-" << reply->errorString();
        emit requestError(url, reply->errorString(), reply->error());
    }

    reply->deleteLater();
}
