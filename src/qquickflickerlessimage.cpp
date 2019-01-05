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

#include "qquickflickerlessimage.h"
#include <QtCore/qmath.h>
#include <QVector4D>

#include <QtGui/qguiapplication.h>
#include <QtGui/qscreen.h>
#include <QtGui/qicon.h>

#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlfile.h>

#include <QtQuick/qsgtextureprovider.h>

#include <QtQuick/private/qsgcontext_p.h>
#include <QtQuick/private/qsgadaptationlayer_p.h>
#include <private/qnumeric_p.h>

#include <QtCore/qmath.h>
#include <QtGui/qpainter.h>
#include <QtCore/QRunnable>
#include <private/qquickpixmapcache_p.h>

// This function gives derived classes the chance set the devicePixelRatio
// if they're not happy with our implementation of it.
bool QQuickImageBasePrivate::updateDevicePixelRatio(qreal targetDevicePixelRatio)
{
    // QQuickImageProvider and SVG can generate a high resolution image when
    // sourceSize is set (this function is only called if it's set).
    // If sourceSize is not set then the provider default size will be used, as usual.
    bool setDevicePixelRatio = false;
    if (url.scheme() == QLatin1String("image")) {
        setDevicePixelRatio = true;
    } else {
        QString stringUrl = url.path(QUrl::PrettyDecoded);
        if (stringUrl.endsWith(QLatin1String("svg")) ||
            stringUrl.endsWith(QLatin1String("svgz"))) {
            setDevicePixelRatio = true;
        }
    }

    if (setDevicePixelRatio)
        devicePixelRatio = targetDevicePixelRatio;

    return setDevicePixelRatio;
}

QQuickImageBase::QQuickImageBase(QQuickItem *parent)
: QQuickImplicitSizeItem(*(new QQuickImageBasePrivate), parent)
{
    setFlag(ItemHasContents);
}

QQuickImageBase::QQuickImageBase(QQuickImageBasePrivate &dd, QQuickItem *parent)
: QQuickImplicitSizeItem(dd, parent)
{
    setFlag(ItemHasContents);
}

QQuickImageBase::~QQuickImageBase()
{
}

QQuickImageBase::Status QQuickImageBase::status() const
{
    Q_D(const QQuickImageBase);
    return d->status;
}


qreal QQuickImageBase::progress() const
{
    Q_D(const QQuickImageBase);
    return d->progress;
}


bool QQuickImageBase::asynchronous() const
{
    Q_D(const QQuickImageBase);
    return d->async;
}

void QQuickImageBase::setAsynchronous(bool async)
{
    Q_D(QQuickImageBase);
    if (d->async != async) {
        d->async = async;
        emit asynchronousChanged();
    }
}

QUrl QQuickImageBase::source() const
{
    Q_D(const QQuickImageBase);
    return d->url;
}

void QQuickImageBase::setSource(const QUrl &url)
{
    Q_D(QQuickImageBase);

    if (url == d->url)
        return;

    d->url = url;
    emit sourceChanged(d->url);

    if (isComponentComplete())
        load();
}

void QQuickImageBase::setSourceSize(const QSize& size)
{
    Q_D(QQuickImageBase);
    if (d->sourcesize == size)
        return;

    d->sourcesize = size;
    emit sourceSizeChanged();
    if (isComponentComplete())
        load();
}

QSize QQuickImageBase::sourceSize() const
{
    Q_D(const QQuickImageBase);

    int width = d->sourcesize.width();
    int height = d->sourcesize.height();
    return QSize(width != -1 ? width : d->pix->width(), height != -1 ? height : d->pix->height());
}

void QQuickImageBase::resetSourceSize()
{
    setSourceSize(QSize());
}

bool QQuickImageBase::cache() const
{
    Q_D(const QQuickImageBase);
    return d->cache;
}

void QQuickImageBase::setCache(bool cache)
{
    Q_D(QQuickImageBase);
    if (d->cache == cache)
        return;

    d->cache = cache;
    emit cacheChanged();
    if (isComponentComplete())
        load();
}

QImage QQuickImageBase::image() const
{
    Q_D(const QQuickImageBase);
    return d->pix->image();
}

void QQuickImageBase::setMirror(bool mirror)
{
    Q_D(QQuickImageBase);
    if (mirror == d->mirror)
        return;

    d->mirror = mirror;

    if (isComponentComplete())
        update();

    emit mirrorChanged();
}

