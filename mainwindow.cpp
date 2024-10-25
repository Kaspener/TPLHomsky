#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "chaintreedialog.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QChar>
#include <QList>
#include <QMap>
#include <QString>
#include <QDebug>
#include <QFileDialog>
#include <QStringListModel>
#include <QInputDialog>
#include <QMessageBox>
#include <QStandardItemModel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->loadConfigurationButton, &QPushButton::clicked, this, &MainWindow::onLoadConfiguration);
    connect(ui->calculateHomskiy, &QPushButton::clicked, this, &MainWindow::onCalculateHomskiy);
    connect(ui->showChains, &QPushButton::clicked, this, &MainWindow::onShowChains);
    connect(ui->checkEqualButton, &QPushButton::clicked, this, &MainWindow::onCheckEqual);

    ui->listCFG->setEditTriggers(QAbstractItemView::DoubleClicked);
    ui->listCFG->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listCFG, &QListView::customContextMenuRequested, this, &MainWindow::showContextMenuCFG);

    ui->listHomskiy->setEditTriggers(QAbstractItemView::DoubleClicked);
    ui->listHomskiy->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listHomskiy, &QListView::customContextMenuRequested, this, &MainWindow::showContextMenuHomskiy);

    ui->errorLabel->hide();
    ui->homskiyRules->hide();
    ui->calculateHomskiy->hide();
    ui->lableHomsky->hide();
    ui->lableCFG->hide();
    ui->showChains->hide();
    ui->left->hide();
    ui->right->hide();
    ui->listCFG->hide();
    ui->listHomskiy->hide();
    ui->checkEqualButton->hide();
}

Grammars::CFG parseCFGFromJson(const QString& filePath) {

    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open the file!";
        return {};
    }

    QByteArray fileData = file.readAll();
    file.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(fileData);
    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        qWarning() << "Invalid JSON format!";
        return {};
    }

    QJsonObject jsonObj = jsonDoc.object();

    Grammars::CFG cfg;

    QJsonArray terminalsArray = jsonObj["terminals"].toArray();
    for (const QJsonValue& terminalValue : terminalsArray) {
        cfg.terminals.insert(terminalValue.toString().at(0));
    }

    QJsonArray nonterminalsArray = jsonObj["nonterminals"].toArray();
    for (const QJsonValue& nonterminalValue : nonterminalsArray) {
        cfg.nonterminals.insert(nonterminalValue.toString().at(0));
    }

    QJsonObject rulesObj = jsonObj["rules"].toObject();
    for (auto it = rulesObj.begin(); it != rulesObj.end(); ++it) {
        QChar key = it.key().at(0);
        QJsonArray ruleArray = it.value().toArray();
        QStringList ruleList;
        for (const QJsonValue& ruleValue : ruleArray) {
            ruleList.append(ruleValue.toString());
        }
        cfg.rules.insert(key, ruleList);
    }

    cfg.startSymbol = jsonObj["startSymbol"].toString().at(0);

    return cfg;
}

template<typename T>
QString genGrammar(const T& cfg){
    QString rules = "G({" + QStringList(cfg.terminals.begin(), cfg.terminals.end()).join(", ");
    rules += QString("},{") + QStringList(cfg.nonterminals.begin(), cfg.nonterminals.end()).join(", ");
    rules += "},P,";
    rules += cfg.startSymbol;
    rules += "):";
    return rules;
}

void MainWindow::updateUIRules(const Grammars::CFG& cfg){
    QString rules = genGrammar<Grammars::CFG>(cfg);
    rules += "\n\n";
    for (auto it = cfg.rules.begin(); it != cfg.rules.end(); ++it) {
        QString key = it.key();
        QList<QString> ruleList = it.value();

        rules += key;
        rules += " → ";
        rules += ruleList.join(" | ");
        rules += "\n";
    }
    ui->rules->setPlainText(rules);
}

bool termsAndNontermsIsDifferent(const Grammars::CFG &cfg){
    for(auto it: cfg.terminals){
        if (cfg.nonterminals.contains(it))
            return false;
    }
    return true;
}

bool isValidSymbolsInRules(const Grammars::CFG &cfg){
    for(auto it = cfg.rules.begin(); it != cfg.rules.end(); ++it){
        QStringList ruleList = it.value();
        for(auto& rule: ruleList){
            for (auto& symbol: rule){
                if ((!cfg.terminals.contains(symbol) && !cfg.nonterminals.contains(symbol) && rule != "λ") || (symbol==cfg.startSymbol && it.key() != cfg.startSymbol))
                    return false;
            }
        }
    }
    return true;
}

