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

#include "cwe_manage_simulation.h"
#include "ui_cwe_manage_simulation.h"

#include "CFDanalysis/CFDanalysisType.h"
#include "CFDanalysis/CFDcaseInstance.h"

#include "../popupWindows/create_case_popup.h"
#include "../popupWindows/duplicate_case_popup.h"

#include "mainWindow/cwe_mainwindow.h"

#include "cwe_interfacedriver.h"
#include "cwe_globals.h"

CWE_manage_simulation::CWE_manage_simulation(QWidget *parent) :
    CWE_Super(parent),
    ui(new Ui::CWE_manage_simulation)
{
    ui->setupUi(this);

    QObject::connect(ui->treeView, SIGNAL(newFileSelected(FileNodeRef)),
                             this, SLOT(newFileSelected(FileNodeRef)));

    clearSelectView();

    ui->stageListView->setModel(&stageListModel);
}

CWE_manage_simulation::~CWE_manage_simulation()
{
    delete ui;
}

void CWE_manage_simulation::linkDriver()
{
    cwe_globals::get_CWE_Driver()->getMainWindow()->getFileModel()->linkRemoteFileTreeToModel(ui->treeView);
    QObject::connect(cwe_globals::get_CWE_Driver(), SIGNAL(haveNewCase()),
                     this, SLOT(newCaseGiven()));
}

void CWE_manage_simulation::newFileSelected(FileNodeRef newFile)
{
    if (!newFile.fileNodeExtant())
    {
        cwe_globals::get_CWE_Driver()->setCurrentCase();
        return;
    }

    cwe_globals::get_CWE_Driver()->setCurrentCase(newFile);
}

void CWE_manage_simulation::newCaseGiven()
{
    CFDcaseInstance * newCase = cwe_globals::get_CWE_Driver()->getCurrentCase();

    //TODO: This code is to fix problem with inconsistant case selection
    //Should clear slection if new case is not same as already selected folder
    //if ((newCase->getCaseFolder().fileNodeExtant()) &&
    //    (newCase->getCaseFolder() != ui->treeView->getSelectedFile()))
    //{
    //    ui->treeView->selectFileByNode(newCase->getCaseFolder());
    //}

    ui->label_caseStatus->setCurrentCase(newCase);    

    ui->treeView->setEnabled(true);
    clearSelectView();

    ui->pb_duplicateCase->setEnabled(false);
    ui->pb_viewParameters->setEnabled(false);
    ui->pb_viewResults->setEnabled(false);

    if (newCase != NULL)
    {
        ui->label_caseName->setText(newCase->getCaseName());

        QObject::connect(newCase, SIGNAL(haveNewState(CaseState)),
                         this, SLOT(newCaseState(CaseState)));
        newCaseState(newCase->getCaseState());
    }
    else
    {
        ui->treeView->clearSelection();
    }
}

void CWE_manage_simulation::newCaseState(CaseState newState)
{
    if (newState == CaseState::DOWNLOAD)
    {
        //TODO: This should be visible but unclickable
        ui->treeView->setEnabled(false);

        showSelectView();
        ui->pb_duplicateCase->setEnabled(false);
        ui->pb_viewParameters->setEnabled(false);
        ui->pb_viewResults->setEnabled(false);
        return;
    }

    ui->treeView->setEnabled(true);

    if ((newState == CaseState::DEFUNCT) ||
            (newState == CaseState::ERROR) ||
            (newState == CaseState::INVALID) ||
            (newState == CaseState::LOADING))
    {
        if (newState == CaseState::DEFUNCT)
        {
            ui->treeView->clearSelection();
        }
        if (newState != CaseState::LOADING)
        {
            clearSelectView();
            ui->pb_duplicateCase->setEnabled(false);
            ui->pb_viewParameters->setEnabled(false);
            ui->pb_viewResults->setEnabled(false);
        }
        return;
    }

    if ((newState == CaseState::RUNNING) ||
            (newState == CaseState::READY) ||
            (newState == CaseState::EXTERN_OP))
    {
        ui->pb_duplicateCase->setEnabled(true);
        ui->pb_viewParameters->setEnabled(true);
        ui->pb_viewResults->setEnabled(true);

        CFDcaseInstance * theCase = cwe_globals::get_CWE_Driver()->getCurrentCase();

        CFDanalysisType * theType = theCase->getMyType();
        if (theType == NULL)
        {
            cwe_globals::get_CWE_Driver()->fatalInterfaceError("Type/stage mismatch for case.");
            return;
        }
        ui->label_CaseTypeIcon->setPixmap(theType->getIcon()->pixmap(150,100));

        QMap<QString, StageState> stages = theCase->getStageStates();
        stageListModel.clear();

        for (auto itr = stages.cbegin(); itr != stages.cend(); itr++)
        {
            QString aLine = theCase->getMyType()->translateStageId(itr.key());
            aLine = aLine.append(" - ");
            aLine = aLine.append(getStateText(*itr));
            stageListModel.appendRow(new QStandardItem(aLine));
        }

        showSelectView();
    }
}

void CWE_manage_simulation::create_new_case_clicked()
{
    Create_Case_Popup * createCase = new Create_Case_Popup(this);
    createCase->show();
}

void CWE_manage_simulation::duplicate_case_clicked()
{
    FileNodeRef slectedFolder = ui->treeView->getSelectedFile();
    if (slectedFolder.isNil())
    {
        cwe_globals::displayPopup("Please select a folder to duplicate.");
        return;
    }
    if (slectedFolder.getFileType() != FileType::DIR)
    {
        cwe_globals::displayPopup("Please select a folder to duplicate.");
        return;
    }

    Duplicate_Case_Popup * duplicateCase = new Duplicate_Case_Popup(slectedFolder, this);
    duplicateCase->show();
}

void CWE_manage_simulation::on_pb_viewParameters_clicked()
{
    // switch main window to parameters tab
    cwe_globals::get_CWE_Driver()->getMainWindow()->switchToParameterTab();
}

void CWE_manage_simulation::on_pb_viewResults_clicked()
{
    // switch main window to results tab
    cwe_globals::get_CWE_Driver()->getMainWindow()->switchToResultsTab();
}

void CWE_manage_simulation::clearSelectView()
{
    ui->label_caseTypeTag->setVisible(false);
    ui->label_CaseTypeIcon->setVisible(false);
    ui->label_stageListTag->setVisible(false);
    ui->stageListView->setVisible(false);
}

void CWE_manage_simulation::showSelectView()
{
    ui->label_caseTypeTag->setVisible(true);
    ui->label_CaseTypeIcon->setVisible(true);
    ui->label_stageListTag->setVisible(true);
    ui->stageListView->setVisible(true);
}

QString CWE_manage_simulation::getStateText(StageState theState)
{
    if (theState == StageState::DOWNLOADING)
        return "Downloading Case";
    if (theState == StageState::ERROR)
        return "*** ERROR ***";
    if (theState == StageState::FINISHED)
        return "Finished";
    if (theState == StageState::FINISHED_PREREQ)
        return "Finished";
    if (theState == StageState::LOADING)
        return "Loading ...";
    if (theState == StageState::OFFLINE)
        return "Offline (Debug)";
    if (theState == StageState::RUNNING)
        return "Running ...";
    if (theState == StageState::UNREADY)
        return "Needs Prev. Stage";
    if (theState == StageState::UNRUN)
        return "Not Yet Run";
    return "*** TOTAL ERROR ***";
}