bool QQuickImageBase::mirror() const
{
    Q_D(const QQuickImageBase);
    return d->mirror;
}

void QQuickImageBase::load()
{
    Q_D(QQuickImageBase);

    if (d->url.isEmpty()) {
        d->pix->clear(this);
        d->pixLoading->clear(this);
        if (d->progress != 0.0) {
            d->progress = 0.0;
            emit progressChanged(d->progress);
        }
        pixmapChange();
        d->status = Null;
        emit statusChanged(d->status);

        if (sourceSize() != d->oldSourceSize) {
            d->oldSourceSize = sourceSize();
            emit sourceSizeChanged();
        }
        if (autoTransform() != d->oldAutoTransform) {
            d->oldAutoTransform = autoTransform();
            emitAutoTransformBaseChanged();
        }
        update();

    } else {
        QQuickPixmap::Options options;
        if (d->async)
            options |= QQuickPixmap::Asynchronous;
        if (d->cache)
            options |= QQuickPixmap::Cache;
        d->pixLoading->clear(this);

        const qreal targetDevicePixelRatio = (window() ? window()->effectiveDevicePixelRatio() : qApp->devicePixelRatio());
        d->devicePixelRatio = 1.0;

        QUrl loadUrl = d->url;

        bool updatedDevicePixelRatio = false;
        if (d->sourcesize.isValid())
            updatedDevicePixelRatio = d->updateDevicePixelRatio(targetDevicePixelRatio);

        if (!updatedDevicePixelRatio) {
            // (possible) local file: loadUrl and d->devicePixelRatio will be modified if
            // an "@2x" file is found.
            resolve2xLocalFile(d->url, targetDevicePixelRatio, &loadUrl, &d->devicePixelRatio);
        }

        d->pixLoading->load(qmlEngine(this),
                            loadUrl,
                            QRect(),
                            d->sourcesize * d->devicePixelRatio,
                            options,
                            d->providerOptions);

        if (d->pixLoading->isLoading()) {
            if (d->progress != 0.0) {
                d->progress = 0.0;
                emit progressChanged(d->progress);
            }
            if (d->status != Loading) {
                d->status = Loading;
                emit statusChanged(d->status);
            }

            static int thisRequestProgress = -1;
            static int thisRequestFinished = -1;
            if (thisRequestProgress == -1) {
                thisRequestProgress =
                    QQuickImageBase::staticMetaObject.indexOfSlot("requestProgress(qint64,qint64)");
                thisRequestFinished =
                    QQuickImageBase::staticMetaObject.indexOfSlot("requestFinished()");
            }

            d->pixLoading->connectFinished(this, thisRequestFinished);
            d->pixLoading->connectDownloadProgress(this, thisRequestProgress);
            if (d->pix == d->pixLoading) // not double-buffered
                update(); //pixmap may have invalidated texture, updatePaintNode needs to be called before the next repaint
        } else {
            requestFinished();
        }
    }
}

void QQuickImageBase::requestFinished()
{
    Q_D(QQuickImageBase);

    if (d->pixLoading->isError()) {
        qmlWarning(this) << d->pixLoading->error();
        d->pixLoading->clear(this);
        d->pix->clear(this);
        d->status = Error;
        if (d->progress != 0.0) {
            d->progress = 0.0;
            emit progressChanged(d->progress);
        }
    } else {
        d->swapPixBuffers();
        if (d->pix != d->pixLoading) // double-buffered
            d->pixLoading->clear(this);
        d->status = Ready;
        if (d->progress != 1.0) {
            d->progress = 1.0;
            emit progressChanged(d->progress);
        }
    }
    pixmapChange();
    emit statusChanged(d->status);
    if (sourceSize() != d->oldSourceSize) {
        d->oldSourceSize = sourceSize();
        emit sourceSizeChanged();
    }
    if (autoTransform() != d->oldAutoTransform) {
        d->oldAutoTransform = autoTransform();
        emitAutoTransformBaseChanged();
    }
    update();
}

void QQuickImageBase::requestProgress(qint64 received, qint64 total)
{
    Q_D(QQuickImageBase);
    if (d->status == Loading && total > 0) {
        d->progress = qreal(received)/total;
        emit progressChanged(d->progress);
    }
}

void QQuickImageBase::itemChange(ItemChange change, const ItemChangeData &value)
{
    Q_D(QQuickImageBase);
    // If the screen DPI changed, reload image.
    if (change == ItemDevicePixelRatioHasChanged && value.realValue != d->devicePixelRatio) {
        // ### how can we get here with !qmlEngine(this)? that implies
        // itemChange() on an item pending deletion, which seems strange.
        if (qmlEngine(this) && isComponentComplete() && d->url.isValid()) {
            load();
        }
    }
    QQuickItem::itemChange(change, value);
}

