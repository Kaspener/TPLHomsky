#ifndef CHAINTREEDIALOG_H
#define CHAINTREEDIALOG_H

#include <QDialog>

namespace Ui {
class ChainTreeDialog;
}

class ChainTreeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChainTreeDialog(const QString& chain, const QString& treePath, QWidget *parent = nullptr);
    ~ChainTreeDialog();

private:
    Ui::ChainTreeDialog *ui;
};

#endif // CHAINTREEDIALOG_H
