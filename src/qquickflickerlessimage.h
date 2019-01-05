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

#ifndef QQUICKFLICKERLESSIMAGE_H
#define QQUICKFLICKERLESSIMAGE_H

#include <private/qquickimplicitsizeitem_p.h>
#include <private/qquickimplicitsizeitem_p_p.h>
#include <private/qquickpixmapcache_p.h>
#include <private/qtquickglobal_p.h>
#include <QtQuick/qsgtextureprovider.h>
#include <QtQuick/qsgtexturematerial.h>
#include <private/qsgdefaultinternalimagenode_p.h>
#include <private/qsgmaterialshader_p.h>
#include <private/qsgtexturematerial_p.h>
#include <private/qsgdefaultrendercontext_p.h>

class QNetworkReply;

class QQuickImageBasePrivate;
class QQuickImageBase : public QQuickImplicitSizeItem
{
    Q_OBJECT

    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(bool asynchronous READ asynchronous WRITE setAsynchronous NOTIFY asynchronousChanged)
    Q_PROPERTY(bool cache READ cache WRITE setCache NOTIFY cacheChanged)
    Q_PROPERTY(QSize sourceSize READ sourceSize WRITE setSourceSize RESET resetSourceSize NOTIFY sourceSizeChanged)
    Q_PROPERTY(bool mirror READ mirror WRITE setMirror NOTIFY mirrorChanged)

public:
    QQuickImageBase(QQuickItem *parent=nullptr);
    ~QQuickImageBase();
    enum Status { Null, Ready, Loading, Error };
    Q_ENUM(Status)
    Status status() const;
    qreal progress() const;

    QUrl source() const;
    virtual void setSource(const QUrl &url);

    bool asynchronous() const;
    void setAsynchronous(bool);

    bool cache() const;
    void setCache(bool);

    QImage image() const;

    virtual void setSourceSize(const QSize&);
    QSize sourceSize() const;
    void resetSourceSize();

    virtual void setMirror(bool mirror);
    bool mirror() const;

    virtual void setAutoTransform(bool transform);
    bool autoTransform() const;

    static void resolve2xLocalFile(const QUrl &url, qreal targetDevicePixelRatio, QUrl *sourceUrl, qreal *sourceDevicePixelRatio);

    // Use a virtual rather than a signal->signal to avoid the huge
    // connect/conneciton overhead for this rare case.
    virtual void emitAutoTransformBaseChanged() { }

Q_SIGNALS:
    void sourceChanged(const QUrl &);
    void sourceSizeChanged();
    void statusChanged(QQuickImageBase::Status);
    void progressChanged(qreal progress);
    void asynchronousChanged();
    void cacheChanged();
    void mirrorChanged();

protected:
    virtual void load();
    void componentComplete() override;
    virtual void pixmapChange();
    void itemChange(ItemChange change, const ItemChangeData &value) override;
    QQuickImageBase(QQuickImageBasePrivate &dd, QQuickItem *parent);

private Q_SLOTS:
    virtual void requestFinished();
    void requestProgress(qint64,qint64);

private:
    Q_DISABLE_COPY(QQuickImageBase)
    Q_DECLARE_PRIVATE(QQuickImageBase)
};

class QQuickImageBasePrivate : public QQuickImplicitSizeItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickImageBase)

public:
    QQuickImageBasePrivate()
      : status(QQuickImageBase::Null),
        progress(0.0),
        devicePixelRatio(1.0),
        async(false),
        cache(true),
        mirror(false),
        oldAutoTransform(false)
    {
        pix = &front;
        pixLoading = pix; // set it to a different pixmap to enable double buffering;
    }

    virtual bool updateDevicePixelRatio(qreal targetDevicePixelRatio);
    void swapPixBuffers()
    {
        QQuickPixmap *tmp = pix;
        pix = pixLoading;
        pixLoading = tmp;
    }

    QQuickPixmap front;
    QQuickPixmap *pix;
    QQuickPixmap *pixLoading;
    QQuickImageBase::Status status;
    QUrl url;
    qreal progress;
    QSize sourcesize;
    QSize oldSourceSize;
    qreal devicePixelRatio;
    QQuickImageProviderOptions providerOptions;
    bool async : 1;
    bool cache : 1;
    bool mirror: 1;
    bool oldAutoTransform : 1;
};

class QQuickImagePrivate;
class Q_QUICK_PRIVATE_EXPORT QQuickImage : public QQuickImageBase
{
    Q_OBJECT