void QQuickImageBase::componentComplete()
{
    Q_D(QQuickImageBase);
    QQuickItem::componentComplete();
    if (d->url.isValid())
        load();
}

void QQuickImageBase::pixmapChange()
{
    Q_D(QQuickImageBase);
    setImplicitSize(d->pix->width() / d->devicePixelRatio, d->pix->height() / d->devicePixelRatio);
}

void QQuickImageBase::resolve2xLocalFile(const QUrl &url, qreal targetDevicePixelRatio, QUrl *sourceUrl, qreal *sourceDevicePixelRatio)
{
    Q_ASSERT(sourceUrl);
    Q_ASSERT(sourceDevicePixelRatio);

    // Bail out if "@2x" image loading is disabled, don't change the source url or devicePixelRatio.
    static const bool disable2xImageLoading = !qEnvironmentVariableIsEmpty("QT_HIGHDPI_DISABLE_2X_IMAGE_LOADING");
    if (disable2xImageLoading)
        return;

    const QString localFile = QQmlFile::urlToLocalFileOrQrc(url);

    // Non-local file path: @2x loading is not supported.
    if (localFile.isEmpty())
        return;

    // Special case: the url in the QML source refers directly to an "@2x" file.
    int atLocation = localFile.lastIndexOf(QLatin1Char('@'));
    if (atLocation > 0 && atLocation + 3 < localFile.size()) {
        if (localFile[atLocation + 1].isDigit()
                && localFile[atLocation + 2] == QLatin1Char('x')
                && localFile[atLocation + 3] == QLatin1Char('.')) {
            *sourceDevicePixelRatio = localFile[atLocation + 1].digitValue();
            return;
        }
    }

    // Look for an @2x version
    QString localFileX = qt_findAtNxFile(localFile, targetDevicePixelRatio, sourceDevicePixelRatio);
    if (localFileX != localFile)
        *sourceUrl = QUrl::fromLocalFile(localFileX);
}

bool QQuickImageBase::autoTransform() const
{
    Q_D(const QQuickImageBase);
    if (d->providerOptions.autoTransform() == QQuickImageProviderOptions::UsePluginDefaultTransform)
        return d->pix->autoTransform() == QQuickImageProviderOptions::ApplyTransform;
    return d->providerOptions.autoTransform() == QQuickImageProviderOptions::ApplyTransform;
}

void QQuickImageBase::setAutoTransform(bool transform)
{
    Q_D(QQuickImageBase);
    if (d->providerOptions.autoTransform() != QQuickImageProviderOptions::UsePluginDefaultTransform &&
        transform == (d->providerOptions.autoTransform() == QQuickImageProviderOptions::ApplyTransform))
        return;
    d->providerOptions.setAutoTransform(transform ? QQuickImageProviderOptions::ApplyTransform : QQuickImageProviderOptions::DoNotApplyTransform);
    emitAutoTransformBaseChanged();
}



QQuickImageTextureProvider::QQuickImageTextureProvider()
    : m_texture(nullptr)
    , m_smooth(false)
{
}

void QQuickImageTextureProvider::updateTexture(QSGTexture *texture) {
    if (m_texture == texture)
        return;
    m_texture = texture;
    emit textureChanged();
}

QSGTexture *QQuickImageTextureProvider::texture() const {
    if (m_texture) {
        m_texture->setFiltering(m_smooth ? QSGTexture::Linear : QSGTexture::Nearest);
        m_texture->setMipmapFiltering(m_mipmap ? QSGTexture::Linear : QSGTexture::None);
        m_texture->setHorizontalWrapMode(QSGTexture::ClampToEdge);
        m_texture->setVerticalWrapMode(QSGTexture::ClampToEdge);
    }
    return m_texture;
}

QQuickImagePrivate::QQuickImagePrivate()
    : fillMode(QQuickImage::Stretch)
    , paintedWidth(0)
    , paintedHeight(0)
    , pixmapChanged(false)
    , mipmap(false)
    , hAlign(QQuickImage::AlignHCenter)
    , vAlign(QQuickImage::AlignVCenter)
    , provider(nullptr)
{
}


QQuickImage::QQuickImage(QQuickItem *parent)
    : QQuickImageBase(*(new QQuickImagePrivate), parent)
{
}