bool isValidKeysInRules(const Grammars::CFG &cfg){
    for(auto it = cfg.rules.begin(); it != cfg.rules.end(); ++it){
        QChar key = it.key();
        if (!cfg.nonterminals.contains(key)){
            return false;
        }
    }
    return true;
}

bool hasLambdas(const Grammars::CFG &cfg){
    for(auto it = cfg.rules.begin(); it != cfg.rules.end(); ++it){
        QStringList ruleList = it.value();
        for(auto& rule: ruleList){
            if (it.key() == cfg.startSymbol)
                continue;
            if (rule == "λ")
                return true;
        }
    }
    return false;
}

bool hasCicles(const Grammars::CFG &cfg){
    for(auto it = cfg.rules.begin(); it != cfg.rules.end(); ++it){
        QChar key = it.key();
        QStringList ruleList = it.value();
        for(auto& rule: ruleList){
            if (rule.length() == 1 && rule[0] != key && cfg.nonterminals.contains(rule[0]))
                return true;
        }
    }
    return false;
}

bool hasUseless(const Grammars::CFG &cfg){
    QSet<QChar> usefull;
    auto onlyTerms = [&cfg, &usefull](const QString& str) -> bool{
        for(QChar it: str){
            if (cfg.nonterminals.contains(it) && !usefull.contains(it))
                return false;
        }
        return true;
    };
    int lastCount = -1;
    int curCount = 0;
    while(curCount != lastCount){
        lastCount = curCount;
        for(auto it = cfg.rules.begin(); it != cfg.rules.end(); ++it){
            QChar key = it.key();
            if (usefull.contains(key)) continue;
            QStringList ruleList = it.value();
            for(auto& rule: ruleList){
                if (onlyTerms(rule))
                {
                    usefull.insert(key);
                    curCount++;
                    break;
                }
            }
        }
    }
    return !(usefull.size() == cfg.nonterminals.size());
}

bool hasUnattainable(const Grammars::CFG &cfg){
    QSet<QChar> symbols;
    symbols.insert(cfg.startSymbol);
    int lastCount = -1;
    int curCount = 0;
    while(curCount != lastCount){
        lastCount = curCount;
        QSet<QChar> temp = symbols;
        for(auto it = temp.begin(); it != temp.end(); ++it){
            if (!cfg.nonterminals.contains(*it)) continue;
            QChar key = *it;
            QStringList ruleList = cfg.rules[key];
            for(auto& rule: ruleList){
                for(auto symbol: rule){
                    if (!symbols.contains(symbol)){
                        symbols.insert(symbol);
                        curCount++;
                    }
                }
            }
        }
    }
    return !(symbols.size() == (cfg.nonterminals.size() + cfg.terminals.size()));
}

bool MainWindow::checkCanon(const Grammars::CFG &cfg)
{
    if (!termsAndNontermsIsDifferent(cfg)){
        ui->errorLabel->setText("Терминал содержится в нетерменалах или наоборот");
        return false;
    }
    if (!isValidSymbolsInRules(cfg)){
        ui->errorLabel->setText("Правила КС-грамматики содержат невозможные символы");
        return false;
    }
    if (!isValidKeysInRules(cfg)){
        ui->errorLabel->setText("Ключи правил КС-грамматики содержат невозможные символы");
        return false;
    }
    if (hasLambdas(cfg)){
        ui->errorLabel->setText("Правила КС-грамматики содержат λ");
        return false;
    }
    if (hasCicles(cfg)){
        ui->errorLabel->setText("Правила КС-грамматики содержат циклы");
        return false;
    }
    if (hasUseless(cfg)){
        ui->errorLabel->setText("КС-грамматика содержат бесполезные нетерминалы");
        return false;
    }
    if (hasUnattainable(cfg)){
        ui->errorLabel->setText("КС-грамматика содержат недостижимые символы");
        return false;
    }
    return true;
}

void insertRuleIntoHomskyGrammar(Grammars::Homskiy &homsky, const QString& key, const QStringList& rule){
    if (homsky.rules.contains(key)){
        if (!homsky.rules[key].contains(rule))
            homsky.rules[key].append(rule);
    }
    else{
        QList<QStringList> list{rule};
        homsky.rules[key] = list;
        homsky.nonterminals.insert(key);
    }
}

