/*********************************************************************/
/* Copyright (c) 2011 - 2012, The University of Texas at Austin.     */
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
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/

#include "DesktopSelectionRectangle.h"

#include <cmath>

#define PEN_WIDTH 10 // should be even
#define CORNER_RESIZE_THRESHOLD 50
#define DEFAULT_SIZE 100

DesktopSelectionRectangle::DesktopSelectionRectangle()
    : _resizing( false )
{
    setFlag( QGraphicsItem::ItemIsMovable, true );
    setPen( QPen( QBrush( QColor( 255, 0, 0 )), PEN_WIDTH ));
    setCoordinates( QRect( 0, 0, DEFAULT_SIZE, DEFAULT_SIZE ));
}

void DesktopSelectionRectangle::paint( QPainter* painter,
                                       const QStyleOptionGraphicsItem* option,
                                       QWidget* widget )
{
    QGraphicsRectItem::paint( painter, option, widget );
}

void DesktopSelectionRectangle::setCoordinates( const QRect& coordinates )
{
    _coordinates = coordinates;

    setRect( mapRectFromScene( _coordinates.x() - PEN_WIDTH/2,
                               _coordinates.y() - PEN_WIDTH/2,
                               _coordinates.width() + PEN_WIDTH,
                               _coordinates.height() + PEN_WIDTH ));
}

void DesktopSelectionRectangle::mouseMoveEvent( QGraphicsSceneMouseEvent*
                                                mouseEvent )
{
    if( mouseEvent->buttons().testFlag( Qt::LeftButton ))
    {
        if( _resizing )
        {
            QRectF r = rect();
            r.setBottomRight( mouseEvent->pos( ));
            setRect( r );
        }
        else
        {
            QPointF delta = mouseEvent->pos() - mouseEvent->lastPos();
            moveBy( delta.x(), delta.y( ));
        }

        _updateCoordinates();
        emit coordinatesChanged( _coordinates );
    }
}

void DesktopSelectionRectangle::mousePressEvent( QGraphicsSceneMouseEvent*
                                                 mouseEvent )
{
    // item rectangle and event position
    const QPointF cornerPos = rect().bottomRight();
    const QPointF eventPos = mouseEvent->pos();

    // check to see if user clicked on the resize button
    if( std::abs( cornerPos.x() - eventPos.x( )) <= CORNER_RESIZE_THRESHOLD &&
        std::abs( cornerPos.y() - eventPos.y( )) <= CORNER_RESIZE_THRESHOLD )
    {
        _resizing = true;
    }

    QGraphicsItem::mousePressEvent( mouseEvent );
}

void DesktopSelectionRectangle::mouseReleaseEvent( QGraphicsSceneMouseEvent*
                                                   mouseEvent )
{
    _resizing = false;
    QGraphicsItem::mouseReleaseEvent( mouseEvent );
}

void DesktopSelectionRectangle::_updateCoordinates()
{
    const QRectF sceneRect = mapRectToScene( rect( ));

    _coordinates.setX( (int)sceneRect.x() + PEN_WIDTH/2 );
    _coordinates.setY( (int)sceneRect.y() + PEN_WIDTH/2 );
    _coordinates.setWidth( (int)sceneRect.width() - PEN_WIDTH );
    _coordinates.setHeight( (int)sceneRect.height() - PEN_WIDTH );
}