QQuickImage::QQuickImage(QQuickImagePrivate &dd, QQuickItem *parent)
    : QQuickImageBase(dd, parent)
{
}

QQuickImage::~QQuickImage()
{
    Q_D(QQuickImage);
    if (d->provider) {
        // We're guaranteed to have a window() here because the provider would have
        // been released in releaseResources() if we were gone from a window.
        QQuickWindowQObjectCleanupJob::schedule(window(), d->provider);
    }
}

void QQuickImagePrivate::setImage(const QImage &image)
{
    Q_Q(QQuickImage);
    pix->setImage(image);

    q->pixmapChange();
    status = pix->isNull() ? QQuickImageBase::Null : QQuickImageBase::Ready;

    q->update();
}

void QQuickImagePrivate::setPixmap(const QQuickPixmap &pixmap)
{
    Q_Q(QQuickImage);
    pix->setPixmap(pixmap);

    q->pixmapChange();
    status = pix->isNull() ? QQuickImageBase::Null : QQuickImageBase::Ready;

    q->update();
}

QQuickImage::FillMode QQuickImage::fillMode() const
{
    Q_D(const QQuickImage);
    return d->fillMode;
}

void QQuickImage::setFillMode(FillMode mode)
{
    Q_D(QQuickImage);
    if (d->fillMode == mode)
        return;
    d->fillMode = mode;
    if ((mode == PreserveAspectCrop) != d->providerOptions.preserveAspectRatioCrop()) {
        d->providerOptions.setPreserveAspectRatioCrop(mode == PreserveAspectCrop);
        if (isComponentComplete())
            load();
    } else if ((mode == PreserveAspectFit) != d->providerOptions.preserveAspectRatioFit()) {
        d->providerOptions.setPreserveAspectRatioFit(mode == PreserveAspectFit);
        if (isComponentComplete())
            load();
    }
    update();
    updatePaintedGeometry();
    emit fillModeChanged();
}

qreal QQuickImage::paintedWidth() const
{
    Q_D(const QQuickImage);
    return d->paintedWidth;
}

qreal QQuickImage::paintedHeight() const
{
    Q_D(const QQuickImage);
    return d->paintedHeight;
}

void QQuickImage::updatePaintedGeometry()
{
    Q_D(QQuickImage);

    if (d->fillMode == PreserveAspectFit) {
        if (!d->pix->width() || !d->pix->height()) {
            setImplicitSize(0, 0);
            return;
        }
        const qreal pixWidth = d->pix->width() / d->devicePixelRatio;
        const qreal pixHeight = d->pix->height() / d->devicePixelRatio;
        const qreal w = widthValid() ? width() : pixWidth;
        const qreal widthScale = w / pixWidth;
        const qreal h = heightValid() ? height() : pixHeight;
        const qreal heightScale = h / pixHeight;
        if (widthScale <= heightScale) {
            d->paintedWidth = w;
            d->paintedHeight = widthScale * pixHeight;
        } else if (heightScale < widthScale) {
            d->paintedWidth = heightScale * pixWidth;
            d->paintedHeight = h;
        }
        const qreal iHeight = (widthValid() && !heightValid()) ? d->paintedHeight : pixHeight;
        const qreal iWidth = (heightValid() && !widthValid()) ? d->paintedWidth : pixWidth;
        setImplicitSize(iWidth, iHeight);

    } else if (d->fillMode == PreserveAspectCrop) {
        if (!d->pix->width() || !d->pix->height())
            return;
        const qreal pixWidth = d->pix->width() / d->devicePixelRatio;
        const qreal pixHeight = d->pix->height() / d->devicePixelRatio;
        qreal widthScale = width() / pixWidth;
        qreal heightScale = height() / pixHeight;
        if (widthScale < heightScale) {
            widthScale = heightScale;
        } else if (heightScale < widthScale) {
            heightScale = widthScale;
        }

        d->paintedHeight = heightScale * pixHeight;
        d->paintedWidth = widthScale * pixWidth;
    } else if (d->fillMode == Pad) {
        d->paintedWidth = d->pix->width() / d->devicePixelRatio;
        d->paintedHeight = d->pix->height() / d->devicePixelRatio;
    } else {
        d->paintedWidth = width();
        d->paintedHeight = height();
    }
    emit paintedGeometryChanged();
}

void QQuickImage::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickImageBase::geometryChanged(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size())
        updatePaintedGeometry();
}

