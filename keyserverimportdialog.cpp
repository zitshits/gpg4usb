/*
 *
 *      keyserverimportdialog.cpp
 *
 *      Copyright 2008 gpg4usb-team <gpg4usb@cpunk.de>
 *
 *      This file is part of gpg4usb.
 *
 *      Gpg4usb is free software: you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, either version 3 of the License, or
 *      (at your option) any later version.
 *
 *      Gpg4usb is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with gpg4usb.  If not, see <http://www.gnu.org/licenses/>
 */

#include "keyserverimportdialog.h"

KeyServerImportDialog::KeyServerImportDialog(GpgME::GpgContext *ctx, KeyList *keyList, QWidget *parent)
    : QDialog(parent), mCtx(ctx), mKeyList(keyList)
{
    // Buttons
    closeButton = createButton(tr("&Close"), SLOT(close()));
    importButton = createButton(tr("&Import"), SLOT(slotImport()));
    searchButton = createButton(tr("&Search"), SLOT(slotSearch()));

    // Line edit for search string
    searchLabel = new QLabel(tr("Search string:"));
    searchLineEdit = new QLineEdit();

    // combobox for keyserverlist
    keyServerLabel = new QLabel(tr("Keyserver:"));
    keyServerComboBox = createComboBox();

    // table containing the keys found
    createKeysTable();
    message = new QLabel;
    icon = new QLabel;

    // Layout for messagebox
    QHBoxLayout *messageLayout= new QHBoxLayout;
    messageLayout->addWidget(icon);
    messageLayout->addWidget(message);
    messageLayout->addStretch();

    // Layout for import and close button
    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(importButton);
    buttonsLayout->addWidget(closeButton);

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(searchLabel, 1, 0);
    mainLayout->addWidget(searchLineEdit, 1, 1);
    mainLayout->addWidget(searchButton,1, 2);
    mainLayout->addWidget(keyServerLabel, 2, 0);
    mainLayout->addWidget(keyServerComboBox, 2, 1);
    mainLayout->addWidget(keysTable, 3, 0, 1, 3);
    mainLayout->addLayout(messageLayout, 4, 0, 1, 3);
    mainLayout->addLayout(buttonsLayout, 5, 0, 1, 3);

    this->setLayout(mainLayout);
    this->setWindowTitle(tr("Import Keys from Keyserver"));
    this->resize(700, 300);
    this->setModal(true);
}

QPushButton *KeyServerImportDialog::createButton(const QString &text, const char *member)
{
    QPushButton *button = new QPushButton(text);
    connect(button, SIGNAL(clicked()), this, member);
    return button;
}

QComboBox *KeyServerImportDialog::createComboBox()
{
    QComboBox *comboBox = new QComboBox;
    comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // Read keylist from ini-file and fill it into combobox
    QSettings settings;
    comboBox->addItems(settings.value("keyserver/keyServerList").toStringList());

    // set default keyserver in combobox
    QString keyserver = settings.value("keyserver/defaultKeyServer").toString();
    comboBox->setCurrentIndex(comboBox->findText(keyserver));

    return comboBox;
}

void KeyServerImportDialog::createKeysTable()
{
    keysTable = new QTableWidget();
    keysTable->setColumnCount(4);

    // always a whole row is marked
    keysTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    keysTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Make just one row selectable
    keysTable->setSelectionMode(QAbstractItemView::SingleSelection);

    QStringList labels;
    labels  << tr("UID") << tr("Creation date") << tr("KeyID") << tr("Tag");
    keysTable->horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
    keysTable->setHorizontalHeaderLabels(labels);
    keysTable->verticalHeader()->hide();

    connect(keysTable, SIGNAL(cellActivated(int,int)),
            this, SLOT(slotImport()));
}

void KeyServerImportDialog::setMessage(const QString &text, bool error)
{
    message->setText(text);
    if (error) {
        QIcon undoicon = QIcon::fromTheme("dialog-error");
        QPixmap pixmap = undoicon.pixmap(QSize(32,32),QIcon::Normal,QIcon::On);
        icon->setPixmap(pixmap);
    } else {
        QIcon undoicon = QIcon::fromTheme("dialog-information");
        QPixmap pixmap = undoicon.pixmap(QSize(32,32),QIcon::Normal,QIcon::On);
        icon->setPixmap(pixmap);
    }
}

