#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListView>

namespace Grammars {
    struct CFG{
        QSet<QChar> terminals;
        QSet<QChar> nonterminals;
        QMap<QChar, QStringList> rules;
        QChar startSymbol{};

        void generateChains(const QString& currentChain, int minLength, int maxLength, QSet<QString>& result) {
            if (currentChain.length() > maxLength) return;

            bool isTerminalChain = true;
            for(auto ch: currentChain){
                if (nonterminals.contains(ch)){
                    isTerminalChain = false;
                    break;
                }
            }

            if (isTerminalChain && currentChain.length() >= minLength){
                result.insert(currentChain);
                return;
            }

            for(int i = 0; i < currentChain.length(); ++i){
                QChar symbol = currentChain[i];
                if (rules.contains(symbol)){
                    for(auto& rule: rules[symbol]){
                        QString newChain = currentChain;
                        newChain.removeAt(i);
                        for(int j = 0; j < rule.length(); ++j){
                            newChain.insert(i+j, rule[j]);
                        }
                        generateChains(newChain, minLength, maxLength, result);
                    }
                }
            }
        }

        QSet<QString> generateAllChains(int minLength, int maxLength) {
            QSet<QString> result;
            generateChains(QString{startSymbol}, minLength, maxLength, result);
            return result;
        }
    };

    struct Homskiy{
        QSet<QString> terminals;
        QSet<QString> nonterminals;
        QMap<QString, QList<QStringList>> rules;
        QString startSymbol{};

        void generateChains(const QStringList& currentChain, int minLength, int maxLength, QSet<QStringList>& result) {
            if (currentChain.length() > maxLength) return;

            bool isTerminalChain = true;
            for(auto& ch: currentChain){
                if (nonterminals.contains(ch)){
                    isTerminalChain = false;
                    break;
                }
            }

            if (isTerminalChain && currentChain.length() >= minLength){
                result.insert(currentChain);
                return;
            }

            for(int i = 0; i < currentChain.length(); ++i){
                QString symbol = currentChain[i];
                if (rules.contains(symbol)){
                    for(auto& rule: rules[symbol]){
                        QStringList newChain = currentChain;
                        newChain.removeAt(i);
                        for(int j = 0; j < rule.length(); ++j){
                            newChain.insert(i+j, rule[j]);
                        }
                        generateChains(newChain, minLength, maxLength, result);
                    }
                }
            }
        }

        QSet<QStringList> generateAllChains(int minLength, int maxLength) {
            QSet<QStringList> result;
            generateChains(QStringList{startSymbol}, minLength, maxLength, result);
            return result;
        }
    };
}

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    Grammars::CFG cfg;
    Grammars::Homskiy homsky;

    void updateUIRules(const Grammars::CFG& cfg);
    bool checkCanon(const Grammars::CFG& cfg);
    void translateToHomskiy();

private slots:
    void onLoadConfiguration();
    void onCalculateHomskiy();
    void onShowChains();
    void onCheckEqual();
    void showContextMenuCFG(const QPoint &pos);
    void showContextMenuHomskiy(const QPoint &pos);
    void addRuleCFG();
    void editRuleCFG(const QModelIndex &index);
    void removeRuleCFG(const QModelIndex &index);
    void buildRuleTreeCFG(const QModelIndex &index);
    void addRuleHomskiy();
    void editRuleHomskiy(const QModelIndex &index);
    void removeRuleHomskiy(const QModelIndex &index);
    void buildRuleTreeHomskiy(const QModelIndex &index);
};
#endif // MAINWINDOW_H
