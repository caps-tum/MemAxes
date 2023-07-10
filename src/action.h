#ifndef VIZACTIONS_H
#define VIZACTIONS_H
#include "pcvizwidget.h"
#include <QPushButton>
#include <QLineEdit>
#include <QCompleter>


class VizAction{
    public:
        virtual bool applicable(){return false;}
        virtual void perform(){}
        virtual string title(){return "this should never show up";};
    protected:
        DataObject* dataSet;
        PCVizWidget* pcViz;
        int ibsAxisAvailable(string axisName){
            for(int i = 0; i < dataSet->getNumberOfAttributes(); i++){
                if(dataSet->GetAttributeName(i) == axisName)return i;
            }
            return -1;
        }

};

class CorrelateDatasourceInstructionLine: public VizAction{
    public:
        CorrelateDatasourceInstructionLine(PCVizWidget* pcVizIn, DataObject* dataSetIn);
        string title() override {return "Correlate Code Lines with Data Sources";};
        bool applicable() override;
        void perform() override;
};

class CorrelateIBSL2TLBMissInstructionLine: public VizAction{
    public:
        CorrelateIBSL2TLBMissInstructionLine(PCVizWidget* pcVizIn, DataObject* dataSetIn);
        string title() override {return "Correlate Code Lines with IBS L2 TLB data";};
        bool applicable() override;
        void perform() override;
    private:
        int l2tlbmissIndex;
};

class CorrelateIBSL1TLBMissInstructionLine: public VizAction{
    public:
        CorrelateIBSL1TLBMissInstructionLine(PCVizWidget* pcVizIn, DataObject* dataSetIn);
        string title() override {return "Correlate Code Lines with IBS L1 TLB data";};
        bool applicable() override;
        void perform() override;
    private:
        int l1tlbmissIndex;
};

class CorrelateL1DCMissInstructionLine: public VizAction{
    public:
        CorrelateL1DCMissInstructionLine(PCVizWidget* pcVizIn, DataObject* dataSetIn);
        string title() override {return "Correlate Code Lines with L1 Data Cache Misses";};
        bool applicable() override;
        void perform() override;
};

class ActionManager : public VizWidget{
    Q_OBJECT
    public:
        ActionManager(DataObject* dataSetIn, PCVizWidget* pcViz, QLineEdit* searchbarNew);
        void loadDataset(DataObject* dataSetIn);

    public slots:
        void returnPressed();
        void textEdited(QString text);

        void textChanged(QString text);

    private:
        void sortActions();

        VizAction *findActionByTitle(string title);

    private:
        vector<VizAction*> actions;
        DataObject* dataSet;
        PCVizWidget* pcViz;
        QLineEdit* searchbar;
        QString inputText;
        QString userInputText;
};

#endif