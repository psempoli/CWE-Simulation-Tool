/*********************************************************************************
**
** Copyright (c) 2017 The University of Notre Dame
** Copyright (c) 2017 The Regents of the University of California
**
** Redistribution and use in source and binary forms, with or without modification,
** are permitted provided that the following conditions are met:
**
** 1. Redistributions of source code must retain the above copyright notice, this 
** list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright notice, this
** list of conditions and the following disclaimer in the documentation and/or other
** materials provided with the distribution.
**
** 3. Neither the name of the copyright holder nor the names of its contributors may
** be used to endorse or promote products derived from this software without specific
** prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
** EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
** SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
** TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
** BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
** IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
**
***********************************************************************************/

// Contributors:

#include "cwe_file_manager2.h"
#include "ui_cwe_file_manager2.h"

#include "../AgaveClientInterface/filemetadata.h"

#include "../AgaveExplorer/remoteFileOps/filetreenode.h"
#include "../AgaveExplorer/remoteFileOps/fileoperator.h"
#include "../AgaveExplorer/utilFuncs/singlelinedialog.h"

#include "vwtinterfacedriver.h"
#include "cwe_globals.h"

CWE_file_manager2::CWE_file_manager2(QWidget *parent) :
    CWE_Super(parent),
    ui(new Ui::CWE_file_manager2)
{
    ui->setupUi(this);

    // Creates our new model and populate
    localFileModel = new QFileSystemModel(this);

    // Set filter
    localFileModel->setFilter(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files);
    ui->localTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // Attach the model to the view
    ui->localTreeView->setModel(localFileModel);

    // QFileSystemModel requires root path
    localFileModel->setRootPath(QDir::homePath());
    ui->localTreeView->setRootIndex(localFileModel->setRootPath(QDir::homePath()));
}

CWE_file_manager2::~CWE_file_manager2()
{
    delete ui;
}

void CWE_file_manager2::linkDriver(VWTinterfaceDriver * theDriver)
{
    CWE_Super::linkDriver(theDriver);
    if (!myDriver->inOfflineMode())
    {
        ui->remoteTreeView->setFileOperator(myDriver->getFileHandler());
        ui->remoteTreeView->setupFileView();
        QObject::connect(ui->remoteTreeView, SIGNAL(customContextMenuRequested(QPoint)),
                         this, SLOT(customFileMenu(QPoint)));
        QObject::connect(myDriver->getFileHandler(), SIGNAL(fileOpDone(RequestState)),
                         this, SLOT(remoteOpDone()));
    }
}

void CWE_file_manager2::on_pb_upload_clicked()
{
    if (myDriver->getFileHandler()->operationIsPending())
    {
        cwe_globals::displayPopup("Currently running file operation. Please Wait.");
        return;
    }

    FileTreeNode * targetFile = ui->remoteTreeView->getSelectedNode();

    if ((targetFile == NULL) || (targetFile->getFileData().getFileType() != FileType::DIR))
    {
        cwe_globals::displayPopup("Please select a destination folder to upload to.");
        return;
    }

    QModelIndex localSelectIndex = ui->localTreeView->currentIndex();
    QFileInfo fileData = localFileModel->fileInfo(localSelectIndex);

    if (!fileData.isFile())
    {
        cwe_globals::displayPopup("Please select exactly 1 local file to upload.");
        return;
    }

    myDriver->getFileHandler()->sendUploadReq(targetFile, fileData.absoluteFilePath());

    if (!myDriver->getFileHandler()->operationIsPending())
    {
        cwe_globals::displayPopup("Error: Unable to start file operation. Please try again.");
        return;
    }
    ui->pb_upload->setDisabled(true);
    ui->pb_download->setDisabled(true);
}

void CWE_file_manager2::on_pb_download_clicked()
{
    if (myDriver->getFileHandler()->operationIsPending())
    {
        cwe_globals::displayPopup("Currently running file operation. Please Wait.");
        return;
    }

    FileTreeNode * targetFile = ui->remoteTreeView->getSelectedNode();

    if ((targetFile == NULL) || (targetFile->getFileData().getFileType() != FileType::FILE))
    {
        cwe_globals::displayPopup("Please select a file to download to.");
        return;
    }

    QModelIndex localSelectIndex = ui->localTreeView->currentIndex();
    QFileInfo fileData = localFileModel->fileInfo(localSelectIndex);

    if (!fileData.isDir())
    {
        cwe_globals::displayPopup("Please select exactly 1 local folder to download to");
        return;
    }

    QString localPath = fileData.absoluteFilePath();
#ifdef Q_OS_WIN
    localPath = localPath.append('\\');
#else
    localPath = localPath.append('/');
#endif

    localPath = localPath.append(targetFile->getFileData().getFileName());

    myDriver->getFileHandler()->sendDownloadReq(targetFile, localPath);

    if (!myDriver->getFileHandler()->operationIsPending())
    {
        cwe_globals::displayPopup("Error: Unable to start file operation. Please check that the local file does not already exist and try again.");
        return;
    }
    ui->pb_upload->setDisabled(true);
    ui->pb_download->setDisabled(true);
}