void homskyStep(Grammars::Homskiy &homskiy, QStringList& rule){
    if (rule.size() == 2 && homskiy.nonterminals.contains(rule[0]) && homskiy.terminals.contains(rule[1])){  //rule 3
        QString newKey = "<" + rule[1] + ">";
        QStringList newValue{rule[1]};
        rule[1] = newKey;
        insertRuleIntoHomskyGrammar(homskiy, newKey, newValue);
    }
    else if (rule.size() == 2 && homskiy.nonterminals.contains(rule[1]) && homskiy.terminals.contains(rule[0])){ //rule 2
        QString newKey = "<" + rule[0] + ">";
        QStringList newValue{rule[0]};
        rule[0] = newKey;
        insertRuleIntoHomskyGrammar(homskiy, newKey, newValue);
    }
    else if (rule.size() == 2 && homskiy.terminals.contains(rule[0]) && homskiy.terminals.contains(rule[1])){ //rule 4
        QString newKey1 = "<" + rule[0] + ">";
        QString newKey2 = "<" + rule[1] + ">";
        QStringList newValue1{rule[0]};
        QStringList newValue2{rule[1]};
        rule[0] = newKey1;
        rule[1] = newKey2;
        insertRuleIntoHomskyGrammar(homskiy, newKey1, newValue1);
        insertRuleIntoHomskyGrammar(homskiy, newKey2, newValue2);
    }
    else if (rule.size() > 2){
        QStringList newRule = rule;
        QString freeRule = rule[0];
        newRule.remove(0, 1);
        if (homskiy.terminals.contains(freeRule)){
            QStringList term {freeRule};
            freeRule = "<" + freeRule + ">";
            insertRuleIntoHomskyGrammar(homskiy, freeRule, term);
        }
        QString newRuleString = "<" + newRule.join("") + ">";
        rule = {freeRule, newRuleString};
        homskyStep(homskiy, newRule);
        insertRuleIntoHomskyGrammar(homskiy, newRuleString, newRule);
    }
}

Grammars::Homskiy makeHomskyFromCFG(const Grammars::CFG &cfg){
    Grammars::Homskiy homskiy;
    for(auto it: cfg.terminals)
        homskiy.terminals.insert(QString(it));
    for(auto it: cfg.nonterminals)
        homskiy.nonterminals.insert(QString(it));
    for(auto it = cfg.rules.begin(); it != cfg.rules.end(); ++it){
        QList<QStringList> lst;
        for(const QString& string: it.value()){
            QStringList curString;
            for(QChar symbol: string){
                curString.append(symbol);
            }
            lst.append(curString);
        }
        homskiy.rules[it.key()] = lst;
    }
    homskiy.startSymbol = cfg.startSymbol;

    for(auto it = homskiy.rules.begin(); it != homskiy.rules.end(); ++it){
        QString key = it.key();
        QList<QStringList> rulesList = it.value();
        for(auto& rule: rulesList){
            homskyStep(homskiy, rule);
        }
        homskiy.rules[key] = rulesList;
    }

    return homskiy;
}

void MainWindow::translateToHomskiy()
{
    homsky = makeHomskyFromCFG(cfg);

    QString rules = genGrammar<Grammars::Homskiy>(homsky);
    rules += "\n\n";
    for (auto it = homsky.rules.begin(); it != homsky.rules.end(); ++it) {
        QString key = it.key();
        QList<QStringList> ruleList = it.value();

        rules += key;
        rules += " → ";
        for (const QStringList& list : ruleList) {
            rules += list.join("");

            if (list != ruleList.last()) {
                rules += " | ";
            }
        }

        rules += "\n";
    }
    ui->homskiyRules->setPlainText(rules);
}

void MainWindow::onLoadConfiguration(){
    QString filePath = QFileDialog::getOpenFileName(this, "Выберите JSON файл", "", "JSON Files (*.json)");

    if (filePath.isEmpty()) {
        qDebug() << "Файл не выбран";
        return;
    }
    ui->calculateHomskiy->show();
    cfg = parseCFGFromJson(filePath);
    updateUIRules(cfg);
    ui->errorLabel->hide();
    ui->homskiyRules->hide();
    ui->lableHomsky->hide();
    ui->lableCFG->hide();
    ui->showChains->hide();
    ui->left->hide();
    ui->right->hide();
    ui->listCFG->hide();
    ui->listHomskiy->hide();
    ui->checkEqualButton->hide();
}

