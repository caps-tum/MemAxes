#ifndef VIZACTIONS_H
#define VIZACTIONS_H
#include "pcvizwidget.h"
#include <QPushButton>


class VizAction{
    public:
        virtual bool applicable(){return false;}
        virtual float heuristic(){return 0;}
        virtual void perform(){}
        virtual string title(){return "this should never show up";};
    protected:
        DataObject* dataSet;
        PCVizWidget* pcViz;


};

class CorrelateDatasourceInstructionLine: public VizAction{
    public:
        CorrelateDatasourceInstructionLine(PCVizWidget* pcVizIn, DataObject* dataSetIn);
        string title() override {return "Correlate Code Lines with Data Sources";};
        bool applicable() override;
        float heuristic() override;
        void perform() override;
};

class CorrelateIBSL2TLBMissInstructionLine: public VizAction{
    public:
        CorrelateIBSL2TLBMissInstructionLine(PCVizWidget* pcVizIn, DataObject* dataSetIn);
        string title() override {return "Correlate Code Lines with IBS L2 TLB data";};
        bool applicable() override;
        float heuristic() override;
        void perform() override;
};

class CorrelateIBSL1TLBMissInstructionLine: public VizAction{
    public:
        CorrelateIBSL1TLBMissInstructionLine(PCVizWidget* pcVizIn, DataObject* dataSetIn);
        string title() override {return "Correlate Code Lines with IBS L1 TLB data";};
        bool applicable() override;
        float heuristic() override;
        void perform() override;
};

class CorrelateL1DCMissInstructionLine: public VizAction{
    public:
        CorrelateL1DCMissInstructionLine(PCVizWidget* pcVizIn, DataObject* dataSetIn);
        string title() override {return "Correlate Code Lines with L1 Data Cache Misses";};
        bool applicable() override;
        float heuristic() override;
        void perform() override;
};

class ActionManager : public VizWidget{
    Q_OBJECT
    public:
        ActionManager(DataObject* dataSetIn, vector<QPushButton*> buttonsIn, PCVizWidget* pcViz);

    public slots:
        void firstButton();
        void secondButton();
        void thirdButton();

    private:
        void actionButtonClicked(int index);
        void sortActions();
        void bindButtonToAction(int buttonId, int actionId);
        

    private:
        vector<VizAction*> actions;
        vector<QPushButton*> buttons;
        vector<int> buttonActionMapping;
        DataObject* dataSet;
};

#endif