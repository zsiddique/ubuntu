#ifndef CONFIGUREARTNETOUTDIALOG_H
#define CONFIGUREARTNETOUTDIALOG_H

#include <QDialog>

class ArtNetOut;

namespace Ui
{
class ConfigureArtNetOutDialog;
}

class ConfigureArtNetOutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigureArtNetOutDialog(ArtNetOut* plugin, QWidget *parent = 0);
    ~ConfigureArtNetOutDialog();
    QString ip();
    void updateStatus();

private slots:
    void okButtonClicked();
private:
    Ui::ConfigureArtNetOutDialog *ui;
    ArtNetOut* m_plugin;
};

#endif // CONFIGUREARTNETOUTDIALOG_H
