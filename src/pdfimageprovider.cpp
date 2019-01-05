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

#include "pdfimageprovider.h"
#include <QPdfDocument>
#include <QFileInfo>
#include <QtCore/qmath.h>
#include <QVector4D>

class AsyncImageResponse : public QQuickImageResponse, public QRunnable
{
public:
    AsyncImageResponse(const QString &id, const QSize &requestedSize, PdfManager &pdfManager)
     : m_id(id), m_requestedSize(requestedSize), m_manager(pdfManager)
    {
        setAutoDelete(false);
        // m_id should be document/page
        QStringList parts = m_id.split('/');
        if (parts.size() < 2)
            return;
        bool ok = true;
        m_documentId = parts.at(0).toInt(&ok);
        if (!ok) m_documentId = -1;
        m_page = parts.at(1).toInt(&ok);
        if (!ok) m_page = -1;
        if (parts.size() < 3) {
            // qDebug() << "Image Request w/o margins";
            return;
        }
        QString mrgs = parts.at(2);
        mrgs = mrgs.mid(1, mrgs.size() - 2);
        QStringList margins = mrgs.split(",");
        if (margins.size() == 4) {
            m_margins.setX(margins[0].toDouble());
            m_margins.setY(margins[1].toDouble());
            m_margins.setZ(margins[2].toDouble());
            m_margins.setW(margins[3].toDouble());
        }
        // qDebug() << "Image Request w margins:" << id << mrgs << margins << m_margins ;
    }

    static QSize croppableSize(const QSize &requestedSize, const QVector4D &margins)
    {
        int width = (float(requestedSize.width())
                     / (1.0 - margins.x() - margins.z()));
//        int height = (float(requestedSize.height())
//                      / (1.0 - margins.y() - margins.w()));
        int height = (float(requestedSize.height())
                      / (1.0 - margins.x() - margins.z()));
        return QSize(width, height);
    }

    void run() override
    {
        QSize sz = croppableSize(m_requestedSize, m_margins);
        m_image = m_manager.render(m_documentId, m_page, sz);
//        QString output = "/tmp/PDF" + QString::number(m_documentId) + "_" +
//                QString::number(m_page) + ".png" ;
//        m_image.save(output);
        if (!m_margins.isNull()) { // crop
//            m_image = m_image.copy((sz.width() * m_margins.x()),
//                                   (sz.height() * m_margins.y()),
//                                   m_requestedSize.width(),
//                                   m_requestedSize.height());
            qreal heightPct = (1.0 - m_margins.y() - m_margins.w());
            m_image = m_image.copy((sz.width() * m_margins.x()),
                                   (sz.height() * m_margins.y()),
                                   m_requestedSize.width(),
                                   m_image.height() * heightPct);
        }
//        qDebug() << "Image Rendered:" << m_documentId << m_margins << m_image.size();
        emit finished();
    }

    QQuickTextureFactory *textureFactory() const override
    {
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }

    QString m_id;
    int m_documentId = -1;
    int m_page = -1;
    QSize m_requestedSize;
    QImage m_image;
    PdfManager &m_manager;
    QVector4D m_margins;
};

QQuickImageResponse *PdfImageProvider::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    if (!m_manager)
        return nullptr;

    AsyncImageResponse *response = new AsyncImageResponse(id, requestedSize, *m_manager);
    m_pool.start(response);
    return response;
}

PdfImageProvider &PdfImageProvider::instance()
{
    static PdfImageProvider pdfProvider; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return pdfProvider;
}

void PdfImageProvider::setManager(PdfManager &manager)
{
    m_manager = &manager;
}

