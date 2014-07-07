/*
 * This file is part of the KDE Baloo Project
 * Copyright (C) 2013  Vishesh Handa <me@vhanda.in>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "filesearchstoretest.h"
#include "filesearchstore.h"
#include "../../database.h"
#include "filemapping.h"
#include "query.h"
#include "term.h"

#include <QDebug>
#include <QtTest>

#include <xapian.h>


using namespace Baloo;

FileSearchStoreTest::FileSearchStoreTest(QObject* parent)
    : QObject(parent)
    , m_tempDir(0)
    , m_db(0)
    , m_store(0)
{
}

void FileSearchStoreTest::init()
{
    cleanupTestCase();

    m_db = new Database(this);
    m_tempDir = new QTemporaryDir();
    m_db->setPath(m_tempDir->path());
    m_db->init();

    m_store = new FileSearchStore(this);
    m_store->setDbPath(m_tempDir->path());
}

void FileSearchStoreTest::initTestCase()
{
}

void FileSearchStoreTest::cleanupTestCase()
{
    delete m_store;
    m_store = 0;

    delete m_db;
    m_db = 0;

    delete m_tempDir;
    m_tempDir = 0;
}

uint FileSearchStoreTest::insertUrl(const QString& url)
{
    FileMapping file(url);
    file.create(m_db->sqlDatabase());

    return file.id();
}

void FileSearchStoreTest::insertType(int id, const QString& type)
{
    Xapian::Document doc;
    doc.add_term(("T" + type.toLower()).toUtf8().constData());

    std::string dir = m_tempDir->path().toUtf8().constData();
    QScopedPointer<Xapian::WritableDatabase> wdb(new Xapian::WritableDatabase(dir,
                                                                              Xapian::DB_CREATE_OR_OPEN));
    wdb->replace_document(id, doc);
    wdb->commit();

    m_db->xapianDatabase()->db()->reopen();

}

void FileSearchStoreTest::insertText(int id, const QString& text)
{
    Xapian::Document doc;

    Xapian::TermGenerator termGen;
    termGen.set_document(doc);
    termGen.index_text(text.toUtf8().constData());

    std::string dir = m_tempDir->path().toUtf8().constData();
    QScopedPointer<Xapian::WritableDatabase> wdb(new Xapian::WritableDatabase(dir,
                                                                              Xapian::DB_CREATE_OR_OPEN));
    wdb->replace_document(id, doc);
    wdb->commit();

    m_db->xapianDatabase()->db()->reopen();
}

void FileSearchStoreTest::insertRating(int id, int rating)
{
    Xapian::Document doc;

    QString str = QLatin1Char('R') + QString::number(rating);
    doc.add_term(str.toUtf8().constData());

    std::string dir = m_tempDir->path().toUtf8().constData();
    QScopedPointer<Xapian::WritableDatabase> wdb(new Xapian::WritableDatabase(dir,
                                                                              Xapian::DB_CREATE_OR_OPEN));
    wdb->replace_document(id, doc);
    wdb->commit();

    m_db->xapianDatabase()->db()->reopen();
}


void FileSearchStoreTest::testSimpleSearchString()
{
    QString url1(QLatin1String("/home/t/a"));
    uint id1 = insertUrl(url1);
    insertText(id1, QLatin1String("This is sample text"));

    QString url2(QLatin1String("/home/t/b"));
    uint id2 = insertUrl(url2);
    insertText(id2, QLatin1String("sample sample more sample text"));

    Query q;
    q.addType(QLatin1String("File"));
    q.setSearchString(QLatin1String("Sample"));

    int qid = m_store->exec(q);
    QCOMPARE(qid, 1);
    QVERIFY(m_store->next(qid));
    QCOMPARE(m_store->id(qid), serialize("file", id2));
    QCOMPARE(m_store->url(qid), QUrl::fromLocalFile(url2));

    QVERIFY(m_store->next(qid));
    QCOMPARE(m_store->id(qid), serialize("file", id1));
    QCOMPARE(m_store->url(qid), QUrl::fromLocalFile(url1));

    QVERIFY(!m_store->next(qid));
    QVERIFY(m_store->id(qid).isEmpty());
    QVERIFY(m_store->url(qid).isEmpty());

    m_store->close(qid);
}

void FileSearchStoreTest::testIncludeDir()
{
    QString url1(QLatin1String("/home/t/a"));
    uint id1 = insertUrl(url1);
    insertText(id1, QLatin1String("This is sample text"));

    QString url2(QLatin1String("/home/t/b"));
    uint id2 = insertUrl(url2);
    insertText(id2, QLatin1String("sample sample more sample text"));

    QString url3(QLatin1String("/home/garden/b"));
    uint id3 = insertUrl(url3);
    insertText(id3, QLatin1String("The grass is green in the garden."));

    QString url4(QLatin1String("/home/tt/b"));
    uint id4 = insertUrl(url4);
    insertText(id4, QLatin1String("Let's see if this works."));

    QString url5(QLatin1String("/home/t/c"));
    uint id5 = insertUrl(url5);
    insertText(id5, QLatin1String("sample sample more sample text"));

    Query q;
    q.addType(QLatin1String("File"));
    q.addCustomOption(QLatin1String("includeFolder"), QVariant(QLatin1String("/home/t")));

    int qid = m_store->exec(q);
    QCOMPARE(qid, 1);
    QVERIFY(m_store->next(qid));
    QCOMPARE(m_store->id(qid), serialize("file", id1));
    QCOMPARE(m_store->url(qid), QUrl::fromLocalFile(url1));

    QVERIFY(m_store->next(qid));
    QCOMPARE(m_store->id(qid), serialize("file", id2));
    QCOMPARE(m_store->url(qid), QUrl::fromLocalFile(url2));

    QVERIFY(m_store->next(qid));
    QCOMPARE(m_store->id(qid), serialize("file", id5));
    QCOMPARE(m_store->url(qid), QUrl::fromLocalFile(url5));

    QVERIFY(!m_store->next(qid));
    QVERIFY(m_store->id(qid).isEmpty());
    QVERIFY(m_store->url(qid).isEmpty());

    m_store->close(qid);
}

void FileSearchStoreTest::testRatings()
{
    QString url1(QLatin1String("/home/t/a"));
    uint id1 = insertUrl(url1);
    insertRating(id1, 2);

    QString url2(QLatin1String("/home/t/b"));
    uint id2 = insertUrl(url2);
    insertRating(id2, 4);

    QString url3(QLatin1String("/home/garden/b"));
    uint id3 = insertUrl(url3);
    insertRating(id3, 6);

    QString url4(QLatin1String("/home/tt/b"));
    uint id4 = insertUrl(url4);
    insertRating(id4, 10);

    QString url5(QLatin1String("/home/tt/c"));
    uint id5 = insertUrl(url5);
    insertText(id5, QLatin1String("Test text"));

    //
    // Less than 5
    //
    Query q;
    q.addType(QLatin1String("File"));
    q.setTerm(Term(QLatin1String("rating"), 5, Term::Less));

    int qid1 = m_store->exec(q);
    QCOMPARE(qid1, 1);

    QVERIFY(m_store->next(qid1));
    QCOMPARE(m_store->id(qid1), serialize("file", id1));
    QVERIFY(m_store->next(qid1));
    QCOMPARE(m_store->id(qid1), serialize("file", id2));
    QVERIFY(!m_store->next(qid1));

    //
    // Greater than 6
    //
    q.setTerm(Term(QLatin1String("rating"), 6, Term::Greater));

    int qid2 = m_store->exec(q);
    QCOMPARE(qid2, 2);

    QVERIFY(m_store->next(qid2));
    QCOMPARE(m_store->id(qid2), serialize("file", id4));
    QVERIFY(!m_store->next(qid2));
}

void FileSearchStoreTest::testEmptySearchString()
{
    QString url1(QLatin1String("/home/t/a"));
    uint id1 = insertUrl(url1);
    insertText(id1, QLatin1String("File A"));

    QString url2(QLatin1String("/home/t/b"));
    uint id2 = insertUrl(url2);
    insertText(id2, QLatin1String("File B"));

    QString url3(QLatin1String("/home/garden/b"));
    uint id3 = insertUrl(url3);
    insertText(id3, QLatin1String("Garden B"));

    QString url4(QLatin1String("/home/tt/b"));
    uint id4 = insertUrl(url4);
    insertText(id4, QLatin1String("TT B"));

    QString url5(QLatin1String("/home/tt/c"));
    uint id5 = insertUrl(url5);
    insertText(id5, QLatin1String("TT C"));

    Query q;
    q.addType(QLatin1String("File"));

    int qid1 = m_store->exec(q);
    QCOMPARE(qid1, 1);

    QVERIFY(m_store->next(qid1));
    QCOMPARE(m_store->id(qid1), serialize("file", id1));
    QVERIFY(m_store->next(qid1));
    QCOMPARE(m_store->id(qid1), serialize("file", id2));
    QVERIFY(m_store->next(qid1));
    QCOMPARE(m_store->id(qid1), serialize("file", id3));
    QVERIFY(m_store->next(qid1));
    QCOMPARE(m_store->id(qid1), serialize("file", id4));
    QVERIFY(m_store->next(qid1));
    QCOMPARE(m_store->id(qid1), serialize("file", id5));
    QVERIFY(!m_store->next(qid1));
}

void FileSearchStoreTest::testAllVideos()
{
    QString url1(QLatin1String("/home/t/a"));
    uint id1 = insertUrl(url1);
    insertType(id1, QLatin1String("Video"));

    QString url2(QLatin1String("/home/t/b"));
    uint id2 = insertUrl(url2);
    insertType(id2, QLatin1String("Image"));

    QString url3(QLatin1String("/home/garden/b"));
    uint id3 = insertUrl(url3);
    insertType(id3, QLatin1String("Video"));

    Query q;
    q.addType(QLatin1String("Video"));

    int qid1 = m_store->exec(q);
    QCOMPARE(qid1, 1);

    QVERIFY(m_store->next(qid1));
    QCOMPARE(m_store->id(qid1), serialize("file", id1));
    QVERIFY(m_store->next(qid1));
    QCOMPARE(m_store->id(qid1), serialize("file", id3));
    QVERIFY(!m_store->next(qid1));
}



QTEST_MAIN(Baloo::FileSearchStoreTest)