void KeyServerImportDialog::slotSearch()
{
    QUrl url = keyServerComboBox->currentText()+":11371/pks/lookup?search="+searchLineEdit->text()+"&op=index&options=mr";
    qnam = new QNetworkAccessManager(this);
    QNetworkReply* reply = qnam->get(QNetworkRequest(url));
    connect(reply, SIGNAL(finished()),
            this, SLOT(slotSearchFinished()));
}

void KeyServerImportDialog::slotSearchFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    keysTable->clearContents();
    keysTable->setRowCount(0);
    QString firstLine = QString(reply->readLine(1024));

    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (reply->error()) {
        setMessage(tr("Couldn't contact keyserver!"),true);
        //setMessage(reply->error());
        qDebug() << reply->error();
    }
    if (firstLine.contains("Error"))
    {
        QString text= QString(reply->readLine(1024));
        if (text.contains("Too many responses")) {
            setMessage(tr("Too many responses from keyserver!"),true);
        } else if (text.contains("No keys found")) {
            // if string looks like hex string, search again with 0x prepended
            QRegExp rx("[0-9A-Fa-f]*");
            QString query = searchLineEdit->text();
            if (rx.exactMatch(query)) {
               setMessage(tr("No keys found, input may be kexId, retrying search with 0x."),true);
               searchLineEdit->setText(query.prepend("0x"));
               this->slotSearch();
            } else {
                setMessage(tr("No keys found containing the search string!"),true);
            }
        } else if (text.contains("Insufficiently specific words")) {
            setMessage(tr("Insufficiently specific search string!"),true);
        } else {
            setMessage(text, true);
        }
    } else {
        int row = 0;
        char buff[1024];
        bool strikeout=false;
        while (reply->readLine(buff,sizeof(buff)) !=-1) {
            QString decoded = QString::fromUtf8(QByteArray::fromPercentEncoding(buff));
            QStringList line = decoded.split(":");

            //TODO: have a look at two following pub lines
            if (line[0] == "pub") {
                strikeout=false;

                QString flags = line[line.size()-1];

                keysTable->setRowCount(row+1);

                // flags can be "d" for disabled, "r" for revoked
                // or "e" for expired
                if (flags.contains("r") or flags.contains("d") or flags.contains("e")) {
                    strikeout=true;
                    if (flags.contains("e")) {
                        keysTable->setItem(row, 3, new QTableWidgetItem( QString("expired")));
                    }
                    if (flags.contains("r")) {
                        keysTable->setItem(row, 3, new QTableWidgetItem( QString(tr("revoked"))));
                    }
                    if (flags.contains("d")) {
                        keysTable->setItem(row, 3, new QTableWidgetItem( QString(tr("disabled"))));
                    }
                }

                QStringList line2 = QString(reply->readLine()).split(":");

                QTableWidgetItem *uid = new QTableWidgetItem();
                if (line2.size() > 1) {
                    uid->setText(line2[1]);
                    keysTable->setItem(row, 0, uid);
                }
                QTableWidgetItem *creationdate = new QTableWidgetItem(QDateTime::fromTime_t(line[4].toInt()).toString("dd. MMM. yyyy"));
                keysTable->setItem(row, 1, creationdate);
                QTableWidgetItem *keyid = new QTableWidgetItem(line[1]);
                keysTable->setItem(row, 2, keyid);
                if (strikeout) {
                    QFont strike = uid->font();
                    strike.setStrikeOut(true);
                    uid->setFont(strike);
                    creationdate->setFont(strike);
                    keyid->setFont(strike);
                }
                row++;
            } else {
                if (line[0] == "uid") {
                    QStringList l;
                    int height=keysTable->rowHeight(row-1);
                    keysTable->setRowHeight(row-1,height+16);
                    QString tmp=keysTable->item(row-1,0)->text();
                    tmp.append(QString("\n")+line[1]);
                    QTableWidgetItem *tmp1 = new QTableWidgetItem(tmp);
                    keysTable->setItem(row-1,0,tmp1);
                    if (strikeout) {
                        QFont strike = tmp1->font();
                        strike.setStrikeOut(true);
                        tmp1->setFont(strike);
                    }
                }
            }
            setMessage(tr("%1 keys found. Doubleclick a key to import it.").arg(row),false);
        }
        keysTable->resizeColumnsToContents();
    }
    reply->deleteLater();
    reply = 0;
}

