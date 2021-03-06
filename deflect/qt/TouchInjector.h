/*********************************************************************/
/* Copyright (c) 2016, EPFL/Blue Brain Project                       */
/*                     Raphael Dumusc <raphael.dumusc@epfl.ch>       */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE  IS  PROVIDED  BY  THE  ECOLE  POLYTECHNIQUE    */
/*    FEDERALE DE LAUSANNE  ''AS IS''  AND ANY EXPRESS OR IMPLIED    */
/*    WARRANTIES, INCLUDING, BUT  NOT  LIMITED  TO,  THE  IMPLIED    */
/*    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR  A PARTICULAR    */
/*    PURPOSE  ARE  DISCLAIMED.  IN  NO  EVENT  SHALL  THE  ECOLE    */
/*    POLYTECHNIQUE  FEDERALE  DE  LAUSANNE  OR  CONTRIBUTORS  BE    */
/*    LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,    */
/*    EXEMPLARY,  OR  CONSEQUENTIAL  DAMAGES  (INCLUDING, BUT NOT    */
/*    LIMITED TO,  PROCUREMENT  OF  SUBSTITUTE GOODS OR SERVICES;    */
/*    LOSS OF USE, DATA, OR  PROFITS;  OR  BUSINESS INTERRUPTION)    */
/*    HOWEVER CAUSED AND  ON ANY THEORY OF LIABILITY,  WHETHER IN    */
/*    CONTRACT, STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE    */
/*    OR OTHERWISE) ARISING  IN ANY WAY  OUT OF  THE USE OF  THIS    */
/*    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.   */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of Ecole polytechnique federale de Lausanne.          */
/*********************************************************************/

#ifndef DELFECT_QT_TOUCHINJECTOR_H
#define DELFECT_QT_TOUCHINJECTOR_H

#include <QObject>
#include <QTouchEvent>

#include <functional>
#include <memory>

class QWindow;

namespace deflect
{
namespace qt
{
/**
 * Inject complete QTouchEvent from separate touch added/updated/removed events.
 */
class TouchInjector : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(TouchInjector)

public:
    /** Function to map normalized coordinates to scene / window coordinates. */
    using MapToSceneFunc = std::function<QPointF(const QPointF&)>;

    /**
     * Constructor.
     * @param target the target QObject that should receive the touch events
     * @param mapFunc the function to generate scene / window coordinates
     */
    TouchInjector(QObject& target, MapToSceneFunc mapFunc);

    /**
     * Create a touch injector for a QWindow.
     * @param window the target window that should receive the touch events
     */
    static std::unique_ptr<TouchInjector> create(QWindow& window);

    /**
     * Insert a new touch point.
     *
     * Does nothing if a point with the same id already exists.
     * @param id the identifier for the point
     * @param position the initial normalized position of the point
     */
    void addTouchPoint(int id, QPointF position);

    /**
     * Update an existing touch point.
     *
     * Does nothing if the given point has not been added or was removed.
     * @param id the identifier for the point
     * @param position the new normalized position of the point
     */
    void updateTouchPoint(int id, QPointF position);

    /**
     * Remove an existing touch point.
     *
     * Does nothing if the given point has not been added or was removed.
     * @param id the identifier for the point
     * @param position the new normalized position of the point
     */
    void removeTouchPoint(int id, QPointF position);

    /** Remove all touch points. */
    void removeAllTouchPoints();

private:
    void _handleEvent(const int id, const QPointF& normalizedPos,
                      const QEvent::Type eventType);

    QObject& _target;
    MapToSceneFunc _mapToSceneFunction;
    QTouchDevice _device;
    QMap<int, QTouchEvent::TouchPoint> _touchPointMap;
};
}
}

#endif
