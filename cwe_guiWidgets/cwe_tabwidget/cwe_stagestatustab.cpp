#include "cwe_stagestatustab.h"
#include "ui_cwe_stagestatustab.h"
//#include <QPainter>
#include <QMouseEvent>

CWE_StageStatusTab::CWE_StageStatusTab(QString stageName, QWidget *parent) :
    QFrame(parent),
    ui(new Ui::CWE_StageStatusTab)
{
    ui->setupUi(this);
    this->setName(stageName);
    this->setInActive();
}

CWE_StageStatusTab::~CWE_StageStatusTab()
{
    delete ui;
}

void CWE_StageStatusTab::setCorrespondingPanel(CWE_GroupsWidget * newPanel)
{
    myPanel = newPanel;
}

void CWE_StageStatusTab::setStatus(QString s)
{
    m_status = s;
    ui->statusLabel->setText(s);
}

void CWE_StageStatusTab::setText(QString s)
{
    m_text = s;
    ui->mainLabel->setText(s);
}

void CWE_StageStatusTab::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        this->setActive(true);
        this->setStyleSheet("QFrame {background: #B0BEC5;}");
        emit btn_pressed(myPanel, m_name);
    }
}

void CWE_StageStatusTab::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        this->setActive();
        emit btn_released(myPanel);
    }
}

void CWE_StageStatusTab::setActive(bool b)
{
    this->setStyleSheet("QFrame {background: #64B5F6; border-color: #808080; border-width: 1.5px; border-radius: 3px; border-style: inset;} QLabel {border-style: none}");
    if (!b) this->setInActive();
    m_active = b;
}

void CWE_StageStatusTab::setInActive(bool b)
{
    this->setStyleSheet("QFrame {background: #C0C0C8; border-color: #808080; border-width: 2px; border-radius: 3px; border-style: onset;} QLabel {border-style: none}");
    if (!b) this->setActive();
    m_active = !b;
}


void CWE_StageStatusTab::setName(const QString s)
{
    m_name = s;
    ui->mainLabel->setText(s);
}

void CWE_StageStatusTab::linkWidget(CWE_GroupsWidget *ptr)
{
    myPanel = ptr;
}
