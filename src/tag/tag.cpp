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

#include "tag.h"

Tag::Tag(const QByteArray& id)
{
    m_id = id;

    qRegisterMetaType<Tag*>();
}

Tag::Tag(const QString& name)
{
    m_name = name;
    m_id = 0;
}

QByteArray Tag::id()
{
    return m_id;
}

QString Tag::name()
{
    return m_name;
}

void Tag::setName(const QString& name)
{
    m_name = name;
}

QByteArray Tag::type()
{
    return QByteArray("Tag");
}

TagFetchJob* Tag::fetch()
{
    return new TagFetchJob(this);
}