QRectF QQuickImage::boundingRect() const
{
    Q_D(const QQuickImage);
    return QRectF(0, 0, qMax(width(), d->paintedWidth), qMax(height(), d->paintedHeight));
}

QSGTextureProvider *QQuickImage::textureProvider() const
{
    Q_D(const QQuickImage);

    // When Item::layer::enabled == true, QQuickItem will be a texture
    // provider. In this case we should prefer to return the layer rather
    // than the image itself. The layer will include any children and any
    // the image's wrap and fill mode.
    if (QQuickItem::isTextureProvider())
        return QQuickItem::textureProvider();

    if (!d->window || !d->sceneGraphRenderContext() || QThread::currentThread() != d->sceneGraphRenderContext()->thread()) {
        qWarning("QQuickImage::textureProvider: can only be queried on the rendering thread of an exposed window");
        return nullptr;
    }

    if (!d->provider) {
        QQuickImagePrivate *dd = const_cast<QQuickImagePrivate *>(d);
        dd->provider = new QQuickImageTextureProvider;
        dd->provider->m_smooth = d->smooth;
        dd->provider->m_mipmap = d->mipmap;
        dd->provider->updateTexture(d->sceneGraphRenderContext()->textureForFactory(d->pix->textureFactory(), window()));
    }

    return d->provider;
}

void QQuickImage::invalidateSceneGraph()
{
    Q_D(QQuickImage);
    delete d->provider;
    d->provider = nullptr;
}

void QQuickImage::releaseResources()
{
    Q_D(QQuickImage);
    if (d->provider) {
        QQuickWindowQObjectCleanupJob::schedule(window(), d->provider);
        d->provider = nullptr;
    }
}

