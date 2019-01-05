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

#ifndef FLICKABLEGESTUREAREA_H
#define FLICKABLEGESTUREAREA_H

#include <QQuickItem>
#include <QLineF>
#include <QDebug>
#include <QDateTime>

class FlickableGestureArea : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(qreal scale READ scale WRITE setScale NOTIFY scaleChanged)
    Q_PROPERTY(qreal maximumScale        MEMBER mMax)
    Q_PROPERTY(qreal minimumScale        MEMBER mMin)
    Q_PROPERTY(QPointF centroid     MEMBER mCentroid)
    Q_PROPERTY(bool active          MEMBER mActive)
    Q_PROPERTY(bool wheeled          MEMBER mWheeled)

public:
    FlickableGestureArea(QQuickItem *parent = 0) : QQuickItem(parent)
    {
        setAcceptedMouseButtons(Qt::LeftButton);
        setAcceptTouchEvents(true);
        setFiltersChildMouseEvents(true);
    }

    qreal scale() const
    {
        return mFactor;
    }

    void setScale(qreal scale)
    {
        scale = qBound(mMin, scale, mMax);
        if (scale == mFactor)
            return;
        mFactor = mCurrentFactor = scale;
        emit scaleChanged();
    }

signals:
    void scaleChanged();
    void clicked();
    void doubleClicked();
    void centroidChanged();
    void activeChanged();
    void wheeledChanged();

public slots:


protected slots:

protected:

    void componentComplete() override
    {
        QQuickItem::componentComplete();
    }

    inline void zoom(const qreal& f)
    {
        qreal newFactor = qBound(mMin, mCurrentFactor * f, mMax);
        if (newFactor == mFactor)
            return;
        mFactor = newFactor;
        emit scaleChanged();
    }


    void touchEvent(QTouchEvent *touchEvent) override {
        switch (touchEvent->type()) {
        case QTouchEvent::TouchBegin:
        case QTouchEvent::TouchUpdate: // only getting touchUpdate!
        case QTouchEvent::TouchEnd: {
            QList<QTouchEvent::TouchPoint> touchPoints = touchEvent->touchPoints();
            // 1 touch point = handle with MouseEvent (event is always synthesized)
            // let the synthesized mouse event grab the mouse,
            // note there is no mouse grabber at this point since
            // touch event comes first (see Qt::AA_SynthesizeMouseForUnhandledTouchEvents)
//            qWarning() << "Type: "<<touchEvent->type() << " cnt: "<<touchPoints.count();
            if (touchPoints.count() == 2) {
                const QTouchEvent::TouchPoint &touchPoint0 = touchPoints.first();
                const QTouchEvent::TouchPoint &touchPoint1 = touchPoints.last();
                const QPointF centroid = (touchPoint0.pos() + touchPoint1.pos()) * 0.5;
                const qreal startLength = QLineF(touchPoint0.startPos(), touchPoint1.startPos()).length();
                const qreal currentLength = QLineF(touchPoint0.pos(), touchPoint1.pos()).length();
                const qreal s = currentLength / startLength;


                if (centroid != mCentroid) {
                    mCentroid = centroid;
                    emit centroidChanged();
                }

                if (touchEvent->touchPointStates() & Qt::TouchPointReleased) {
//                    qWarning() << "Storing mCurrentFactor "<< mCurrentFactor;
                    mCurrentFactor = mFactor;
                } else {
                    zoom(s);
                }


                if (touchEvent->type() == QTouchEvent::TouchBegin ||
                        touchEvent->type() == QTouchEvent::TouchUpdate) {
                    if (!mActive) {
                        QVector<int> ids;
                        ids << touchPoint0.id() << touchPoint1.id();
                        setKeepTouchGrab(true);
                        setKeepMouseGrab(true);
                        grabMouse();
                        grabTouchPoints(ids);
                        mActive = true;
                        emit activeChanged();
//                        qWarning() << "Touch Begins " << "initial scale: "<<s;
                    }
                }
            } else if (touchPoints.count() == 1) {
                if (touchEvent->type() == QTouchEvent::TouchBegin)
                    tapOne();
            }
            if (touchEvent->type() != QTouchEvent::TouchEnd) {
                mPointsInLastEvent = touchPoints.count();
            } else if (mActive)  {
                mActive = false;
                emit activeChanged();
                ungrabTouchPoints();
                ungrabMouse();
                setKeepTouchGrab(false);
                setKeepMouseGrab(false);
//                qWarning() << "Touch Ends";
            }
        }
        default:
            break;
        }
    }

    void wheelEvent(QWheelEvent* wheelEvent) override
    {
        //mCurrentFactor = zoom(mCurrentFactor + (wheelEvent->angleDelta().y() > 0? 1:-1) * mWheelFactor);
        qWarning() << "WheelEvent mods:"<<wheelEvent->modifiers();
        if (wheelEvent->modifiers().testFlag(Qt::ControlModifier)) {

            mActive = true;
            emit activeChanged();
            mWheeled = true;
            emit wheeledChanged();

            double s = wheelEvent->delta() / 120.0;
            QPointF centroid = wheelEvent->posF();
            qDebug() << centroid;
            if (centroid != mCentroid) {
                mCentroid = centroid;
                emit centroidChanged();
            }


            zoom(1.0 + s * 0.1);




            mCurrentFactor = mFactor;
            mActive = false;
            emit activeChanged();

            wheelEvent->accept();
            return;
        }
        QQuickItem::wheelEvent(wheelEvent);
    }

    void tapOne()
    {
        qint64 msec = QDateTime::currentMSecsSinceEpoch();
        if (msec - mLastPress < 200 && mPointsInLastEvent ==1) {
            emit doubleClicked();
            mLastPress = 0;
        } else {
            mLastPress = msec;
        }
    }

    void clickOne()
    {
        qint64 msec = QDateTime::currentMSecsSinceEpoch();
        if (msec - mLastPress < 200) {
            emit doubleClicked();
            mLastPress = 0;
        } else {
            mLastPress = msec;
        }
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        clickOne();
        QQuickItem::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        QQuickItem::mouseReleaseEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        QQuickItem::mouseMoveEvent(event);
    }

    void mouseUngrabEvent() override
    {
        QQuickItem::mouseUngrabEvent();
    }

    void mouseDoubleClickEvent(QMouseEvent *event) override
    {
//        emit doubleClicked(); // doesn't work apparently
        QQuickItem::mouseDoubleClickEvent(event);
    }

    void touchUngrabEvent() override
    {
        QQuickItem::touchUngrabEvent();
    }



    qint64      mLastPress;
    QPointF     mCentroid;
    qreal       mCurrentFactor = 1;
    qreal       mFactor = 1;
    qreal       mMax = 8;
    qreal       mMin = 1;
    int         mPointsInLastEvent = 0;
    bool        mActive = false;
    bool        mWheeled = false;
};


#endif // FLICKABLEGESTUREAREA_H
