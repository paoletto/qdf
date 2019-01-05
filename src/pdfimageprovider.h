/*
Copyright (C) 2023- Paolo Angelelli <paoletto@gmail.com>

This work is licensed under the terms of the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/ or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.

In addition to the above,
- The use of this work for training artificial intelligence is prohibited for both commercial and non-commercial use.
- Any and all donation options in derivative work must be the same as in the original work.
- All use of this work outside of the above terms must be explicitly agreed upon in advance with the exclusive copyright owner(s).
- Any derivative work must retain the above copyright and acknowledge that any and all use of the derivative work outside the above terms
  must be explicitly agreed upon in advance with the exclusive copyright owner(s) of the original work.
*/

#ifndef PDFIMAGEPROVIDER_H
#define PDFIMAGEPROVIDER_H


#include <QQuickAsyncImageProvider>
#include <QQuickImageResponse>
#include <QAbstractItemModel>
#include <QAbstractListModel>
#include <QVariantMap>
#include <QVariantList>
#include <QPdfDocument>
#include <QUrl>
#include <QDebug>
#include <QPointer>
#include <QThreadPool>
#include <QSharedPointer>




class PdfManager : public QObject
{
    Q_OBJECT
public:
    PdfManager(QObject *parent = nullptr);
    ~PdfManager();

    Q_INVOKABLE int openDocument(const QUrl &doc);
    Q_INVOKABLE void closeDocument(int documentId);
    Q_INVOKABLE int pageCount(int documentId);
    Q_INVOKABLE quint64 bytesCount(int documentId);
    Q_INVOKABLE QString fileName(int documentId);
    Q_INVOKABLE QSizeF pageSize(int documentId, int page);
    Q_INVOKABLE QVariantMap metadata(int documentId);
    Q_INVOKABLE QVariantList pages(int documentId);

    struct DocumentLayout
    {
        QSize documentSize;
        QHash<int, QRect> pageGeometries;
    };

    enum PageMode
    {
        SinglePage,
        MultiPage
    };
    Q_ENUM(PageMode)

    bool isReady(int documentId);
    QImage render(int documentId
                  ,int page
                  ,QSize imageSize
                  /*,QPdfDocumentRenderOptions options = QPdfDocumentRenderOptions()*/ );

public slots:
    void onLoadFinished(int documentId);
signals:
    void ready(int documentId);

public:
    QMap<int, DocumentLayout> m_layouts;
    PageMode m_pageMode = SinglePage;
    QMap<int, QPointer<QPdfDocument>> m_documents;
    QMap<int, QString> m_documentsFileName;
    QMap<int, bool> m_ready;
    QMap<int, QUrl> m_urls;
    int m_maxId = -1;
};

// ToDo: This crashes on destruction. Figure out why
class PdfImageProvider : public QQuickAsyncImageProvider
{
public:
// QQuickImageProvider
    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;

    static PdfImageProvider& instance();

    void setManager(PdfManager &manager);

private:
    PdfImageProvider();

public:
    PdfImageProvider(PdfImageProvider const&) = delete;
    void operator=(PdfImageProvider const&)  = delete;

    QPointer<PdfManager> m_manager;
    QThreadPool m_pool;
};

#endif // PDFIMAGEPROVIDER_H
