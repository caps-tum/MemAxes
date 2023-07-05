#include "action.h"

bool orderByHeuristic(VizAction* a, VizAction* b){
    return a->heuristic() > b->heuristic();
}

ActionManager::ActionManager(DataObject *dataSetIn, vector<QPushButton *> buttonsIn, PCVizWidget *pcViz)
{
    dataSet = dataSetIn;
    buttons = buttonsIn;

    for (QPushButton *button : buttons)
    {
        button->setText("this button belongs to ActionManager");
        buttonActionMapping.push_back(-1);
    }

    CorrelateDatasourceInstructionLine *corDataInsLine = new CorrelateDatasourceInstructionLine(pcViz, dataSet);
    actions.push_back(corDataInsLine);

    CorrelateIBSL2TLBMissInstructionLine *corL2TLBInsLine = new CorrelateIBSL2TLBMissInstructionLine(pcViz, dataSet);
    actions.push_back(corL2TLBInsLine);

    bindButtonToAction(0, 0);
    bindButtonToAction(1,1);

    sortActions();
}

void ActionManager::firstButton()
{
    actionButtonClicked(0);
}

void ActionManager::secondButton()
{
    actionButtonClicked(1);
}

void ActionManager::thirdButton()
{
    actionButtonClicked(2);
}

void ActionManager::sortActions(){
    std::sort(actions.begin(), actions.end(), orderByHeuristic);
    for(int i = 0; i < buttons.size() && i < actions.size(); i++){
        bindButtonToAction(i, i);
    }
}

void ActionManager::actionButtonClicked(int index)
{
    if (buttonActionMapping[index] < 0)
    {
        std::cerr << "clicked unassigned button with index " << index << std::endl;
        return;
    }

    actions[buttonActionMapping[index]]->perform();
}

void ActionManager::bindButtonToAction(int buttonId, int actionId)
{
    buttons[buttonId]->setText(QString::fromStdString(actions[actionId]->title()));
    buttonActionMapping[buttonId] = actionId;
}