PdfImageProvider::PdfImageProvider()
    : QQuickAsyncImageProvider()
{

}
/*
static PdfManager::DocumentLayout calculateDocumentLayout(QPdfDocument &m_document,
                                                          PdfManager::PageMode &m_pageMode,
                                                          QMargins m_documentMargins,
                                                          qreal  m_zoomFactor,
                                                          qreal m_screenResolution, // pixel per point
                                                          QRect m_viewport,
                                                          int m_pageSpacing,
                                                          int pageNum)
{
    // The DocumentLayout describes a virtual layout where all pages are positioned inside
    //    - For SinglePage mode, this is just an area as large as the current page surrounded
    //      by the m_documentMargins.
    //    - For MultiPage mode, this is the area that is covered by all pages which are placed
    //      below each other, with m_pageSpacing inbetween and surrounded by m_documentMargins

    DocumentLayout documentLayout;

    if (!m_document || m_document->status() != QPdfDocument::Ready)
        return documentLayout;

    QHash<int, QRect> pageGeometries;

    const int pageCount = m_document->pageCount();

    int totalWidth = 0;

    const int startPage = (m_pageMode == QPdfView::SinglePage ? pageNum : 0);
    const int endPage = (m_pageMode == QPdfView::SinglePage ? pageNum + 1 : pageCount);

    // calculate page sizes
    for (int page = startPage; page < endPage; ++page) {
        QSize pageSize;
        if (m_zoomMode == QPdfView::CustomZoom) {
            pageSize = QSizeF(m_document->pageSize(page) * m_screenResolution * m_zoomFactor).toSize();
        } else if (m_zoomMode == QPdfView::FitToWidth) {
            pageSize = QSizeF(m_document->pageSize(page) * m_screenResolution).toSize();
            const qreal factor = (qreal(m_viewport.width() - m_documentMargins.left() - m_documentMargins.right()) / qreal(pageSize.width()));
            pageSize *= factor;
        } else if (m_zoomMode == QPdfView::FitInView) {
            const QSize viewportSize(m_viewport.size() + QSize(-m_documentMargins.left() - m_documentMargins.right(), -m_pageSpacing));

            pageSize = QSizeF(m_document->pageSize(page) * m_screenResolution).toSize();
            pageSize = pageSize.scaled(viewportSize, Qt::KeepAspectRatio);
        }

        totalWidth = qMax(totalWidth, pageSize.width());

        pageGeometries[page] = QRect(QPoint(0, 0), pageSize);
    }

    totalWidth += m_documentMargins.left() + m_documentMargins.right();

    int pageY = m_documentMargins.top();

    // calculate page positions
    for (int page = startPage; page < endPage; ++page) {
        const QSize pageSize = pageGeometries[page].size();

        // center horizontal inside the viewport
        const int pageX = (qMax(totalWidth, m_viewport.width()) - pageSize.width()) / 2;

        pageGeometries[page].moveTopLeft(QPoint(pageX, pageY));

        pageY += pageSize.height() + m_pageSpacing;
    }

    pageY += m_documentMargins.bottom();

    documentLayout.pageGeometries = pageGeometries;

    // calculate overall document size
    documentLayout.documentSize = QSize(totalWidth, pageY);

    return documentLayout;
}
*/



PdfManager::PdfManager(QObject *parent) : QObject(parent)
{
    PdfImageProvider::instance().setManager(*this);
}

PdfManager::~PdfManager()
{}

// returns the document id
int PdfManager::openDocument(const QUrl &doc)
{
    // ToDo use QNam to fetch from both local AND remote.
    const QString filePath = doc.toString(QUrl::NormalizePathSegments);
    const bool fileExists = QFileInfo::exists(filePath) && QFileInfo(filePath).isFile();
    if (!fileExists)
        return -1;

    m_maxId++;
    int documentId = m_maxId;

    QPdfDocument *dc = new QPdfDocument(this);
    m_documents[documentId] = dc;
    m_documentsFileName[documentId] = QFileInfo(filePath).fileName();
    connect(dc, &QPdfDocument::statusChanged,
            [this, documentId](const QPdfDocument::Status &status) {
                if (status == QPdfDocument::Ready)
                    this->onLoadFinished(documentId);
                else {
                    qWarning() << "QPdfDocument status changed for " << documentId << " : " << status;
                }
            });
    dc->load(filePath);
    return documentId;
}

