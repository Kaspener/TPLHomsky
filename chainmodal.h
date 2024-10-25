#ifndef CHAINMODAL_H
#define CHAINMODAL_H

#include <QDialog>
#include "mainwindow.h"

namespace Ui {
class ChainModal;
}

class ChainModal : public QDialog
{
    Q_OBJECT

public:
    explicit ChainModal(const QString& chain, const Grammars::CFG& cfg, QWidget *parent = nullptr);
    explicit ChainModal(const QString& chain, const Grammars::Homskiy& homskiy, QWidget *parent = nullptr);
    ~ChainModal();

private:
    Ui::ChainModal *ui;
};

#endif // CHAINMODAL_H