void CWE_file_manager2::copyMenuItem()
{
    SingleLineDialog newNamePopup("Please type a file name to copy to:", "newname");
    if (newNamePopup.exec() != QDialog::Accepted)
    {
        return;
    }

    ui->remoteTreeView->getFileOperator()->sendCopyReq(targetNode, newNamePopup.getInputText());
}

void CWE_file_manager2::moveMenuItem()
{
    SingleLineDialog newNamePopup("Please type a file name to move to:", "newname");

    if (newNamePopup.exec() != QDialog::Accepted)
    {
        return;
    }

    ui->remoteTreeView->getFileOperator()->sendMoveReq(targetNode,newNamePopup.getInputText());
}

void CWE_file_manager2::renameMenuItem()
{
    SingleLineDialog newNamePopup("Please type a new file name:", "newname");

    if (newNamePopup.exec() != QDialog::Accepted)
    {
        return;
    }

    ui->remoteTreeView->getFileOperator()->sendRenameReq(targetNode, newNamePopup.getInputText());
}

void CWE_file_manager2::deleteMenuItem()
{
    if (ui->remoteTreeView->getFileOperator()->deletePopup(targetNode))
    {
        ui->remoteTreeView->getFileOperator()->sendDeleteReq(targetNode);
    }
}

void CWE_file_manager2::createFolderMenuItem()
{
    SingleLineDialog newFolderNamePopup("Please input a name for the new folder:", "newFolder1");

    if (newFolderNamePopup.exec() != QDialog::Accepted)
    {
        return;
    }
    ui->remoteTreeView->getFileOperator()->sendCreateFolderReq(targetNode, newFolderNamePopup.getInputText());
}

void CWE_file_manager2::compressMenuItem()
{
    ui->remoteTreeView->getFileOperator()->sendCompressReq(targetNode);
}

void CWE_file_manager2::decompressMenuItem()
{
    ui->remoteTreeView->getFileOperator()->sendDecompressReq(targetNode);
}

void CWE_file_manager2::refreshMenuItem()
{
    ui->remoteTreeView->getFileOperator()->enactFolderRefresh(targetNode);
}

void CWE_file_manager2::downloadBufferItem()
{
    ui->remoteTreeView->getFileOperator()->sendDownloadBuffReq(targetNode);
}

void CWE_file_manager2::remoteOpDone()
{
    ui->pb_upload->setDisabled(false);
    ui->pb_download->setDisabled(false);
}

void CWE_file_manager2::customFileMenu(const QPoint &pos)
{
    QMenu fileMenu;
    if (ui->remoteTreeView->getFileOperator()->operationIsPending())
    {
        fileMenu.addAction("File Operation In Progress . . .");
        fileMenu.exec(QCursor::pos());
        return;
    }

    QModelIndex targetIndex = ui->remoteTreeView->indexAt(pos);
    ui->remoteTreeView->fileEntryTouched(targetIndex);

    targetNode = ui->remoteTreeView->getSelectedNode();

    //If we did not click anything, we should return
    if (targetNode == NULL) return;
    if (targetNode->isRootNode()) return;
    FileMetaData theFileData = targetNode->getFileData();

    if (theFileData.getFileType() == FileType::INVALID) return;
    if (theFileData.getFileType() == FileType::UNLOADED) return;
    if (theFileData.getFileType() == FileType::EMPTY_FOLDER) return;

    fileMenu.addAction("Copy To . . .",this, SLOT(copyMenuItem()));
    fileMenu.addAction("Move To . . .",this, SLOT(moveMenuItem()));
    fileMenu.addAction("Rename",this, SLOT(renameMenuItem()));
    //We don't let the user delete the username folder
    if (!(targetNode->getParentNode()->isRootNode()))
    {
        fileMenu.addSeparator();
        fileMenu.addAction("Delete",this, SLOT(deleteMenuItem()));
        fileMenu.addSeparator();
    }
    if (theFileData.getFileType() == FileType::DIR)
    {
        fileMenu.addAction("Create New Folder",this, SLOT(createFolderMenuItem()));
    }
    if (theFileData.getFileType() == FileType::FILE)
    {
        fileMenu.addAction("Download Buffer (DEBUG)",this, SLOT(downloadBufferItem()));
    }

    if ((theFileData.getFileType() == FileType::DIR) || (theFileData.getFileType() == FileType::FILE))
    {
        fileMenu.addSeparator();
        fileMenu.addAction("Refresh Data",this, SLOT(refreshMenuItem()));
        fileMenu.addSeparator();
    }

    fileMenu.exec(QCursor::pos());
}

void CWE_file_manager2::on_localButton_newFolder_clicked()
{

}

void CWE_file_manager2::on_localButton_deleteFolder_clicked()
{

}

void CWE_file_manager2::on_localButton_deleteFile_clicked()
{

}

void CWE_file_manager2::on_remoteButton_newFolder_clicked()
{

}

void CWE_file_manager2::on_remoteButton_deleteFolder_clicked()
{

}

void CWE_file_manager2::on_remoteButton_deleteFile_clicked()
{

}