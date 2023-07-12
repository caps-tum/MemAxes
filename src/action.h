#ifndef VIZACTIONS_H
#define VIZACTIONS_H
#include "pcvizwidget.h"
#include <QPushButton>
#include <QLineEdit>
#include <QCompleter>

class VizAction
{
public:
    VizAction(PCVizWidget *pcVizIn, DataObject *dataSetIn) : pcViz(pcVizIn), dataSet(dataSetIn) {}
    virtual bool applicable() { return false; }
    virtual void perform() {}
    virtual string title() { return "this should never show up"; }
    virtual vector<string> tags() = 0;
    float heuristic = 0;

protected:
    DataObject *dataSet;
    PCVizWidget *pcViz;
    int ibsAxisAvailable(string axisName)
    {
        for (int i = 0; i < dataSet->getNumberOfAttributes(); i++)
        {
            if (dataSet->GetAttributeName(i) == axisName)
                return i;
        }
        return -1;
    }
};

class CorrelateDatasourceInstructionLine : public VizAction
{
public:
    CorrelateDatasourceInstructionLine(PCVizWidget *pcVizIn, DataObject *dataSetIn) : VizAction(pcVizIn, dataSetIn) {}
    string title() override { return "Correlate Code Lines with Data Sources"; };
    bool applicable() override;
    void perform() override;
    vector<string> tags() override { return {"code line", "source line", "data source"}; }
};

class CorrelateIBSL2TLBMissInstructionLine : public VizAction
{
public:
    CorrelateIBSL2TLBMissInstructionLine(PCVizWidget *pcVizIn, DataObject *dataSetIn) : VizAction(pcVizIn, dataSetIn) {}
    string title() override { return "Correlate Code Lines with IBS L2 TLB data"; };
    bool applicable() override;
    void perform() override;
    vector<string> tags() override { return {"l2", "lookaside", "translation"}; }

private:
    int l2tlbmissIndex;
};

class CorrelateIBSL1TLBMissInstructionLine : public VizAction
{
public:
    CorrelateIBSL1TLBMissInstructionLine(PCVizWidget *pcVizIn, DataObject *dataSetIn) : VizAction(pcVizIn, dataSetIn) {}
    string title() override { return "Correlate Code Lines with IBS L1 TLB data"; };
    bool applicable() override;
    void perform() override;
    vector<string> tags() override { return {"tlb", "lookaside", "buffer", "l1", "tlb l1", "source line"}; }

private:
    int l1tlbmissIndex;
};

class CorrelateL1DCMissInstructionLine : public VizAction
{
public:
    CorrelateL1DCMissInstructionLine(PCVizWidget *pcVizIn, DataObject *dataSetIn) : VizAction(pcVizIn, dataSetIn) {}
    string title() override { return "Correlate Code Lines with L1 Data Cache Misses"; };
    bool applicable() override;
    void perform() override;
    vector<string> tags() override { return {"cache miss", "l1", "dc miss", "select", "source line"}; }
};

class CorrelateL2DCMissInstructionLine : public VizAction
{
public:
    CorrelateL2DCMissInstructionLine(PCVizWidget *pcVizIn, DataObject *dataSetIn) : VizAction(pcVizIn, dataSetIn) {}
    string title() override { return "Correlate Code Lines with L2 Data Cache Misses"; };
    bool applicable() override;
    void perform() override;
    vector<string> tags() override { return {"cache miss", "l2", "dc miss", "select", "source line"}; }
};

class CorrelateL1DCHitInstructionLine : public VizAction
{
public:
    CorrelateL1DCHitInstructionLine(PCVizWidget *pcVizIn, DataObject *dataSetIn) : VizAction(pcVizIn, dataSetIn) {}
    string title() override { return "Correlate Code Lines with L1 Data Cache Hits"; };
    bool applicable() override;
    void perform() override;
    vector<string> tags() override { return {"cache miss", "l1", "dc hit", "select", "source line"}; }
};

class CorrelateL2DCHitInstructionLine : public VizAction
{
public:
    CorrelateL2DCHitInstructionLine(PCVizWidget *pcVizIn, DataObject *dataSetIn) : VizAction(pcVizIn, dataSetIn) {}
    string title() override { return "Correlate Code Lines with L2 Data Cache Hits"; };
    bool applicable() override;
    void perform() override;
    vector<string> tags() override { return {"cache miss", "l2", "dc hit", "select", "source line"}; }
};


class CorrBrnMispSrcLine : public VizAction
{
public:
    CorrBrnMispSrcLine(PCVizWidget *pcVizIn, DataObject *dataSetIn) : VizAction(pcVizIn, dataSetIn) {}
    string title() override { return "Correlate Branch mispredictions with source lines"; };
    bool applicable() override;
    void perform() override;
    vector<string> tags() override { return {"misprediction", "branch", "source line"}; }
private:
    int brnMispDataIndex;
};

class CorrBrnMispSelect : public VizAction
{
public:
    CorrBrnMispSelect(PCVizWidget *pcVizIn, DataObject *dataSetIn) : VizAction(pcVizIn, dataSetIn) {}
    string title() override { return "Select Branch mispredictions"; };
    bool applicable() override;
    void perform() override;
    vector<string> tags() override { return {"misprediction", "branch", "select"}; }
private:
    int brnMispDataIndex;
};

class CorrTagToRetSrc : public VizAction
{
    public: 
    CorrTagToRetSrc(PCVizWidget *pcVizIn, DataObject *dataSetIn) : VizAction(pcVizIn, dataSetIn){}
    string title() override { return "Correlate Tag to Retire counter with Source Line"; };
    bool applicable() override;
    void perform() override;
    vector<string> tags() override { return {"tag to", "retire", "correlate", "source"}; }
private:
    int tagToRetDataIndex;
};

class CorrTagToRetIns : public VizAction
{
    public: 
    CorrTagToRetIns(PCVizWidget *pcVizIn, DataObject *dataSetIn) : VizAction(pcVizIn, dataSetIn){}
    string title() override { return "Correlate Tag to Retire counter with Instruction"; };
    bool applicable() override;
    void perform() override;
    vector<string> tags() override { return {"tag to", "retire", "correlate", "instruction"}; }
private:
    int tagToRetDataIndex;
};

class ActionManager : public VizWidget
{
    Q_OBJECT
public:
    ActionManager(DataObject *dataSetIn, PCVizWidget *pcViz, QLineEdit *searchbarNew);
    void loadDataset(DataObject *dataSetIn);

public slots:
    void returnPressed();
    void textEdited(QString text);

    void textChanged(QString text);

private:
    void sortActions();

    VizAction *findActionByTitle(string title);

private:
    vector<VizAction *> actions;
    DataObject *dataSet;
    PCVizWidget *pcViz;
    QLineEdit *searchbar;
    QString inputText;
    QString userInputText;
};

#endif