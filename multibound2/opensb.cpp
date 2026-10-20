#include "util.h" // part of utility header

// functions for OpenStarbound installation and update management

#include "data/config.h"

#include <cmath>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QEventLoop>

#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>


// QDateTime::fromString(str, Qt::ISODate)

namespace { // clazy:excludeall=non-pod-global-static
    const auto releasesUrl = qs("https://api.github.com/repos/OpenStarbound/OpenStarbound/releases");
    //
    QJsonDocument releaseInfo;
    QJsonObject eligibleRelease;
    QString eligibleReleaseUrl;
    QJsonObject currentRelease;

    const auto ciUrl = qs("https://api.github.com/repos/OpenStarbound/OpenStarbound/actions/runs");

    // os-specific things
#if defined(Q_OS_WIN)
    const auto pkgAssetName = qs("OpenStarbound-Windows-Client.zip");
    const auto ciPlatform = qs("Windows");
    const auto ciArtifact = qs("OpenStarbound-Windows-Client.zip");
#elif defined (Q_OS_MACOS)
    const auto pkgAssetName = qs("osx_opensb.zip except not because we don't have code for this"); // it just won't find an eligible release
    const auto ciPlatform = qs("macOS");
    const auto ciArtifact = qs("and now we have a problem");
#else // linux
    const auto pkgAssetName = qs("linux_opensb.zip");
    const auto ciPlatform = qs("Ubuntu Linux");
    const auto ciArtifact = qs("OpenStarbound-Linux-Client");
#endif

    // some utility stuff
    QEventLoop ev;
    void await(QNetworkReply* reply) {
        QObject::connect(reply, &QNetworkReply::finished, &ev, &QEventLoop::quit);
        ev.exec(); // wait for things
        reply->deleteLater();
    }
}

QJsonDocument& getReleaseInfo(bool forceRefresh = false) {
    if (!forceRefresh && !releaseInfo.isEmpty()) return releaseInfo;

    QNetworkAccessManager net;

    QNetworkRequest req(releasesUrl);
    auto reply = net.get(req);
    await(reply);

    releaseInfo = QJsonDocument::fromJson(reply->readAll());
    return releaseInfo;
}

QJsonObject& findEligibleRelease(bool forceRefresh = false) {
    if (!forceRefresh && !eligibleRelease.isEmpty()) return eligibleRelease;
    getReleaseInfo(forceRefresh);

    auto ra = releaseInfo.array();

    // for now we assume releases are listed newest to oldest
    for (const auto& rv : ra) {
        auto r = rv.toObject();
        // loop through assets to find the one we want
        for (const auto& av : r[qs("assets")].toArray()) {
            auto a = av.toObject();
            if (a[qs("name")] == pkgAssetName) {
                eligibleRelease = r;
                eligibleReleaseUrl = a[qs("browser_download_url")].toString();
                return eligibleRelease;
            }
        }
    }

    return eligibleRelease;
}

void MultiBound::Util::openSBCheckForUpdates() {
    if (currentRelease.isEmpty()) {
        if (QFile f(Util::splicePath(Config::openSBRoot, "/release.json")); f.exists()) {
            f.open(QFile::ReadOnly);
            currentRelease = QJsonDocument::fromJson(f.readAll()).object();
        }
    }
    // if still empty, assume none and... prompt?
    if (currentRelease.isEmpty()) {
        // TODO
    }

    findEligibleRelease(true);
    if (eligibleRelease.isEmpty()) return; // nothing to show
    auto ed = QDateTime::fromString(eligibleRelease["published_at"].toString(), Qt::ISODate);
    auto cd = QDateTime::fromString(currentRelease["published_at"].toString(), Qt::ISODate);
    if (ed > cd) { // eligible is newer (or more extant) than current, prompt to update
        auto ver = eligibleRelease["name"].toString();
        if (ver.isEmpty()) ver = eligibleRelease["tag_name"].toString();
        if (ver.isEmpty()) ver = "unknown";

        auto res = QMessageBox::information(nullptr, "OpenStarbound update", QString("A new version of OpenStarbound (%1) is available. Would you like to install it?").arg(ver), QMessageBox::Yes, QMessageBox::No);
        if (res == QMessageBox::Yes) {
            openSBUpdate();
        }
        //
    }
}