void MainWindow::onCalculateHomskiy()
{
    if(!checkCanon(cfg)){
        ui->errorLabel->show();
        return;
    }
    ui->lableCFG->show();
    ui->lableHomsky->show();
    ui->homskiyRules->show();
    translateToHomskiy();
    ui->calculateHomskiy->hide();
    ui->showChains->show();
    ui->left->show();
    ui->right->show();
    ui->checkEqualButton->hide();
}

void displayChainsInListView(const QSet<QStringList>& chains, QStandardItemModel* model) {
    model->clear();
    QList<QString> sortedChains;
    for (const QStringList& chain : chains) {
        sortedChains.append(chain.join(""));
    }
    std::sort(sortedChains.begin(), sortedChains.end());

    for (const QString& chain : sortedChains) {
        QStandardItem *item = new QStandardItem(chain);
        model->appendRow(item);
    }
}

void displayChainsInListView(const QSet<QString>& chains, QStandardItemModel* model) {
    model->clear();
    QList<QString> sortedChains = chains.values();
    std::sort(sortedChains.begin(), sortedChains.end());

    for (const QString& chain : sortedChains) {
        QStandardItem *item = new QStandardItem(chain);
        model->appendRow(item);
    }
}

void MainWindow::onShowChains()
{
    ui->listCFG->show();
    ui->listHomskiy->show();
    QStandardItemModel* modelHomskiy = new QStandardItemModel(this);
    QStandardItemModel* modelCFG = new QStandardItemModel(this);

    ui->listCFG->setModel(modelCFG);
    ui->listHomskiy->setModel(modelHomskiy);

    QSet<QString> cfgChains;
    cfg.generateAllChains(ui->left->value(), ui->right->value(), cfgChains, cfgTrees);

    QSet<QStringList> homskiyChains;
    homsky.generateAllChains(ui->left->value(), ui->right->value(), homskiyChains, homskiyTrees);

    displayChainsInListView(cfgChains, modelCFG);
    displayChainsInListView(homskiyChains, modelHomskiy);
    ui->checkEqualButton->show();

}

void MainWindow::onCheckEqual()
{
    QStandardItemModel* modelCFG = qobject_cast<QStandardItemModel*>(ui->listCFG->model());
    QStandardItemModel* modelHomskiy = qobject_cast<QStandardItemModel*>(ui->listHomskiy->model());

    QStringList items1, items2;

    for (int row = 0; row < modelCFG->rowCount(); ++row) {
        items1 << modelCFG->item(row)->text();
    }

    for (int row = 0; row < modelHomskiy->rowCount(); ++row) {
        items2 << modelHomskiy->item(row)->text();
    }

    if (items1 == items2) {
        QMessageBox::information(this, "Результат", "Все элементы одинаковы.");
        return;
    }
    QMessageBox::information(this, "Результат", "Есть разные элементы. Они помечены красным.");

    for (int row = 0; row < modelCFG->rowCount(); ++row) {
        QStandardItem *item = modelCFG->item(row);
        int conts = items2.contains(item->text());
        if (conts != 1) {
            item->setBackground(QColor(Qt::red));
        }
    }

    for (int row = 0; row < modelHomskiy->rowCount(); ++row) {
        QStandardItem *item = modelHomskiy->item(row);
        int conts = items1.contains(item->text());
        if (conts != 1) {
            item->setBackground(QColor(Qt::red));
        }
    }
}

void MainWindow::showContextMenuCFG(const QPoint &pos) {
    QModelIndex index = ui->listCFG->indexAt(pos);
    QMenu contextMenu(this);

    QAction *addAction = contextMenu.addAction("Добавить");
    QAction *editAction = contextMenu.addAction("Редактировать");
    QAction *removeAction = contextMenu.addAction("Удалить");
    QAction *buildTree = contextMenu.addAction("Построить дерево вывода");

    connect(addAction, &QAction::triggered, this, &MainWindow::addRuleCFG);
    connect(editAction, &QAction::triggered, this, [this, index]() {
        if (index.isValid()) {
            editRuleCFG(index);
        } else {
            QMessageBox::warning(this, "Ошибка", "Ничего не выбрано для редактирования.");
        }
    });
    connect(removeAction, &QAction::triggered, this, [this, index]() {
        if (index.isValid()) {
            removeRuleCFG(index);
        } else {
            QMessageBox::warning(this, "Ошибка", "Ничего не выбрано для удаления.");
        }
    });
    connect(buildTree, &QAction::triggered, this, [this, index]() {
        if (index.isValid()) {
            buildRuleTreeCFG(index);
        } else {
            QMessageBox::warning(this, "Ошибка", "Ничего не выбрано для построения дерева.");
        }
    });

    contextMenu.exec(ui->listCFG->viewport()->mapToGlobal(pos));
}

