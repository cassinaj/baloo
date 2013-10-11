/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright 2013  Vishesh Handa <me@vhanda.in>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef TAGCREATEJOB_H
#define TAGCREATEJOB_H

#include "itemcreatejob.h"

class Tag;

class TagCreateJob : public ItemCreateJob
{
    Q_OBJECT
public:
    TagCreateJob(Tag* tag, QObject* parent = 0);

    virtual void start();

    enum Error {
        Error_ConnectionError = 1,
        Error_TagExists = 2,
        Error_TagIdProvided = 3,
        Error_TagEmptyName = 4
    };

signals:
    void tagCreated(Tag* tag);

private slots:
    void doStart();

private:
    Tag* m_tag;
};

#endif // TAGCREATEJOB_H