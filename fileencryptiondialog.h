/*
 *      fileencryptiondialog.h
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

#ifndef __FILEENCRYPTIONDIALOG_H__
#define __FILEENCRYPTIONDIALOG_H__

#include "gpgcontext.h"
#include "keylist.h"
#include "verifydetailsdialog.h"
#include "kgpg/transactions/kgpgencrypt.h"
#include "kgpg/transactions/kgpgdecrypt.h"

QT_BEGIN_NAMESPACE
class QDialog;
class QLineEdit;
class QWidget;
class QDialogButtonBox;
class QLabel;
class QPushButton;
class QHBoxLayout;
class QVBoxLayout;
class QDebug;
class QFileDialog;
QT_END_NAMESPACE

/**
 * @brief
 *
 * @class FileEncryptionDialog fileencryptiondialog.h "fileencryptiondialog.h"
 */
class FileEncryptionDialog : public QDialog
{
    Q_OBJECT

public:

    enum DialogAction {
        Encrypt,
        Decrypt,
        Sign,
        Verify
    };

    /**
     * @brief
     *
     * @fn FileEncryptionDialog
     * @param ctx
     * @param keyList
     * @param parent
     */
    FileEncryptionDialog(GpgME::GpgContext *ctx, QStringList keyList,  DialogAction action, QWidget *parent = 0);
    void checkOutFileOK();

public slots:
    /**
     * @details
     *
     * @fn slotSelectInputFile
     */
    void slotSelectInputFile();

    /**
     * @brief
     *
     * @fn slotSelectOutputFile
     */
    void slotSelectOutputFile();

    /**
     * @brief
     *
     * @fn slotSelectSignFile
     */
    void slotSelectSignFile();

    /**
     * @brief
     *
     * @fn slotExecuteAction
     */
    void slotExecuteAction();

    void slotEncryptDone(int result);
    void slotDecryptDone(int result);

    /**
     * @brief
     *
     * @fn slotHideKeyList
     */
    void slotHideKeyList();

    /**
     * @brief
     *
     * @fn slotShowKeyList
     */
    void slotShowKeyList();

private:
    QLineEdit *outputFileEdit; /**< TODO */
    QLineEdit *inputFileEdit; /**< TODO */
    QLineEdit *signFileEdit; /**< TODO */
    DialogAction mAction; /**< TODO */
    QLabel *statusLabel; /**< TODO */
    QPushButton *okButton;

protected:
    GpgME::GpgContext *mCtx; /**< TODO */
    KeyList *mKeyList; /**< TODO */

};
#endif // __FILEENCRYPTIONDIALOG_H__