void PdfManager::closeDocument(int documentId)
{
    if (!m_documents.contains(documentId) || m_documents[documentId].isNull())
        return;
    m_documents[documentId]->deleteLater();
    m_documents.remove(documentId);
}

int PdfManager::pageCount(int documentId)
{
    if (!m_documents.contains(documentId))
        return 0;
    if (!isReady(documentId))
        return 0;
    return m_documents.value(documentId)->pageCount();
}

quint64 PdfManager::bytesCount(int documentId)
{
    if (!m_documents.contains(documentId))
        return 0;
    if (!isReady(documentId))
        return 0;
    return m_documents.value(documentId)->bytesCount();
}

QString PdfManager::fileName(int documentId)
{
    QString res;
    if (!m_documentsFileName.contains(documentId))
        return res;
    return m_documentsFileName.value(documentId);
}

// One point is 1/72 inch (around 0.3528 mm, better: 0.352777778mm).
// Screen.pixelDensity provides the number of pixels per millimeter.

QSizeF PdfManager::pageSize(int documentId, int page)
{
    if (!m_documents.contains(documentId))
        return QSizeF();;
    if (!isReady(documentId))
        return QSizeF();;
    if (page < 0 || page >= pageCount(documentId))
        return QSizeF();
    return m_documents.value(documentId)->pageSize(page);
}

QVariantMap PdfManager::metadata(int documentId)
{
    if (!m_documents.contains(documentId))
        return QVariantMap();
    if (!isReady(documentId))
        return QVariantMap();
    QList<QPdfDocument::MetaDataField> fields { {         QPdfDocument::Title,
                    QPdfDocument::Subject,
                    QPdfDocument::Author,
                    QPdfDocument::Keywords,
                    QPdfDocument::Producer,
                    QPdfDocument::Creator,
                    QPdfDocument::CreationDate,
                    QPdfDocument::ModificationDate } };
    QMetaEnum metaEnum = QMetaEnum::fromType<QPdfDocument::MetaDataField>();
    QVariantMap meta;
    for (const auto f: fields)
        meta[metaEnum.valueToKey(f)] = m_documents.value(documentId)->metaData(f);

    meta["Url"] = m_urls[documentId];
    return meta;
}

QVariantList PdfManager::pages(int documentId)
{
    QVariantList p;
    if (!m_documents.contains(documentId))
        return p;
    if (!isReady(documentId))
        return p;

    for (int i = 0; i < pageCount(documentId); i++) {
        QVariantMap entry;
        entry["image"] =  QStringLiteral("image://pdfpages/")
                        + QString::number(documentId)
                        + QStringLiteral("/")
                        + QString::number(i);
        entry["page_number"] = QString::number(i);
        entry["doc_id"] = QString::number(documentId);
        QSizeF ps = pageSize(documentId, i);
        entry["page_width"] = ps.width();
        entry["page_height"] = ps.height();
        entry["page_ar"] = ps.width() / ps.height();
        p.append(entry);
    }
    return p;
}

bool PdfManager::isReady(int documentId)
{
    return m_ready.value(documentId, false);
}

QImage PdfManager::render(int documentId, int page, QSize imageSize /*,QPdfDocumentRenderOptions options*/ )
{
    if (!m_documents.contains(documentId))
        return QImage();;
    if (!isReady(documentId))
        return QImage();;
    if (page < 0 || page >= pageCount(documentId))
        return QImage();

    QPdfDocumentRenderOptions opts;
    opts.setRenderFlags(QPdf::RenderAnnotations
//                        |QPdf::RenderAnnotations
//                        |QPdf::RenderOptimizedForLcd
//                        |QPdf::RenderGrayscale
//                        |QPdf::RenderForceHalftone
//                        |QPdf::RenderTextAliased
//                        |QPdf::RenderImageAliased
//                        |QPdf::RenderPathAliased
                        );
    return m_documents.value(documentId)->render(page, imageSize, opts);
}

void PdfManager::onLoadFinished(int documentId)
{
    m_ready[documentId] = true;
    emit ready(documentId);
}