bool MultiBound::Util::openSBWindowsExtract(const QString& zipPath, const QString& destDir) {
    QProcess ps;
    QObject::connect(&ps, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), &ev, &QEventLoop::quit);
    QDir dst(destDir);
    // extract using tar or powershell 7
    auto tar  = QStandardPaths::findExecutable("tar");
    auto pwsh = QStandardPaths::findExecutable("pwsh");
    const QString exe = dst.absoluteFilePath("client_distribution/win/starbound.exe");

    if (!tar.isEmpty()) {
        qDebug() << "tar start";
        ps.start(tar, QStringList() << "-xf" << zipPath << "-C" << destDir);
        ev.exec();
        qDebug() << "tar end" << ps.exitCode(); // fuck you for return 1 on success
    }

    qDebug() << "exe" << QFileInfo::exists(exe);
    if (!QFileInfo::exists(exe)) {
        if (pwsh.isEmpty()) {
            QMessageBox::critical(nullptr, "", "Extraction failed. Install PowerShell 7 (pwsh).");
            return false;
        }
        qDebug() << "pwsh start";
        const QString pc = QString("Expand-Archive -Force -DestinationPath '%1' -LiteralPath '%2'").arg(destDir, zipPath);
        ps.start(pwsh, QStringList() << "-NoProfile" << "-NonInteractive" << "-Command" << pc);
        ev.exec();
        qDebug() << "pwsh end" << ps.exitCode();
    }

    qDebug() << "exe" << QFileInfo::exists(exe);
    if (!QFileInfo::exists(exe)) {
        QMessageBox::critical(nullptr, "", "Extraction failed. Client archive could not be extracted.");
        return false;
    }

    QDir wd(destDir);
    if (!wd.cd("client_distribution")) {
        QMessageBox::critical(nullptr, "", "Extraction failed. starbound.exe exists but somehow failed to access client_distribution?!");
        return false;
    }
    qDebug() << "move assets";
    wd.rename("assets", dst.absoluteFilePath("data"));
    wd.cd("win");
    foreach (auto fn, wd.entryList(QDir::Files)) {
        qDebug() << "move" << fn;
        wd.rename(fn, dst.absoluteFilePath(fn));
    }
    wd.setPath(dst.absoluteFilePath("client_distribution"));
    if (wd.exists()) wd.removeRecursively();
    return true;
}


void MultiBound::Util::openSBUpdate() {
    findEligibleRelease();
    if (eligibleRelease.isEmpty()) return; // nothing

    // set up directory structure
    QDir osbd(Config::openSBRoot);
    if (osbd.exists()) { // nuke old installation files if they exist
        osbd.removeRecursively();
    }
    osbd.mkpath("."); // make the directory exist (again)

    // download time!
    QNetworkAccessManager net;
    QEventLoop ev;
    const auto fn = qs("_download.zip");
    QFile f(osbd.absoluteFilePath(fn));
    f.open(QFile::WriteOnly);

    QNetworkRequest req(eligibleReleaseUrl);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy); // allow redirects
    auto reply = net.get(req);
    QObject::connect(reply, &QNetworkReply::downloadProgress, &ev, [](auto c, auto t) {
        updateStatus(QString("Downloading... %1\%").arg(std::floor((double)c/(double)t*100 + 0.5)));
    });
    QObject::connect(reply, &QNetworkReply::readyRead, &f, [&f, reply] {f.write(reply->readAll());});
    await(reply);

    f.flush();
    f.close();

    // now we unzip this fucker
    updateStatus("Extracting package...");
    QProcess ps;
    QObject::connect(&ps, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), &ev, &QEventLoop::quit);

#if defined(Q_OS_WIN)
    if (!openSBWindowsExtract(f.fileName(), osbd.absolutePath())) return;
#else
    // if you're running a desktop without unzip, huh???
    ps.start("unzip", QStringList() << f.fileName() << "-d" << osbd.absolutePath());
    ev.exec();

    foreach (auto fi, osbd.entryInfoList(QDir::Files)) { // fix permissions
        auto ext = fi.suffix();
        QFile ff(fi.absoluteFilePath());
        if (ext == "" or ext == "sh") { // assume no extension is executable
            ff.setPermissions(ff.permissions() | QFile::ExeOwner | QFile::ExeGroup | QFile::ExeOther);
        }
    }
#endif

    osbd.remove(fn); // clean up the zip after we're done

    if (auto f = QFile(osbd.absoluteFilePath("steam_appid.txt")); true) { // ensure appid file
        f.open(QFile::WriteOnly);
        f.write("211820");
        f.flush();
        f.close();
    }

    updateStatus("");

    // finally, write out the release info
    QFile rf(osbd.absoluteFilePath("release.json"));
    rf.open(QFile::WriteOnly);
    rf.write(QJsonDocument(eligibleRelease).toJson(QJsonDocument::Indented));
    rf.flush();
    rf.close();

    currentRelease = eligibleRelease;
}

