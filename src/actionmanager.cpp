#include "action.h"

ActionManager::ActionManager(DataObject* dataSetIn, vector<QPushButton*> buttonsIn){
    dataSet = dataSetIn;
    buttons = buttonsIn;

    for(QPushButton* button : buttons){
        button->setText("this button belongs to ActionManager");
    }
}

void ActionManager::firstButton(){
    actionButtonClicked(0);
}

void ActionManager::secondButton(){
    actionButtonClicked(1);
}

void ActionManager::thirdButton(){
    actionButtonClicked(2);
}

void ActionManager::actionButtonClicked(int index){
    std::cerr << "clicked action button " << index << std::endl;       
}