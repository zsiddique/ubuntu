#include "configureartnetoutdialog.h"
#include "ui_configureartnetoutdialog.h"

#include "artnetout.h"

ConfigureArtNetOutDialog::ConfigureArtNetOutDialog(ArtNetOut * plugin, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigureArtNetOutDialog)
{
    m_plugin = plugin;

    ui->setupUi(this);

    if (plugin->m_ip == QString("")) {
        qDebug("string is empty\n") ;
        ui-> m_deviceEdit->setText("");
    } else {
        ui->m_deviceEdit->setText(plugin->m_ip);
    }
    connect(ui->buttonBox , SIGNAL(accepted()), this, SLOT(okButtonClicked()));

    updateStatus();
}

ConfigureArtNetOutDialog::~ConfigureArtNetOutDialog()
{
    delete ui;
}

void ConfigureArtNetOutDialog::okButtonClicked()
{
    m_plugin->newIp(ui->m_deviceEdit->text());
}

QString ConfigureArtNetOutDialog::ip()
{
    return ui->m_deviceEdit->text();
}

void ConfigureArtNetOutDialog::updateStatus()
{
    if (m_plugin->isOpen()) {
        ui->m_statusLabel->setText("Art-Net is Active");
    } else {
        ui->m_statusLabel->setText("Art-Net is InActive");
    }
}