    Q_PROPERTY(FillMode fillMode READ fillMode WRITE setFillMode NOTIFY fillModeChanged)
    Q_PROPERTY(qreal paintedWidth READ paintedWidth NOTIFY paintedGeometryChanged)
    Q_PROPERTY(qreal paintedHeight READ paintedHeight NOTIFY paintedGeometryChanged)
    Q_PROPERTY(HAlignment horizontalAlignment READ horizontalAlignment WRITE setHorizontalAlignment NOTIFY horizontalAlignmentChanged)
    Q_PROPERTY(VAlignment verticalAlignment READ verticalAlignment WRITE setVerticalAlignment NOTIFY verticalAlignmentChanged)
    Q_PROPERTY(bool mipmap READ mipmap WRITE setMipmap NOTIFY mipmapChanged)
    Q_PROPERTY(bool autoTransform READ autoTransform WRITE setAutoTransform NOTIFY autoTransformChanged)

public:
    QQuickImage(QQuickItem *parent=nullptr);
    ~QQuickImage();

    enum HAlignment { AlignLeft = Qt::AlignLeft,
                       AlignRight = Qt::AlignRight,
                       AlignHCenter = Qt::AlignHCenter };
    Q_ENUM(HAlignment)
    enum VAlignment { AlignTop = Qt::AlignTop,
                       AlignBottom = Qt::AlignBottom,
                       AlignVCenter = Qt::AlignVCenter };
    Q_ENUM(VAlignment)

    enum FillMode { Stretch, PreserveAspectFit, PreserveAspectCrop, Tile, TileVertically, TileHorizontally, Pad };
    Q_ENUM(FillMode)

    FillMode fillMode() const;
    void setFillMode(FillMode);

    qreal paintedWidth() const;
    qreal paintedHeight() const;

    QRectF boundingRect() const override;

    HAlignment horizontalAlignment() const;
    void setHorizontalAlignment(HAlignment align);

    VAlignment verticalAlignment() const;
    void setVerticalAlignment(VAlignment align);

    bool isTextureProvider() const override { return true; }
    QSGTextureProvider *textureProvider() const override;

    bool mipmap() const;
    void setMipmap(bool use);

    void emitAutoTransformBaseChanged() override { emit autoTransformChanged(); }

Q_SIGNALS:
    void fillModeChanged();
    void paintedGeometryChanged();
    void horizontalAlignmentChanged(HAlignment alignment);
    void verticalAlignmentChanged(VAlignment alignment);
    void mipmapChanged(bool);
    void autoTransformChanged();

private Q_SLOTS:
    void invalidateSceneGraph();

protected:
    QQuickImage(QQuickImagePrivate &dd, QQuickItem *parent);
    void pixmapChange() override;
    virtual void updatePaintedGeometry();
    void releaseResources() override;

    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;

private:
    Q_DISABLE_COPY(QQuickImage)
    Q_DECLARE_PRIVATE(QQuickImage)
};


class QQuickImageTextureProvider : public QSGTextureProvider
{
    Q_OBJECT
public:
    QQuickImageTextureProvider();

    void updateTexture(QSGTexture *texture);

    QSGTexture *texture() const override ;

    friend class QQuickImage;

    QSGTexture *m_texture;
    bool m_smooth;
    bool m_mipmap;
};

class QQuickImagePrivate : public QQuickImageBasePrivate
{
    Q_DECLARE_PUBLIC(QQuickImage)

public:
    QQuickImagePrivate();

    QQuickImage::FillMode fillMode;
    qreal paintedWidth;
    qreal paintedHeight;
    void setImage(const QImage &img);
    void setPixmap(const QQuickPixmap &pixmap);

    bool pixmapChanged : 1;
    bool mipmap : 1;
    QQuickImage::HAlignment hAlign;
    QQuickImage::VAlignment vAlign;

    QQuickImageTextureProvider *provider;
};

class CoolTextureMaterialShader : public QSGOpaqueTextureMaterialShader
{
public:
    CoolTextureMaterialShader(): QSGOpaqueTextureMaterialShader()
    {
        setShaderSourceFile(QOpenGLShader::Vertex, QStringLiteral(":/assets/shaders/opaquetexture_vert.glsl"));
        setShaderSourceFile(QOpenGLShader::Fragment, QStringLiteral(":/assets/shaders/texture_frag.glsl"));
    }

    void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect) override;
    char const *const *attributeNames() const override;

    static QSGMaterialType type;

