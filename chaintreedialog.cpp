#include "chaintreedialog.h"
#include "ui_chaintreedialog.h"
#include <QStandardItemModel>

ChainTreeDialog::ChainTreeDialog(const QString &chain, const QString &treePath, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ChainTreeDialog)
{
    ui->setupUi(this);
    this->setWindowTitle("Дерево построения для цепочки: " + chain);
    ui->plainTextEdit->setPlainText(treePath);
}

ChainTreeDialog::~ChainTreeDialog()
{
    delete ui;
}
