/* Copyright (c) 2015, EPFL/Blue Brain Project
 *                     Daniel.Nachbaur@epfl.ch
 *
 * This file is part of Deflect <https://github.com/BlueBrain/Deflect>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "DesktopWindowsModel.h"

#include <QApplication>
#include <QDebug>
#include <QtMac>
#include <QPixmap>
#include <QScreen>

#import <AppKit/NSWorkspace.h>
#import <AppKit/NSRunningApplication.h>
#import <Foundation/Foundation.h>

namespace
{
const int PREVIEWIMAGEWIDTH = 200;
const int PREVIEWIMAGEHEIGHT = PREVIEWIMAGEWIDTH * 0.6;

/**
 * Based on: http://www.qtcentre.org/threads/34752-NSString-to-QString
 */
QString NSStringToQString( const NSString* nsstr )
{
    NSRange range;
    range.location = 0;
    range.length = [nsstr length];
    unichar* chars = new unichar[range.length];
    [nsstr getCharacters:chars range:range];
    QString result = QString::fromUtf16( chars, range.length );
    delete [] chars;
    return result;
}

/**
 * Based on https://github.com/quicksilver/UIAccess-qsplugin/blob/a31107764a9f9951173b326441d46f62b7644dd0/QSUIAccessPlugIn_Action.m#L109
 */
NSArray* getWindows( NSRunningApplication* app, const CFArrayRef& windowList )
{
    __block NSMutableArray* windows = [NSMutableArray array];
    [(NSArray*)windowList enumerateObjectsWithOptions:NSEnumerationConcurrent
                           usingBlock:^( NSDictionary* info, NSUInteger, BOOL* )
    {
        const int pid = [(NSNumber*)[info objectForKey:(NSString *)kCGWindowOwnerPID] intValue];
        if( pid == [app processIdentifier])
            [windows addObject:info];
    }];
    return [[windows copy] autorelease];
}

QPixmap getPreviewPixmap( const QPixmap& pixmap )
{
    return QPixmap::fromImage(
                pixmap.toImage().scaled( PREVIEWIMAGEWIDTH, PREVIEWIMAGEHEIGHT,
                                         Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation ));
}

QPixmap getWindowPixmap( const CGWindowID windowID )
{
    const CGImageRef windowImage =
            CGWindowListCreateImage( CGRectNull,
                                     kCGWindowListOptionIncludingWindow,
                                     windowID,
                                     kCGWindowImageBoundsIgnoreFraming );

    const QPixmap pixmap = QtMac::fromCGImageRef( windowImage );
    CGImageRelease( windowImage );
    return pixmap;
}

QRect getWindowRect( const CGWindowID windowID )
{
    const CGWindowID windowids[1] = { windowID };
    const CFArrayRef windowIDs = CFArrayCreate( kCFAllocatorDefault,
                                         (const void **)windowids, 1, nullptr );
    CFArrayRef windowList = CGWindowListCreateDescriptionFromArray( windowIDs );
    CFRelease( windowIDs );

    if( CFArrayGetCount( windowList ) == 0 )
        return QRect();

    const CFDictionaryRef info =
            (CFDictionaryRef)CFArrayGetValueAtIndex( windowList, 0 );
    CFDictionaryRef bounds = (CFDictionaryRef)CFDictionaryGetValue( info,
                                                              kCGWindowBounds );
    CGRect rect;
    CGRectMakeWithDictionaryRepresentation( bounds, &rect );
    CFRelease( windowList );

    return QRect( CGRectGetMinX( rect ), CGRectGetMinY( rect ),
                  CGRectGetWidth( rect ), CGRectGetHeight( rect ));
}

} // namespace


@interface AppObserver : NSObject
{
    DesktopWindowsModel* parent;
}
- (void)setParent:(DesktopWindowsModel*)parent_;
@end

@implementation AppObserver

