#include "chainmodal.h"
#include "ui_chainmodal.h"
#include <QStandardItemModel>

struct NodeCFG {
    QString chain;
    QString appliedRule;
    std::shared_ptr<NodeCFG> parent;
    QList<std::shared_ptr<NodeCFG>> children;

    NodeCFG(const QString& chain, const QString& rule, std::shared_ptr<NodeCFG> parent = nullptr)
        : chain(chain), appliedRule(rule), parent(parent) {}
};

void generateChainsTree(const QString& targetChain, const Grammars::CFG& cfg, const QString& currentChain, std::shared_ptr<NodeCFG> parentNode = nullptr) {
    // Если текущая цепочка совпадает с целевой цепочкой, добавляем её в дерево
    if (currentChain == targetChain) {
        auto newNode = std::make_shared<NodeCFG>(currentChain, "Output: " + currentChain, parentNode);
        if (parentNode) {
            parentNode->children.append(newNode);
        }
        return;
    }

    // Если длина превышает целевую цепочку, прекращаем выполнение
    if (currentChain.length() > targetChain.length()) return;

    // Проходим по всем символам текущей цепочки
    for (int i = 0; i < currentChain.length(); ++i) {
        QChar symbol = currentChain[i];
        if (cfg.rules.contains(symbol)) {
            for (const auto& rule : cfg.rules[symbol]) {
                QString newChain = currentChain;
                newChain.removeAt(i);
                for (int j = 0; j < rule.length(); ++j) {
                    newChain.insert(i + j, rule[j]);
                }

                // Если новый вариант не равен целевой цепочке, продолжаем генерировать
                if (newChain != targetChain) {
                    // Рекурсивно продолжаем строить дерево
                    generateChainsTree(targetChain, cfg, newChain, parentNode);
                } else {
                    // Если новый вариант равен целевой цепочке, добавляем узел
                    QString ruleStr = QString(symbol) + " -> " + rule;
                    auto newNode = std::make_shared<NodeCFG>(newChain, ruleStr, parentNode);
                    if (parentNode) {
                        parentNode->children.append(newNode);
                    }
                }
            }
        }
    }
}

void generateAllChains(const QString& targetChain, const Grammars::CFG& cfg, std::shared_ptr<NodeCFG>& rootNode) {
    rootNode = std::make_shared<NodeCFG>("Start: " + targetChain, "", nullptr);
    generateChainsTree(targetChain, cfg, QString{cfg.startSymbol}, rootNode);
}

void addNodeToModel(std::shared_ptr<NodeCFG> node, QStandardItem* parentItem) {
    QStandardItem* item = new QStandardItem(node->appliedRule.isEmpty() ? node->chain : node->appliedRule);
    parentItem->appendRow(item);

    for (const auto& child : node->children) {
        addNodeToModel(child, item);
    }
}

ChainModal::ChainModal(const QString& targetChain, const Grammars::CFG& cfg, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ChainModal)
{
    ui->setupUi(this);
    QStandardItemModel* model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels({"Chain Derivation"});

    std::shared_ptr<NodeCFG> rootNode;
    generateAllChains(targetChain, cfg, rootNode);

    QStandardItem* rootItem = new QStandardItem("Start Chain: " + targetChain);
    model->appendRow(rootItem);
    addNodeToModel(rootNode, rootItem);

    ui->treeView->setModel(model);
    ui->treeView->expandAll();
}

ChainModal::~ChainModal() {
    delete ui;
}