void KeyServerImportDialog::slotImport()
{
    if ( keysTable->currentRow() > -1 ) {
        QString keyid = keysTable->item(keysTable->currentRow(),2)->text();
        QUrl url = keyServerComboBox->currentText();
        slotImport(QStringList(keyid), url);
   }
}

void KeyServerImportDialog::slotImport(QStringList keyIds)
{
    QSettings settings;
    QString keyserver=settings.value("keyserver/defaultKeyServer").toString();
    QUrl url(keyserver);
    slotImport(keyIds, url);
}


void KeyServerImportDialog::slotImport(QStringList keyIds, QUrl keyServerUrl)
{
    foreach(QString keyId, keyIds) {
        QUrl reqUrl(keyServerUrl.scheme() + "://" + keyServerUrl.host() + ":11371/pks/lookup?op=get&search=0x"+keyId+"&options=mr");
        //qDebug() << "req to " << reqUrl;
        qnam = new QNetworkAccessManager(this);
        QNetworkReply *reply = qnam->get(QNetworkRequest(reqUrl));
        connect(reply, SIGNAL(finished()),
                this, SLOT(slotImportFinished()));
    }
}

void KeyServerImportDialog::slotImportFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QByteArray key = reply->readAll();

    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (reply->error()) {
        setMessage(tr("Error while contacting keyserver!"),true);
        return;
    }
    this->importKeys(key.constData());
    setMessage(tr("Key imported"),false);

    // Add keyserver to list in config-file, if it isn't contained
    QSettings settings;
    QStringList keyServerList = settings.value("keyserver/keyServerList").toStringList();
    if (!keyServerList.contains(keyServerComboBox->currentText()))
    {
        keyServerList.append(keyServerComboBox->currentText());
        settings.setValue("keyserver/keyServerList", keyServerList);
    }
    reply->deleteLater();
    reply = 0;
}

void KeyServerImportDialog::importKeys(QByteArray inBuffer)
{
    //GpgImportInformation result = mCtx->importKey(inBuffer);
    KGpgImport *imp =  new KGpgImport(this, QString(inBuffer));
    connect(imp, SIGNAL(done(int)), SLOT(slotImportDone(int)));
    imp->start();

}

void KeyServerImportDialog::slotImportDone(int result)
{
    KGpgImport *import = qobject_cast<KGpgImport *>(sender());

    Q_ASSERT(import != NULL);
    const QStringList rawmsgs(import->getMessages());

    if (result != 0) {
        /*KMessageBox::detailedSorry(this, i18n("Key importing failed. Please see the detailed log for more information."),
                rawmsgs.join( QLatin1String( "\n")) , i18n("Key Import" ));*/
        qDebug() << "Key importing failed. Please see the detailed log for more information." << rawmsgs.join( QLatin1String( "\n"));
    }

    QStringList keys(import->getImportedIds(0x1f));
    const bool needsRefresh = !keys.isEmpty();
    keys << import->getImportedIds(0);
/*
    if (!keys.isEmpty()) {
        const QString msg(import->getImportMessage());
        const QStringList keynames(import->getImportedKeys());

        new KgpgDetailedInfo(this, msg, rawmsgs.join( QLatin1String( "\n") ), keynames, i18n("Key Import" ));
        if (needsRefresh)
            imodel->refreshKeys(keys);
        else
            changeMessage(i18nc("Application ready for user input", "Ready"));
    } else{
        changeMessage(i18nc("Application ready for user input", "Ready"));
    }
*/
    //changeMessage(tr("Application ready for user input", "Ready"));
    mCtx->emitKeyDBChanged();
    import->deleteLater();
}