- (id) init
{
    self = [super init];

    NSWorkspace *workspace = [NSWorkspace sharedWorkspace];
    NSNotificationCenter* center = [workspace notificationCenter];
    [center addObserver:self
            selector:@selector(newApplication:)
            name:NSWorkspaceDidLaunchApplicationNotification
            object:Nil];
    [center addObserver:self
            selector:@selector(closedApplication:)
            name:NSWorkspaceDidTerminateApplicationNotification
            object:Nil];
    [center addObserver:self
            selector:@selector(newApplication:)
            name:NSWorkspaceDidUnhideApplicationNotification
            object:Nil];
    [center addObserver:self
            selector:@selector(closedApplication:)
            name:NSWorkspaceDidHideApplicationNotification
            object:Nil];

    return self;
}

- (void) dealloc
{
    NSWorkspace *workspace = [NSWorkspace sharedWorkspace];
    NSNotificationCenter* center = [workspace notificationCenter];
    [center removeObserver:self
            name:NSWorkspaceDidLaunchApplicationNotification
            object:Nil];
    [center removeObserver:self
            name:NSWorkspaceDidTerminateApplicationNotification
            object:Nil];
    [center removeObserver:self
            name:NSWorkspaceDidUnhideApplicationNotification
            object:Nil];
    [center removeObserver:self
            name:NSWorkspaceDidHideApplicationNotification
            object:Nil];

    [super dealloc];
}

- (void)setParent:(DesktopWindowsModel*)parent_
{
    parent = parent_;
}

- (bool)isCurrent:(NSRunningApplication*)app_
{
    NSRunningApplication* current = [NSRunningApplication currentApplication];
    return [app_ processIdentifier] == [current processIdentifier];
}

- (void)newApplication:(NSNotification*) notification
{
    NSRunningApplication* app =
            [[notification userInfo] objectForKey:@"NSWorkspaceApplicationKey"];

    if( ![self isCurrent:app] )
        parent->addApplication( app );
}

- (void)closedApplication:(NSNotification*) notification
{
    NSRunningApplication* app =
            [[notification userInfo] objectForKey:@"NSWorkspaceApplicationKey"];

    if( ![self isCurrent:app] )
        parent->removeApplication( app );
}

@end


class DesktopWindowsModel::Impl
{
public:
    Impl( DesktopWindowsModel& parent )
        : _parent( parent )
        , _observer( [[AppObserver alloc] init] )
    {
        [_observer setParent:&parent];
        reloadData();
    }

    ~Impl()
    {
        [_observer release];
    }

    enum TupleValues
    {
        APPNAME,
        WINDOWID,
        WINDOWIMAGE,
        PID
    };

    void addApplication( NSRunningApplication* app )
    {
        _parent.beginInsertRows( QModelIndex(), _data.size(), _data.size( ));
        for( ;; )
        {
            CFArrayRef windowList =
                CGWindowListCopyWindowInfo( kCGWindowListOptionOnScreenOnly|
                                            kCGWindowListExcludeDesktopElements,
                                            kCGNullWindowID );
            if( _addApplication( app, windowList ))
            {
                CFRelease( windowList );
                break;
            }

            CFRelease( windowList );

            // notification is not sync'd with CGWindowListCopyWindowInfo
            sleep( 1 );
        }
        _parent.endInsertRows();
    }

    void removeApplication( NSRunningApplication* app )
    {
        const QString& appName = NSStringToQString([app localizedName]);
        const auto& i = std::find_if( _data.begin(), _data.end(),
                                      [&]( const Data::value_type& entry )
            { return std::get< Impl::APPNAME >( entry ) == appName; } );
        _parent.beginRemoveRows( QModelIndex(), i - _data.begin(),
                                 i - _data.begin( ));
        _data.erase( i );
        _parent.endRemoveRows();
    }