protected:
    void initialize() override;

    int m_invert_id;
};


class QSGCoolTextureMaterial : public QSGTextureMaterial
{
public:
    struct GLImageNodePlusUniforms
    {
        bool invert = false;
    } uniforms;

    QSGCoolTextureMaterial();

//    void setTexture(QSGTexture *texture) // this does not override, but overshadows
//    {
//        m_texture = texture;
//        setFlag(Blending, true);
//    }

protected:
    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader() const override;
};

class QSGDefaultInternalImageNodePlus : public QSGDefaultInternalImageNode
{
public:
    QSGDefaultInternalImageNodePlus(QSGDefaultRenderContext *rc): QSGDefaultInternalImageNode(rc)
    {
        setMaterial(&m_material);
//        setOpaqueMaterial(&m_material);
        setOpaqueMaterial(nullptr);
    }

    void setMipmapFiltering(QSGTexture::Filtering filtering) override
    {
        if (m_material.mipmapFiltering() == filtering)
            return;

        m_material.setMipmapFiltering(filtering);
        markDirty(DirtyMaterial);
    }
    void setFiltering(QSGTexture::Filtering filtering) override
    {
        if (m_material.filtering() == filtering)
            return;

        m_material.setFiltering(filtering);
        markDirty(DirtyMaterial);
    }
    void setHorizontalWrapMode(QSGTexture::WrapMode wrapMode) override
    {
        if (m_material.horizontalWrapMode() == wrapMode)
            return;

        m_material.setHorizontalWrapMode(wrapMode);
        markDirty(DirtyMaterial);
    }
    void setVerticalWrapMode(QSGTexture::WrapMode wrapMode) override
    {
        if (m_material.verticalWrapMode() == wrapMode)
            return;

        m_material.setVerticalWrapMode(wrapMode);
        markDirty(DirtyMaterial);
    }

    void updateMaterialAntialiasing() override
    {
//        if (m_antialiasing) {
//            setMaterial(&m_smoothMaterial);
//            setOpaqueMaterial(nullptr);
//        } else {
//            setMaterial(&m_materialO);
//            setOpaqueMaterial(&m_material);
//        }
    }
    void setMaterialTexture(QSGTexture *texture) override
    {
        m_material.setTexture(texture);
    }
    QSGTexture *materialTexture() const override
    {
        return m_material.texture();
    }
    bool updateMaterialBlending() override
    {
        const bool alpha = m_material.flags() & QSGMaterial::Blending;
        if (materialTexture() && alpha != materialTexture()->hasAlphaChannel()) {
            m_material.setFlag(QSGMaterial::Blending, !alpha);
            return true;
        }
        return false;
    }
    bool supportsWrap(const QSize &size) const override
    {
        return QSGDefaultInternalImageNode::supportsWrap(size);
    }


    void setUniforms(const QSGCoolTextureMaterial::GLImageNodePlusUniforms &uniforms)
    {
        m_material.uniforms = uniforms;
        markDirty(DirtyMaterial);
    }


private:
    QSGCoolTextureMaterial m_material;
//    QSGTextureMaterial m_material;
};

class QQuickFlickerlessImagePrivate: public QQuickImagePrivate
{
public:
    QQuickFlickerlessImagePrivate()
    {
        pixLoading = &back;
    }
    ~QQuickFlickerlessImagePrivate() {}

    QQuickPixmap back;

};

class QQuickFlickerlessImage : public QQuickImage
{
    Q_OBJECT

    Q_PROPERTY(bool invert READ invert WRITE setInvert NOTIFY invertChanged)
public:
    QQuickFlickerlessImage(QQuickItem *parent=nullptr) : QQuickImage(*(new QQuickFlickerlessImagePrivate), parent)
    {
    }
    ~QQuickFlickerlessImage() {}

    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;

    bool invert() const
    {
        return m_uniforms.invert;
    }
    void setInvert(bool invert)
    {
        if (invert == m_uniforms.invert)
            return;
        m_uniforms.invert = invert;
        update();
        emit invertChanged();
    }

    QSGCoolTextureMaterial::GLImageNodePlusUniforms m_uniforms;
Q_SIGNALS:
    void invertChanged();

private:
    Q_DISABLE_COPY(QQuickFlickerlessImage)
    Q_DECLARE_PRIVATE(QQuickFlickerlessImage)
};

#endif // QQUICKFLICKERLESSIMAGE_H