void MultiBound::Util::openSBUpdateCI() {
    QNetworkAccessManager net;

    auto reply = net.get(QNetworkRequest(ciUrl));
    await(reply);

    auto runs = QJsonDocument::fromJson(reply->readAll()).object()["workflow_runs"].toArray();

    QJsonObject ri;
    for (const auto& rv : runs) {
        auto r = rv.toObject();

        qDebug() << "run entry" << r["name"].toString();

        if (r["head_branch"] == "main" &&
            r["status"] == "completed" &&
            r["conclusion"] == "success") {
            ri = r;
            break;
        }
    }

    if (ri.isEmpty()) {
        QMessageBox::critical(nullptr, "", "No matching artifacts found.");
        return;
    }

    qDebug() << "fetching artifacts";
    reply = net.get(QNetworkRequest(ri["artifacts_url"].toString()));
    await(reply);
    qDebug() << "fetch complete";

    auto afx = QJsonDocument::fromJson(reply->readAll()).object()["artifacts"].toArray();

    QJsonObject ai;
    foreach (const auto& av, afx) {
        auto a = av.toObject();
        qDebug() << "artifact entry" << a["name"].toString();
        if (a["name"] == ciArtifact) {
            ai = a;
            break;
        }
    }

    if (ai.isEmpty()) {
        QMessageBox::critical(nullptr, "", "No matching artifacts found.");
        return;
    } else {
        auto res = QMessageBox::question(nullptr, "", QString("Build #%1 (%2): %3").arg(ri["run_number"].toVariant().toString(), ri["head_sha"].toString().left(7), ri["display_title"].toString()), "Install", "Cancel");
        qDebug() << "prompt returned" << res;
        if (res != 0) return;
    }
    qDebug() << "after prompt";

    QDir cid(Config::openSBCIRoot);
    if (cid.exists()) { // nuke old installation files if they exist
        cid.removeRecursively();
    }
    cid.mkpath("."); // make the directory exist (again)

    const auto fn = qs("_download.zip");
    QFile f(cid.absoluteFilePath(fn));
    f.open(QFile::WriteOnly);

    // we need to take a slight detour here; assemble our download url
    auto lnk = QString("https://nightly.link/OpenStarbound/OpenStarbound/suites/%1/artifacts/%2").arg(ri["check_suite_id"].toVariant().toString(), ai["id"].toVariant().toString());
    QNetworkRequest req(lnk); // prepare for actual download
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy); // allow redirects
    reply = net.get(req);
    QObject::connect(reply, &QNetworkReply::downloadProgress, &ev, [](auto c, auto t) {
        updateStatus(QString("Downloading... %1\%").arg(std::floor((double)c/(double)t*100 + 0.5)));
    });
    QObject::connect(reply, &QNetworkReply::readyRead, &f, [&f, reply] {f.write(reply->readAll());});
    await(reply);

    f.flush();
    f.close();

    // now we unzip this fucker
    updateStatus("Extracting package...");
    QProcess ps;
    QObject::connect(&ps, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), &ev, &QEventLoop::quit);

    // windows: unzip > assets, win
    // linux: unzip, client.tar, client_distribution/[assets, linux]
    // why.

    // ^^ oct 26: the windows package also doing that too :)

    QDir wd(cid.absolutePath()); // working dir
#if defined(Q_OS_WIN) // oh boy, platform implementation time
    if (!openSBWindowsExtract(f.fileName(), cid.absolutePath())) return;
    // super easy (not anymore)
#else // unix, probably linux
    // extract with unzip, untar, move assets out, move everything in linux to root, then nuke client_distribution
    // kind of annoying
    ps.start("unzip", QStringList() << f.fileName() << "-d" << cid.absolutePath());
    ev.exec();
    ps.start("tar", QStringList() << "-xf" << cid.absoluteFilePath("client.tar") << "-C" << cid.absolutePath());
    ev.exec();

    updateStatus("");
    if (!wd.cd("client_distribution")) return; // download failed for some reason or another
    wd.rename("assets", cid.absoluteFilePath("data"));
    wd.cd("linux");
    foreach (auto fi, wd.entryInfoList(QDir::Files)) {
        auto fn = fi.fileName();
        auto ext = fi.suffix();
        QFile ff(fi.absoluteFilePath());
        if (ext == "" or ext == "sh") { // assume no extension is executable
            ff.setPermissions(ff.permissions() | QFile::ExeOwner | QFile::ExeGroup | QFile::ExeOther);
        }
        ff.rename(cid.absoluteFilePath(fn));
    }
    // nuke leftovers
    wd.setPath(cid.absoluteFilePath("client_distribution"));
    if (wd.exists()) wd.removeRecursively();
    cid.remove("client.tar"); // clean up internal subarchive
#endif
    f.remove(); // clean up downloaded archive

    if (auto f = QFile(cid.absoluteFilePath("steam_appid.txt")); true) { // ensure appid file
        f.open(QFile::WriteOnly);
        f.write("211820");
        f.flush();
        f.close();
    }

    updateStatus("");
}