QSGNode *QQuickImage::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    Q_D(QQuickImage);

    QSGTexture *texture = d->sceneGraphRenderContext()->textureForFactory(d->pix->textureFactory(), window());

    // Copy over the current texture state into the texture provider...
    if (d->provider) {
        d->provider->m_smooth = d->smooth;
        d->provider->m_mipmap = d->mipmap;
        d->provider->updateTexture(texture);
    }

    if (!texture || width() <= 0 || height() <= 0) {
        delete oldNode;
        return nullptr;
    }

    QSGInternalImageNode *node = static_cast<QSGInternalImageNode *>(oldNode);
    if (!node) {
        d->pixmapChanged = true;
        node = d->sceneGraphContext()->createInternalImageNode(static_cast<QSGDefaultRenderContext *>(d->sceneGraphRenderContext()));
    }

    QRectF targetRect;
    QRectF sourceRect;
    QSGTexture::WrapMode hWrap = QSGTexture::ClampToEdge;
    QSGTexture::WrapMode vWrap = QSGTexture::ClampToEdge;

    qreal pixWidth = (d->fillMode == PreserveAspectFit) ? d->paintedWidth : d->pix->width() / d->devicePixelRatio;
    qreal pixHeight = (d->fillMode == PreserveAspectFit) ? d->paintedHeight :  d->pix->height() / d->devicePixelRatio;

    int xOffset = 0;
    if (d->hAlign == QQuickImage::AlignHCenter)
        xOffset = qCeil((width() - pixWidth) / 2.);
    else if (d->hAlign == QQuickImage::AlignRight)
        xOffset = qCeil(width() - pixWidth);

    int yOffset = 0;
    if (d->vAlign == QQuickImage::AlignVCenter)
        yOffset = qCeil((height() - pixHeight) / 2.);
    else if (d->vAlign == QQuickImage::AlignBottom)
        yOffset = qCeil(height() - pixHeight);

    switch (d->fillMode) {
    default:
    case Stretch:
        targetRect = QRectF(0, 0, width(), height());
        sourceRect = d->pix->rect();
        break;

    case PreserveAspectFit:
        targetRect = QRectF(xOffset, yOffset, d->paintedWidth, d->paintedHeight);
        sourceRect = d->pix->rect();
        break;

    case PreserveAspectCrop: {
        targetRect = QRect(0, 0, width(), height());
        qreal wscale = width() / qreal(d->pix->width());
        qreal hscale = height() / qreal(d->pix->height());

        if (wscale > hscale) {
            int src = (hscale / wscale) * qreal(d->pix->height());
            int y = 0;
            if (d->vAlign == QQuickImage::AlignVCenter)
                y = qCeil((d->pix->height() - src) / 2.);
            else if (d->vAlign == QQuickImage::AlignBottom)
                y = qCeil(d->pix->height() - src);
            sourceRect = QRectF(0, y, d->pix->width(), src);

        } else {
            int src = (wscale / hscale) * qreal(d->pix->width());
            int x = 0;
            if (d->hAlign == QQuickImage::AlignHCenter)
                x = qCeil((d->pix->width() - src) / 2.);
            else if (d->hAlign == QQuickImage::AlignRight)
                x = qCeil(d->pix->width() - src);
            sourceRect = QRectF(x, 0, src, d->pix->height());
        }
        }
        break;

    case Tile:
        targetRect = QRectF(0, 0, width(), height());
        sourceRect = QRectF(-xOffset, -yOffset, width(), height());
        hWrap = QSGTexture::Repeat;
        vWrap = QSGTexture::Repeat;
        break;

    case TileHorizontally:
        targetRect = QRectF(0, 0, width(), height());
        sourceRect = QRectF(-xOffset, 0, width(), d->pix->height());
        hWrap = QSGTexture::Repeat;
        break;

    case TileVertically:
        targetRect = QRectF(0, 0, width(), height());
        sourceRect = QRectF(0, -yOffset, d->pix->width(), height());
        vWrap = QSGTexture::Repeat;
        break;

    case Pad:
        qreal w = qMin(qreal(pixWidth), width());
        qreal h = qMin(qreal(pixHeight), height());
        qreal x = (pixWidth > width()) ? -xOffset : 0;
        qreal y = (pixHeight > height()) ? -yOffset : 0;
        targetRect = QRectF(x + xOffset, y + yOffset, w, h);
        sourceRect = QRectF(x, y, w, h);
        break;
    };

    qreal nsWidth = (hWrap == QSGTexture::Repeat || d->fillMode == Pad) ? d->pix->width() / d->devicePixelRatio : d->pix->width();
    qreal nsHeight = (vWrap == QSGTexture::Repeat || d->fillMode == Pad) ? d->pix->height() / d->devicePixelRatio : d->pix->height();
    QRectF nsrect(sourceRect.x() / nsWidth,
                  sourceRect.y() / nsHeight,
                  sourceRect.width() / nsWidth,
                  sourceRect.height() / nsHeight);

    if (targetRect.isEmpty()
        || !qt_is_finite(targetRect.width()) || !qt_is_finite(targetRect.height())
        || nsrect.isEmpty()
        || !qt_is_finite(nsrect.width()) || !qt_is_finite(nsrect.height())) {
        delete node;
        return nullptr;
    }

    if (d->pixmapChanged) {
        // force update the texture in the node to trigger reconstruction of
        // geometry and the likes when a atlas segment has changed.
        if (texture->isAtlasTexture() && (hWrap == QSGTexture::Repeat || vWrap == QSGTexture::Repeat || d->mipmap))
            node->setTexture(texture->removedFromAtlas());
        else
            node->setTexture(texture);
        d->pixmapChanged = false;
    }

    node->setMipmapFiltering(d->mipmap ? QSGTexture::Linear : QSGTexture::None);
    node->setHorizontalWrapMode(hWrap);
    node->setVerticalWrapMode(vWrap);
    node->setFiltering(d->smooth ? QSGTexture::Linear : QSGTexture::Nearest);

    node->setTargetRect(targetRect);
    node->setInnerTargetRect(targetRect);
    node->setSubSourceRect(nsrect);
    node->setMirror(d->mirror);
    node->setAntialiasing(d->antialiasing);
    node->update();

    return node;
}

void QQuickImage::pixmapChange()
{
    Q_D(QQuickImage);
    // PreserveAspectFit calculates the implicit size differently so we
    // don't call our superclass pixmapChange(), since that would
    // result in the implicit size being set incorrectly, then updated
    // in updatePaintedGeometry()
    if (d->fillMode != PreserveAspectFit)
        QQuickImageBase::pixmapChange();
    updatePaintedGeometry();
    d->pixmapChanged = true;

    // When the pixmap changes, such as being deleted, we need to update the textures
    update();
}

QQuickImage::VAlignment QQuickImage::verticalAlignment() const
{
    Q_D(const QQuickImage);
    return d->vAlign;
}

void QQuickImage::setVerticalAlignment(VAlignment align)
{
    Q_D(QQuickImage);
    if (d->vAlign == align)
        return;

    d->vAlign = align;
    update();
    updatePaintedGeometry();
    emit verticalAlignmentChanged(align);
}

