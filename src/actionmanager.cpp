#include "action.h"

bool orderByHeuristic(VizAction* a, VizAction* b){
    return a->heuristic() > b->heuristic();
}

ActionManager::ActionManager(DataObject *dataSetIn, PCVizWidget *pcViz, QLineEdit *searchbarNew)
{
    dataSet = dataSetIn;
    this->pcViz = pcViz;
    this->searchbar = searchbarNew;
    searchbar->setPlaceholderText("type in action command");
    
}


void ActionManager::sortActions(){
    std::sort(actions.begin(), actions.end(), orderByHeuristic);
    
}

void ActionManager::textEdited(QString text){
    //connection text code
    //std::cerr << text.toStdString() << std::endl;


}

void ActionManager::returnPressed(){
    //connection test code
    //std::cerr << "action manager received return pressed signal\n";
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
    wordlist << "test 1" << "test 2";
    QCompleter* completer = new QCompleter(wordlist, this);
    completer->setCompletionMode(QCompleter::PopupCompletion);

    searchbar->setCompleter(completer);
}