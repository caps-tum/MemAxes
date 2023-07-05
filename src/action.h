#ifndef VIZACTIONS_H
#define VIZACTIONS_H
#include "vizwidget.h"
#include <QPushButton>


class VizAction{
    public:
        //virtual bool applicable();
        //virtual float heuristic();
        //virtual void perform();
        string title;
        DataObject* dataSet;


};

class CorrelateDatasourceInstructionLine: public VizAction{
    public:
        string title = "Correlate data source with source line";
};

class ActionManager : public VizWidget{
    Q_OBJECT
    public:
        ActionManager(DataObject* dataSetIn, vector<QPushButton*> buttonsIn);

    public slots:
        void firstButton();
        void secondButton();
        void thirdButton();

    private:
        void actionButtonClicked(int index);
    
    private:
        vector<VizAction> actions;
        vector<QPushButton*> buttons;
        DataObject* dataSet;
};

#endif