/*
   This file is part of the KDE Baloo project.
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

#include "postingdb.h"
#include "singledbtest.h"

using namespace Baloo;

class PostingDBTest : public SingleDBTest
{
    Q_OBJECT
private Q_SLOTS:
    void test() {
        PostingDB db(m_txn);

        QByteArray word("fire");
        PostingList list = {1, 5, 6};

        db.put(word, list);
        QCOMPARE(db.get(word), list);
    }

    void testPrefixIter() {
        PostingDB db(m_txn);

        db.put("abc", {1, 4, 5, 9, 11});
        db.put("fir", {1, 3, 5});
        db.put("fire", {1, 8, 9});
        db.put("fore", {2, 3, 5});

        PostingIterator* it = db.prefixIter("fi");
        QVERIFY(it);

        QVector<uint> result = {1, 3, 5, 8, 9};
        for (uint val : result) {
            QCOMPARE(it->next(), static_cast<uint>(val));
            QCOMPARE(it->docId(), static_cast<uint>(val));
        }
    }
};

QTEST_MAIN(PostingDBTest)

#include "postingdbtest.moc"