QQuickImage::HAlignment QQuickImage::horizontalAlignment() const
{
    Q_D(const QQuickImage);
    return d->hAlign;
}

void QQuickImage::setHorizontalAlignment(HAlignment align)
{
    Q_D(QQuickImage);
    if (d->hAlign == align)
        return;

    d->hAlign = align;
    update();
    updatePaintedGeometry();
    emit horizontalAlignmentChanged(align);
}

bool QQuickImage::mipmap() const
{
    Q_D(const QQuickImage);
    return d->mipmap;
}

void QQuickImage::setMipmap(bool use)
{
    Q_D(QQuickImage);
    if (d->mipmap == use)
        return;
    d->mipmap = use;
    emit mipmapChanged(d->mipmap);

    d->pixmapChanged = true;
    update();
}















QSGNode *QQuickFlickerlessImage::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *)
{
    Q_D(QQuickFlickerlessImage);

    QQuickPixmap *pix = d->pix;
    QSGTexture *texture = d->sceneGraphRenderContext()->textureForFactory(pix->textureFactory(), window());

    // Copy over the current texture state into the texture provider...
    if (d->provider) {
        d->provider->m_smooth = d->smooth;
        d->provider->m_mipmap = d->mipmap;
        d->provider->updateTexture(texture);
    }

    if (!texture || width() <= 0 || height() <= 0) {
        delete oldNode;
        return nullptr;
    }

    QSGDefaultInternalImageNodePlus *node = static_cast<QSGDefaultInternalImageNodePlus *>(oldNode);
//    QSGInternalImageNode *node = static_cast<QSGInternalImageNode *>(oldNode);
    if (!node) {
        d->pixmapChanged = true;
        node = new QSGDefaultInternalImageNodePlus(static_cast<QSGDefaultRenderContext *>(d->sceneGraphRenderContext()));
    }
    node->setUniforms(m_uniforms);

    QRectF targetRect;
    QRectF sourceRect;
    QSGTexture::WrapMode hWrap = QSGTexture::ClampToEdge;
    QSGTexture::WrapMode vWrap = QSGTexture::ClampToEdge;

    qreal pixWidth = (d->fillMode == PreserveAspectFit) ? d->paintedWidth : pix->width() / d->devicePixelRatio;
    qreal pixHeight = (d->fillMode == PreserveAspectFit) ? d->paintedHeight :  pix->height() / d->devicePixelRatio;

    int xOffset = 0;
    if (d->hAlign == QQuickImage::AlignHCenter)
        xOffset = qCeil((width() - pixWidth) / 2.);
    else if (d->hAlign == QQuickImage::AlignRight)
        xOffset = qCeil(width() - pixWidth);

    int yOffset = 0;
    if (d->vAlign == QQuickImage::AlignVCenter)
        yOffset = qCeil((height() - pixHeight) / 2.);
    else if (d->vAlign == QQuickImage::AlignBottom)
        yOffset = qCeil(height() - pixHeight);

    switch (d->fillMode) {
    default:
    case Stretch:
        targetRect = QRectF(0, 0, width(), height());
        sourceRect = pix->rect();
        break;

    case PreserveAspectFit:
        targetRect = QRectF(xOffset, yOffset, d->paintedWidth, d->paintedHeight);
        sourceRect = pix->rect();
        break;

    case PreserveAspectCrop: {
        targetRect = QRect(0, 0, width(), height());
        qreal wscale = width() / qreal(pix->width());
        qreal hscale = height() / qreal(pix->height());

        if (wscale > hscale) {
            int src = (hscale / wscale) * qreal(pix->height());
            int y = 0;
            if (d->vAlign == QQuickImage::AlignVCenter)
                y = qCeil((pix->height() - src) / 2.);
            else if (d->vAlign == QQuickImage::AlignBottom)
                y = qCeil(pix->height() - src);
            sourceRect = QRectF(0, y, pix->width(), src);

        } else {
            int src = (wscale / hscale) * qreal(pix->width());
            int x = 0;
            if (d->hAlign == QQuickImage::AlignHCenter)
                x = qCeil((pix->width() - src) / 2.);
            else if (d->hAlign == QQuickImage::AlignRight)
                x = qCeil(pix->width() - src);
            sourceRect = QRectF(x, 0, src, pix->height());
        }
    }
        break;

    case Tile:
        targetRect = QRectF(0, 0, width(), height());
        sourceRect = QRectF(-xOffset, -yOffset, width(), height());
        hWrap = QSGTexture::Repeat;
        vWrap = QSGTexture::Repeat;
        break;

    case TileHorizontally:
        targetRect = QRectF(0, 0, width(), height());
        sourceRect = QRectF(-xOffset, 0, width(), pix->height());
        hWrap = QSGTexture::Repeat;
        break;

    case TileVertically:
        targetRect = QRectF(0, 0, width(), height());
        sourceRect = QRectF(0, -yOffset, pix->width(), height());
        vWrap = QSGTexture::Repeat;
        break;

    case Pad:
        qreal w = qMin(qreal(pixWidth), width());
        qreal h = qMin(qreal(pixHeight), height());
        qreal x = (pixWidth > width()) ? -xOffset : 0;
        qreal y = (pixHeight > height()) ? -yOffset : 0;
        targetRect = QRectF(x + xOffset, y + yOffset, w, h);
        sourceRect = QRectF(x, y, w, h);
        break;
    };

    qreal nsWidth = (hWrap == QSGTexture::Repeat || d->fillMode == Pad) ? pix->width() / d->devicePixelRatio : pix->width();
    qreal nsHeight = (vWrap == QSGTexture::Repeat || d->fillMode == Pad) ? pix->height() / d->devicePixelRatio : pix->height();
    QRectF nsrect(sourceRect.x() / nsWidth,
                  sourceRect.y() / nsHeight,
                  sourceRect.width() / nsWidth,
                  sourceRect.height() / nsHeight);

    if (targetRect.isEmpty()
            || !qt_is_finite(targetRect.width()) || !qt_is_finite(targetRect.height())
            || nsrect.isEmpty()
            || !qt_is_finite(nsrect.width()) || !qt_is_finite(nsrect.height())) {
        delete node;
        return nullptr;
    }

    if (d->pixmapChanged) {
        // force update the texture in the node to trigger reconstruction of
        // geometry and the likes when a atlas segment has changed.
        if (texture->isAtlasTexture() && (hWrap == QSGTexture::Repeat || vWrap == QSGTexture::Repeat || d->mipmap))
            node->setTexture(texture->removedFromAtlas());
        else
            node->setTexture(texture);
        d->pixmapChanged = false;
    }

    node->setMipmapFiltering(d->mipmap ? QSGTexture::Linear : QSGTexture::None);
    node->setHorizontalWrapMode(hWrap);
    node->setVerticalWrapMode(vWrap);
    node->setFiltering(d->smooth ? QSGTexture::Linear : QSGTexture::Nearest);

    node->setTargetRect(targetRect);
    node->setInnerTargetRect(targetRect);
    node->setSubSourceRect(nsrect);
    node->setMirror(d->mirror);
    node->setAntialiasing(d->antialiasing);
    node->update();

    return node;
}


