/*
 * Copyright (C) 2015  Vishesh Handa <vhanda@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "documentdatadb.h"

using namespace Baloo;

DocumentDataDB::DocumentDataDB(const char* name, MDB_txn* txn)
    : m_txn(txn)
{
    Q_ASSERT(txn != 0);

    int rc = mdb_dbi_open(txn, name, MDB_CREATE | MDB_INTEGERKEY, &m_dbi);
    Q_ASSERT_X(rc == 0, "DocumentUrlDB", mdb_strerror(rc));
}

DocumentDataDB::~DocumentDataDB()
{
    mdb_dbi_close(mdb_txn_env(m_txn), m_dbi);
}

void DocumentDataDB::put(uint docId, const QByteArray& url)
{
    Q_ASSERT(docId > 0);
    Q_ASSERT(!url.isEmpty());

    MDB_val key;
    key.mv_size = sizeof(uint);
    key.mv_data = static_cast<void*>(&docId);

    MDB_val val;
    val.mv_size = url.size();
    val.mv_data = static_cast<void*>(const_cast<char*>(url.constData()));

    int rc = mdb_put(m_txn, m_dbi, &key, &val, 0);
    Q_ASSERT_X(rc == 0, "DocumentDataDB::put", mdb_strerror(rc));
}

QByteArray DocumentDataDB::get(uint docId)
{
    Q_ASSERT(docId > 0);

    MDB_val key;
    key.mv_size = sizeof(uint);
    key.mv_data = static_cast<void*>(&docId);

    MDB_val val;
    int rc = mdb_get(m_txn, m_dbi, &key, &val);
    if (rc == MDB_NOTFOUND) {
        return QByteArray();
    }
    Q_ASSERT_X(rc == 0, "DocumentDataDB::get", mdb_strerror(rc));

    return QByteArray::fromRawData(static_cast<char*>(val.mv_data), val.mv_size);
}

void DocumentDataDB::del(uint docId)
{
    Q_ASSERT(docId > 0);

    MDB_val key;
    key.mv_size = sizeof(uint);
    key.mv_data = static_cast<void*>(&docId);

    int rc = mdb_del(m_txn, m_dbi, &key, 0);
    Q_ASSERT_X(rc == 0, "DocumentUrlDB::del", mdb_strerror(rc));
}
