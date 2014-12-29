/*
   Copyright 2009-2010 Sebastian Trueg <trueg@kde.org>
   Copyright 2013      Vishesh Handa <me@vhanda.in>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "kio_timeline.h"
#include "timelinetools.h"
#include "query.h"
#include "resultiterator.h"

#include <QDebug>
#include <QDate>
#include <QCoreApplication>

#include <KUser>
#include <KFormat>

#include <KLocalizedString>
#include <KCalendarSystem>
#include <KLocale>

using namespace Baloo;

namespace
{
KIO::UDSEntry createFolderUDSEntry(const QString& name, const QString& displayName, const QDate& date)
{
    KIO::UDSEntry uds;
    QDateTime dt(date, QTime(0, 0, 0));
    uds.insert(KIO::UDSEntry::UDS_NAME, name);
    uds.insert(KIO::UDSEntry::UDS_DISPLAY_NAME, displayName);
    uds.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    uds.insert(KIO::UDSEntry::UDS_MIME_TYPE, QString::fromLatin1("inode/directory"));
    uds.insert(KIO::UDSEntry::UDS_MODIFICATION_TIME, dt.toTime_t());
    uds.insert(KIO::UDSEntry::UDS_CREATION_TIME, dt.toTime_t());
    uds.insert(KIO::UDSEntry::UDS_ACCESS, 0700);
    uds.insert(KIO::UDSEntry::UDS_USER, KUser().loginName());
    return uds;
}

KIO::UDSEntry createMonthUDSEntry(int month, int year)
{
    QString dateString
        = KLocale::global()->calendar()->formatDate(QDate(year, month, 1),
                i18nc("Month and year used in a tree above the actual days. "
                      "Have a look at http://api.kde.org/4.x-api/kdelibs-"
                      "apidocs/kdecore/html/classKCalendarSystem.html#a560204439a4b670ad36c16c404f292b4 "
                      "to see which variables you can use and ask kde-i18n-doc@kde.org if you have "
                      "problems understanding how to translate this",
                      "%B %Y"));
    return createFolderUDSEntry(QDate(year, month, 1).toString(QLatin1String("yyyy-MM")),
                                dateString,
                                QDate(year, month, 1));
}

KIO::UDSEntry createDayUDSEntry(const QDate& date)
{
    KIO::UDSEntry uds = createFolderUDSEntry(date.toString(QLatin1String("yyyy-MM-dd")),
                        KFormat().formatRelativeDate(date, QLocale::LongFormat),
                        date);

    return uds;
}

KIO::UDSEntry createFileUDSEntry(const QString& filePath)
{
    KIO::UDSEntry uds;
    // Code from kdelibs/kioslaves/file.cpp
    QT_STATBUF statBuf;
    if( QT_LSTAT(QFile::encodeName(filePath).data(), &statBuf) == 0) {
        uds.insert(KIO::UDSEntry::UDS_MODIFICATION_TIME, statBuf.st_mtime);
        uds.insert(KIO::UDSEntry::UDS_ACCESS_TIME, statBuf.st_atime);
        uds.insert(KIO::UDSEntry::UDS_SIZE, statBuf.st_size);
        uds.insert(KIO::UDSEntry::UDS_USER, statBuf.st_uid);
        uds.insert(KIO::UDSEntry::UDS_GROUP, statBuf.st_gid);

        mode_t type = statBuf.st_mode & S_IFMT;
        mode_t access = statBuf.st_mode & 07777;

        uds.insert(KIO::UDSEntry::UDS_FILE_TYPE, type);
        uds.insert(KIO::UDSEntry::UDS_ACCESS, access);
        QUrl fileUrl = QUrl::fromLocalFile(filePath);
        uds.insert(KIO::UDSEntry::UDS_URL, fileUrl.url());
        uds.insert(KIO::UDSEntry::UDS_NAME, fileUrl.fileName());
    }

    return uds;
}

}


TimelineProtocol::TimelineProtocol(const QByteArray& poolSocket, const QByteArray& appSocket)
    : KIO::SlaveBase("timeline", poolSocket, appSocket)
{
}


TimelineProtocol::~TimelineProtocol()
{
}


void TimelineProtocol::listDir(const QUrl& url)
{
    switch (parseTimelineUrl(url, &m_date, &m_filename)) {
    case RootFolder:
        listEntry(createFolderUDSEntry(QLatin1String("today"), i18n("Today"), QDate::currentDate()));
        listEntry(createFolderUDSEntry(QLatin1String("calendar"), i18n("Calendar"), QDate::currentDate()));
        finished();
        break;

    case CalendarFolder:
        listThisYearsMonths();
        // TODO: add entry for previous years
        finished();
        break;

    case MonthFolder:
        listDays(m_date.month(), m_date.year());
        finished();
        break;

    case DayFolder: {
        Query query;
        query.addType(QLatin1String("File"));
        query.setDateFilter(m_date.year(), m_date.month(), m_date.day());
        query.setSortingOption(Query::SortNone);

        ResultIterator it = query.exec();
        while (it.next()) {
            KIO::UDSEntry uds = createFileUDSEntry(it.filePath());
            if (uds.count())
                listEntry(uds);
        }
        finished();
        break;
    }

    default:
        error(KIO::ERR_DOES_NOT_EXIST, url.toString());
        break;
    }
}


void TimelineProtocol::mimetype(const QUrl& url)
{
    switch (parseTimelineUrl(url, &m_date, &m_filename)) {
    case RootFolder:
    case CalendarFolder:
    case MonthFolder:
    case DayFolder:
        mimetype(QUrl(QString::fromLatin1("inode/directory")));
        break;

    default:
        error(KIO::ERR_DOES_NOT_EXIST, url.toString());
        break;
    }
}


void TimelineProtocol::stat(const QUrl& url)
{
    switch (parseTimelineUrl(url, &m_date, &m_filename)) {
    case RootFolder: {
        KIO::UDSEntry uds;
        uds.insert(KIO::UDSEntry::UDS_NAME, QString::fromLatin1("/"));
        uds.insert(KIO::UDSEntry::UDS_ICON_NAME, QString::fromLatin1("nepomuk"));
        uds.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
        uds.insert(KIO::UDSEntry::UDS_MIME_TYPE, QString::fromLatin1("inode/directory"));
        statEntry(uds);
        finished();
        break;
    }

    case CalendarFolder:
        statEntry(createFolderUDSEntry(QLatin1String("calendar"), i18n("Calendar"), QDate::currentDate()));
        finished();
        break;

    case MonthFolder:
        statEntry(createMonthUDSEntry(m_date.month(), m_date.year()));
        finished();
        break;

    case DayFolder:
        if (m_filename.isEmpty()) {
            statEntry(createDayUDSEntry(m_date));
            finished();
        }
        break;

    default:
        error(KIO::ERR_DOES_NOT_EXIST, url.toString());
        break;
    }
}


void TimelineProtocol::listDays(int month, int year)
{
    const int days = KLocale::global()->calendar()->daysInMonth(year, month);
    for (int day = 1; day <= days; ++day) {
        QDate date(year, month, day);

        if (date <= QDate::currentDate() && filesInDate(date)) {
            listEntry(createDayUDSEntry(date));
        }
    }
}

bool TimelineProtocol::filesInDate(const QDate& date)
{
    Query query;
    query.addType(QLatin1String("File"));
    query.setLimit(1);
    query.setDateFilter(date.year(), date.month(), date.day());
    query.setSortingOption(Query::SortNone);

    ResultIterator it = query.exec();
    return it.next();
}


void TimelineProtocol::listThisYearsMonths()
{
    Query query;
    query.addType(QLatin1String("File"));
    query.setLimit(1);
    query.setSortingOption(Query::SortNone);

    int year = QDate::currentDate().year();
    int currentMonth = QDate::currentDate().month();
    for (int month = 1; month <= currentMonth; ++month) {
        query.setDateFilter(year, month);
        ResultIterator it = query.exec();
        if (it.next()) {
            listEntry(createMonthUDSEntry(month, year));
        }
    }
}


extern "C"
{
    Q_DECL_EXPORT int kdemain(int argc, char** argv)
    {
        QCoreApplication app(argc, argv);
        app.setApplicationName(QStringLiteral("kio_timeline"));
        Baloo::TimelineProtocol slave(argv[2], argv[3]);
        slave.dispatchLoop();
        return 0;
    }
}