QSGMaterialType CoolTextureMaterialShader::type;
QSGCoolTextureMaterial::QSGCoolTextureMaterial()
{

}

QSGMaterialType *QSGCoolTextureMaterial::type() const
{
    return &CoolTextureMaterialShader::type;
}

QSGMaterialShader *QSGCoolTextureMaterial::createShader() const
{
    return new CoolTextureMaterialShader;
}

void CoolTextureMaterialShader::updateState(const QSGMaterialShader::RenderState &state,
                                            QSGMaterial *newEffect,
                                            QSGMaterial *oldEffect)
{

    QSGCoolTextureMaterial *m = static_cast<QSGCoolTextureMaterial *>(newEffect);
    program()->setUniformValue(m_invert_id, m->uniforms.invert);
    if (oldEffect == nullptr) {
        //            // The viewport is constant, so set the pixel size uniform only once.
        //            QRect r = state.viewportRect();
        //            program()->setUniformValue(m_pixelSizeLoc, 2.0f / r.width(), 2.0f / r.height());
    }
    QSGOpaqueTextureMaterialShader::updateState(state, newEffect, oldEffect);
}

const char * const *CoolTextureMaterialShader::attributeNames() const
{
    //    static char const *const attributes[] = {
    //        "vertex",
    //        "multiTexCoord",
    //        "vertexOffset",
    //        "texCoordOffset",
    //        nullptr
    //    };
    //    return attributes;
    static char const *const attr[] = {
        "qt_VertexPosition",
        "qt_VertexTexCoord",
        nullptr };
    return attr;
}

void CoolTextureMaterialShader::initialize()
{
    m_invert_id = program()->uniformLocation("invert");
    QSGOpaqueTextureMaterialShader::initialize();
}