void MainWindow::showContextMenuHomskiy(const QPoint &pos) {
    QModelIndex index = ui->listHomskiy->indexAt(pos);
    QMenu contextMenu(this);

    QAction *addAction = contextMenu.addAction("Добавить");
    QAction *editAction = contextMenu.addAction("Редактировать");
    QAction *removeAction = contextMenu.addAction("Удалить");
    QAction *buildTree = contextMenu.addAction("Построить дерево вывода");

    connect(addAction, &QAction::triggered, this, &MainWindow::addRuleHomskiy);
    connect(editAction, &QAction::triggered, this, [this, index]() {
        if (index.isValid()) {
            editRuleHomskiy(index);
        } else {
            QMessageBox::warning(this, "Ошибка", "Ничего не выбрано для редактирования.");
        }
    });
    connect(removeAction, &QAction::triggered, this, [this, index]() {
        if (index.isValid()) {
            removeRuleHomskiy(index);
        } else {
            QMessageBox::warning(this, "Ошибка", "Ничего не выбрано для удаления.");
        }
    });
    connect(buildTree, &QAction::triggered, this, [this, index]() {
        if (index.isValid()) {
            buildRuleTreeHomskiy(index);
        } else {
            QMessageBox::warning(this, "Ошибка", "Ничего не выбрано для построения дерева.");
        }
    });

    contextMenu.exec(ui->listHomskiy->viewport()->mapToGlobal(pos));
}

void MainWindow::addRuleCFG() {
    bool ok;
    QString text = QInputDialog::getText(this, "Добавить правило", "Имя правила:", QLineEdit::Normal, "", &ok);
    if (ok && !text.isEmpty()) {
        auto* model = qobject_cast<QStandardItemModel*>(ui->listCFG->model());
        if (model) {
            QStandardItem* item = new QStandardItem(text);
            model->appendRow(item);
        }
    }
}

void MainWindow::editRuleCFG(const QModelIndex &index) {
    bool ok;
    QString currentText = ui->listCFG->model()->data(index).toString();
    QString text = QInputDialog::getText(this, "Редактировать правило", "Имя правила:", QLineEdit::Normal, currentText, &ok);
    if (ok && !text.isEmpty()) {
        ui->listCFG->model()->setData(index, text);
    }
}

void MainWindow::removeRuleCFG(const QModelIndex &index) {
    ui->listCFG->model()->removeRow(index.row());
}

void MainWindow::buildRuleTreeCFG(const QModelIndex &index)
{
    QString chain = ui->listCFG->model()->data(index).toString();
    QString treePath = cfgTrees[chain];

    ChainTreeDialog *dialog = new ChainTreeDialog(chain, treePath, this);
    dialog->show();
}

void MainWindow::addRuleHomskiy() {
    bool ok;
    QString text = QInputDialog::getText(this, "Добавить правило", "Имя правила:", QLineEdit::Normal, "", &ok);
    if (ok && !text.isEmpty()) {
        auto* model = qobject_cast<QStandardItemModel*>(ui->listHomskiy->model());
        if (model) {
            QStandardItem* item = new QStandardItem(text);
            model->appendRow(item);
        }
    }
}

void MainWindow::editRuleHomskiy(const QModelIndex &index) {
    bool ok;
    QString currentText = ui->listHomskiy->model()->data(index).toString();
    QString text = QInputDialog::getText(this, "Редактировать правило", "Имя правила:", QLineEdit::Normal, currentText, &ok);
    if (ok && !text.isEmpty()) {
        ui->listHomskiy->model()->setData(index, text);
    }
}

void MainWindow::removeRuleHomskiy(const QModelIndex &index) {
    ui->listHomskiy->model()->removeRow(index.row());
}

void MainWindow::buildRuleTreeHomskiy(const QModelIndex &index)
{
    QString chain = ui->listHomskiy->model()->data(index).toString();
    QString treePath = homskiyTrees[chain];

    ChainTreeDialog *dialog = new ChainTreeDialog(chain, treePath, this);
    dialog->show();
}

MainWindow::~MainWindow()
{
    delete ui;
}
