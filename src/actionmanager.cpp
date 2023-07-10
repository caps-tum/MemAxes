#include "action.h"

ActionManager::ActionManager(DataObject *dataSetIn, PCVizWidget *pcViz, QLineEdit *searchbarNew)
{
    dataSet = dataSetIn;
    this->pcViz = pcViz;
    this->searchbar = searchbarNew;
    searchbar->setPlaceholderText("type in action command");
    inputText = "";
    userInputText = "";
    
}


void ActionManager::sortActions(){


}

VizAction* ActionManager::findActionByTitle(string title){
    for(VizAction* action : actions){
        if(action->title() == title)return action;
    }
    return nullptr;
}

void ActionManager::textEdited(QString text){
    //connection text code
    //std::cerr << "textedit: " << text.toStdString() << std::endl;
    userInputText = text;


}

void ActionManager::textChanged(QString text){
    //std::cerr << "text change: " << text.toStdString() << std::endl;
    inputText = text;
}

void ActionManager::returnPressed(){
    VizAction * action = findActionByTitle(inputText.toStdString());
    if(action != nullptr){
        action->perform();
    }
    searchbar->clear();
}



void ActionManager::loadDataset(DataObject* dataSetIn){
    dataSet = dataSetIn;

    CorrelateDatasourceInstructionLine *corDataInsLine = new CorrelateDatasourceInstructionLine(pcViz, dataSet);
    if(corDataInsLine->applicable())actions.push_back(corDataInsLine);

    CorrelateIBSL2TLBMissInstructionLine *corL2TLBInsLine = new CorrelateIBSL2TLBMissInstructionLine(pcViz, dataSet);
    if(corL2TLBInsLine->applicable())actions.push_back(corL2TLBInsLine);

    CorrelateIBSL1TLBMissInstructionLine *corL1TLBInsLine = new CorrelateIBSL1TLBMissInstructionLine(pcViz, dataSet);
    if(corL1TLBInsLine->applicable())actions.push_back(corL1TLBInsLine);

    CorrelateL1DCMissInstructionLine *corL1DCMissInsLine = new CorrelateL1DCMissInstructionLine(pcViz, dataSet);
    if(corL1DCMissInsLine->applicable())actions.push_back(corL1DCMissInsLine);

    sortActions();

    QStringList wordlist;
    for(VizAction* action : actions){
        wordlist.push_back(QString::fromStdString(action->title()));
    }
    QCompleter* completer = new QCompleter(wordlist, this);
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    completer->setCaseSensitivity(Qt::CaseInsensitive);

    searchbar->setCompleter(completer);
}