    void reloadData()
    {
        _parent.beginResetModel();

        NSWorkspace* workspace = [NSWorkspace sharedWorkspace];
        NSArray* runningApplications = [workspace runningApplications];

        CFArrayRef windowList =
                CGWindowListCopyWindowInfo( kCGWindowListOptionOnScreenOnly |
                                            kCGWindowListExcludeDesktopElements,
                                            kCGNullWindowID );

        _data.clear();
        const QPixmap preview =
                getPreviewPixmap( QApplication::primaryScreen()->grabWindow(0));
        _data.push_back( std::make_tuple( "Desktop", 0, preview, 0 ));

        for( NSRunningApplication* app in runningApplications )
            _addApplication( app, windowList );

        CFRelease( windowList );
        _parent.endResetModel();
    }

    DesktopWindowsModel& _parent;
    typedef std::vector< std::tuple<
                QString, CGWindowID, QPixmap, int > > Data;
    Data _data;
    AppObserver* _observer;

private:
    bool _addApplication( NSRunningApplication* app, CFArrayRef windowList )
    {
        const QString& appName = NSStringToQString([app localizedName]);
        const auto& pid = [app processIdentifier];
        if( appName == "SystemUIServer" || appName == "Dock" ||
            pid == QApplication::applicationPid( ))
        {
           return true;
        }

        NSArray* windows = getWindows( app, windowList );

        bool gotOne = false;
        for( NSDictionary* info in windows )
        {
           const QString& windowName = NSStringToQString(
                             [info objectForKey:(NSString*)kCGWindowName]);

           const int windowLayer =
                  [[info objectForKey:(NSString*)kCGWindowLayer] intValue];
           const CGWindowID windowID =
                   [[info objectForKey:(NSString*)kCGWindowNumber]
                     unsignedIntValue];
           const QRect& rect = getWindowRect( windowID );
           if( rect.width() <= 1 || rect.height() <= 1 || windowLayer != 0 )
           {
               if( windowName.isEmpty( ))
                   qDebug() << "Ignoring" << appName;
               else
                   qDebug() << "Ignoring" << appName << "-" << windowName;
               continue;
           }
           _data.push_back( std::make_tuple( appName, windowID,
                         getPreviewPixmap( getWindowPixmap( windowID )), pid ));
           gotOne = true;
        }
        return gotOne;
    }
};

DesktopWindowsModel::DesktopWindowsModel()
    : QAbstractListModel()
    , _impl( new Impl( *this ))
{
}

int DesktopWindowsModel::rowCount( const QModelIndex& ) const
{
    return int( _impl->_data.size( ));
}

QVariant DesktopWindowsModel::data( const QModelIndex& index, int role ) const
{
    const auto& data = _impl->_data[index.row()];
    switch( role )
    {
    case Qt::DisplayRole:
        return QString("%1").arg( std::get< Impl::APPNAME >( data ));

    case Qt::DecorationRole:
        return std::get< Impl::WINDOWIMAGE >( data );

    case ROLE_PIXMAP:
        return getWindowPixmap( std::get< Impl::WINDOWID >( data ));

    case ROLE_RECT:
        return getWindowRect( std::get< Impl::WINDOWID >( data ));

    case ROLE_PID:
        return std::get< Impl::PID >( data );

    default:
        return QVariant();
    }
}

bool DesktopWindowsModel::isActive( const int pid )
{
    // A pid of 0 means the full desktop, always active
    return [[NSRunningApplication
            runningApplicationWithProcessIdentifier: pid] isActive] || pid == 0;
}

void DesktopWindowsModel::activate( const int pid )
{
    NSRunningApplication* app = [NSRunningApplication
                                 runningApplicationWithProcessIdentifier: pid];
    [app activateWithOptions: NSApplicationActivateAllWindows |
                              NSApplicationActivateIgnoringOtherApps];
}

void DesktopWindowsModel::addApplication( void* app )
{
    _impl->addApplication( (NSRunningApplication*)app );
}

void DesktopWindowsModel::removeApplication( void* app )
{
    _impl->removeApplication( (NSRunningApplication*)app );
}
