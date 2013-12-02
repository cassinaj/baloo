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

#ifndef _BALOO_SEARCHSTORE_H
#define _BALOO_SEARCHSTORE_H

#include <QString>
#include <QHash>
#include <QUrl>
#include <KService>

#include "core_export.h"
#include "item.h"

namespace Baloo {

class Term;
class Query;

class BALOO_CORE_EXPORT SearchStore : public QObject
{
    Q_OBJECT
public:
    explicit SearchStore(QObject* parent = 0);
    virtual ~SearchStore();

    /**
     * Returns a list of types which can be searched for
     * in this store
     */
    virtual QStringList types() = 0;

    /**
     * Executes the particular query synchronously.
     *
     * \return Returns a integer representating the integer
     */
    virtual int exec(const Query& query) = 0;
    virtual bool next(int queryId) = 0;
    virtual void close(int queryId) = 0;

    virtual Item::Id id(int queryId) = 0;

    virtual QUrl url(int queryId);
    virtual QString text(int queryId);
    virtual QString icon(int queryId);
    virtual QString property(int queryId, const QString& propName);
};

}

#define BALOO_EXPORT_SEARCHSTORE( classname, libname )    \
    K_PLUGIN_FACTORY(factory, registerPlugin<classname>();) \
    K_EXPORT_PLUGIN(factory(#libname))

#endif // _BALOO_SEARCHSTORE_H