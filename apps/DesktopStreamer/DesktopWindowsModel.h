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

#ifndef DESKTOPSTREAMER_DESKTOPWINDOWSMODEL_H
#define DESKTOPSTREAMER_DESKTOPWINDOWSMODEL_H

#include <QAbstractListModel>
#include <memory>

/**
 * Item Model which contains all open applications on OSX and updates
 * accordingly to new and closed applications.
 */
class DesktopWindowsModel : public QAbstractListModel
{
public:
    DesktopWindowsModel();

    int rowCount(const QModelIndex&) const final;

    /** @return application name for Qt::DisplayRole, window preview pixmap for
     *          Qt::DecorationRole, original size pixmap for ROLE_PIXMAP, window
     *          rectangle for ROLE_RECT
     */
    QVariant data(const QModelIndex& index, int role) const final;

    enum DataRole
    {
        ROLE_PIXMAP = Qt::UserRole,
        ROLE_RECT,
        ROLE_PID
    };

    /** @internal */
    void addApplication(void* app);

    /** @internal */
    void removeApplication(void* app);

    /**
     * Check if the application corresponding to the specified PID is currently
     * active
     *
     * @param pid the application process id
     * @return true if the application with the PID specified is currently
     *         active. A pid of 0 means the full desktop, always active
     */
    static bool isActive(int pid);

    /**
     * Activate the application corresponding to the specified PID. This will
     * send the application to the front
     *
     * @param pid the application process id
     */
    static void activate(int pid);

private:
    class Impl;
    std::unique_ptr<Impl> _impl;
};

#endif
