#include "action.h"
#include <string>

bool orderByHeuristic(VizAction * a, VizAction * b){
    return a->heuristic > b->heuristic;
}



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
    string userInput = userInputText.toStdString();

    while(!actions.empty()){
        VizAction * a = actions[actions.size() - 1];
        actions.pop_back();
        delete a;
    }

    int actionIdentified = -1;    

    vector<int> actionsMissingLetters;
    vector<float> actionsCompletion;

    for(Phrase * action : actionPhrases){
        actionsMissingLetters.push_back(action->matchMissingLetters(userInput));
        actionsCompletion.push_back(action->matching(userInput));
    }
    
    for(int i = 0; i < actionsMissingLetters.size(); i++){
        if(actionsMissingLetters[i] < 2)actionIdentified = i;
    }

    vector<int> groupMissingLetters;
    vector<float> groupCompletion;

    for(Phrase * group : groupPhrases){
        groupMissingLetters.push_back(group->matchMissingLetters(userInput));
        groupCompletion.push_back(group->matching(userInput));
    }

    int highestCompletionGroup = std::distance(groupCompletion.begin(), std::max_element(groupCompletion.begin(), groupCompletion.end()));

    actions.push_back(new SelectAction(pcViz, dataSet, groups[highestCompletionGroup]));

    if(actionIdentified < 0){

    }else{
        
    }


    //std::cerr << "user input: " << userInput << std::endl;

    /*
    int counter = 0;
    for(VizAction * action : actions){
        float acc = 0;
        for(string tag : action->tags()){
            acc += std::pow(tagMatching(tag, userInput), 3);
           
        }
        acc += tagMatching(action->title(), userInput);
        action->heuristic = acc;
        counter++;
    }
    counter = 0;
    std::sort(actions.begin(), actions.end(), orderByHeuristic);

    float maxHeuristic = actions[0]->heuristic;
    */
    QStringList titles;
    for(VizAction * action : actions){
        titles.push_back(QString::fromStdString(action->title()));
    }
    
    QCompleter * completer = new QCompleter(titles);
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    completer->setCaseSensitivity(Qt::CaseInsensitive);

    searchbar->setCompleter(completer);
    
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
    sortActions();


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

    actionPhrases.push_back(new Phrase("select"));


    //create axis groups
    for(int i = 0; i < dataSet->getNumberOfAttributes() && i < 19; i++){
        groups.push_back(new Group(i, SampleAxes::SampleAxesNames[i].toStdString()));
        groupPhrases.push_back(new Phrase(SampleAxes::SampleAxesNames[i].toStdString()));
    }

/*
    CorrelateDatasourceInstructionLine *corDataInsLine = new CorrelateDatasourceInstructionLine(pcViz, dataSet);
    if(corDataInsLine->applicable())actions.push_back(corDataInsLine);

    CorrelateIBSL2TLBMissInstructionLine *corL2TLBInsLine = new CorrelateIBSL2TLBMissInstructionLine(pcViz, dataSet);
    if(corL2TLBInsLine->applicable())actions.push_back(corL2TLBInsLine);

    CorrelateIBSL1TLBMissInstructionLine *corL1TLBInsLine = new CorrelateIBSL1TLBMissInstructionLine(pcViz, dataSet);
    if(corL1TLBInsLine->applicable())actions.push_back(corL1TLBInsLine);

    CorrelateL1DCMissInstructionLine *corL1DCMissInsLine = new CorrelateL1DCMissInstructionLine(pcViz, dataSet);
    if(corL1DCMissInsLine->applicable())actions.push_back(corL1DCMissInsLine);

    CorrelateL2DCMissInstructionLine *corL2DCMissInsLine = new CorrelateL2DCMissInstructionLine(pcViz, dataSet);
    if(corL2DCMissInsLine->applicable())actions.push_back(corL2DCMissInsLine);

    CorrelateL1DCHitInstructionLine *corL1DCHitInsLine = new CorrelateL1DCHitInstructionLine(pcViz, dataSet);
    if(corL1DCHitInsLine->applicable())actions.push_back(corL1DCHitInsLine);

    CorrelateL2DCHitInstructionLine *corL2DCHitInsLine = new CorrelateL2DCHitInstructionLine(pcViz, dataSet);
    if(corL2DCHitInsLine->applicable())actions.push_back(corL2DCHitInsLine);

    CorrBrnMispSrcLine *corBrnMispSrcLine = new CorrBrnMispSrcLine(pcViz, dataSet);
    if(corBrnMispSrcLine->applicable())actions.push_back(corBrnMispSrcLine);

    CorrBrnMispSelect *corBrnMispSelect = new CorrBrnMispSelect(pcViz, dataSet);
    if(corBrnMispSelect->applicable())actions.push_back(corBrnMispSelect);

    CorrTagToRetSrc *corTagToRetSrc = new CorrTagToRetSrc(pcViz, dataSet);
    if(corTagToRetSrc->applicable())actions.push_back(corTagToRetSrc);

    CorrTagToRetIns *corTagToRetIns = new CorrTagToRetIns(pcViz, dataSet);
    if(corTagToRetIns->applicable())actions.push_back(corTagToRetIns);
*/

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