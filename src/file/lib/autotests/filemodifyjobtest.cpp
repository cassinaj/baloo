/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2014  Vishesh Handa <me@vhanda.in>
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

#include "filemodifyjobtest.h"
#include "filemodifyjob.h"
#include "../db.h"
#include "filemapping.h"
#include "file.h"

#include "qtest_kde.h"
#include <KTemporaryFile>
#include <KDebug>

#include <qjson/serializer.h>
#include <xapian.h>
#include <attr/xattr.h>

using namespace Baloo;

void FileModifyJobTest::testSingleFile()
{
    KTemporaryFile tmpFile;
    tmpFile.open();

    const QString fileUrl = tmpFile.fileName();

    File file(fileUrl);
    file.setRating(5);
    file.setUserComment("User Comment");

    FileModifyJob* job = new FileModifyJob(file);
    QVERIFY(job->exec());

    char buffer[1000];

    const QByteArray arr = QFile::encodeName(fileUrl);

    int len = getxattr(arr.constData(), "user.baloo.rating", &buffer, 1000);
    QVERIFY(len > 0);

    int r = QString::fromUtf8(buffer, len).toInt();
    QCOMPARE(r, 5);

    len = getxattr(arr.constData(), "user.baloo.tags", &buffer, 1000);
    QCOMPARE(len, 0);

    len = getxattr(arr.constData(), "user.xdg.comment", &buffer, 1000);
    QVERIFY(len > 0);
    QString comment = QString::fromUtf8(buffer, len);
    QCOMPARE(comment, QString("User Comment"));

    //
    // Check in Xapian
    //
    FileMapping fileMap(fileUrl);
    QSqlDatabase sqlDb = fileMappingDb();
    QVERIFY(fileMap.fetch(sqlDb));

    const std::string xapianPath = fileIndexDbPath().toStdString();
    Xapian::Database db(xapianPath);

    Xapian::Document doc = db.get_document(fileMap.id());

    Xapian::TermIterator iter = doc.termlist_begin();
    QCOMPARE(*iter, std::string("Ccomment"));

    iter++;
    QCOMPARE(*iter, std::string("Cuser"));

    iter++;
    QCOMPARE(*iter, std::string("R5"));

}

QTEST_KDEMAIN_CORE(FileModifyJobTest)