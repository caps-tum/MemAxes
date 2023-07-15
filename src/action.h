#ifndef VIZACTIONS_H
#define VIZACTIONS_H
#include "pcvizwidget.h"
#include <QPushButton>
#include <QLineEdit>
#include <QCompleter>

class Phrase
{
public:
    Phrase(string phrase)
    {
        str_tolower(&phrase);
        word = phrase;
    }
    string phrase() { return word; };
    float matching(string query)
    {
        str_tolower(&query);
        return (float)lcs(word, query) / (float)word.length();
    }

    float matchMissingLetters(string query){
        str_tolower(&query);
        return word.length() - lcs(word, query);
    }

private:
    string word;
    int lcs(string a, string b)
    {
        int cache[(a.length() + 1) * (b.length() + 1)];

        for (int i = 0; i < (a.length() + 1) * (b.length() + 1); i++)
        {
            cache[i] = 0;
        }

        for (int i = 0; i < b.length(); i++)
        {
            for (int j = 0; j < a.length(); j++)
            {
                if (a.at(j) == b.at(i))
                {
                    cache[(i + 1) * (a.length() + 1) + (j + 1)] = cache[i * (a.length() + 1) + j] + 1;
                }
                else
                {
                    cache[(i + 1) * (a.length() + 1) + (j + 1)] = std::max(cache[(i + 1) * (a.length() + 1) + (j)], cache[(i) * (a.length() + 1) + (j + 1)]);
                }
            }
        }
        return cache[(a.length() + 1) * (b.length()) + a.length()];
    }

    void str_tolower(string *s)
    {
        for (int i = 0; i < s->length(); i++)
            s->at(i) = tolower(s->at(i));
    }
};

class VizAction
{
public:
    VizAction(PCVizWidget *pcVizIn, DataObject *dataSetIn) : pcViz(pcVizIn), dataSet(dataSetIn) {}
    virtual bool applicable() { return false; }
    virtual void perform() {}
    virtual string title() { return "this should never show up"; }
    // virtual vector<string> tags() = 0;
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

struct Group
{
    int dataIndex;
    float min = 0;
    float max = 1;
    bool relative = true;
    string name;
    Group(int index, string nName) : dataIndex(index), name(nName) {}
    bool customLimits() { return max != 1 || min != 0; }
};

class ProceduralAction : public VizAction
{
public:
    ProceduralAction(PCVizWidget *pcVizIn, DataObject *dataSetIn) : VizAction(pcVizIn, dataSetIn)
    {
        selectAbsMax1 = -1;
        selectAbsMax2 = -1;
        selectAbsMin1 = -1;
        selectAbsMin2 = -1;
    }

    bool applicable() override
    {
        return true;
    }

protected:
    Group *g1;
    Group *g2;
    int selectAbsMin1;
    int selectAbsMax1;
    int selectAbsMin2;
    int selectAbsMax2;

    void specifyG2Min(int min)
    {
        selectAbsMin2 = min;
    }

    void specifyG2Max(int max)
    {
        selectAbsMax2 = max;
    }
};

class SelectAction : public ProceduralAction
{
public:
    SelectAction(PCVizWidget *pcVizIn, DataObject *dataSetIn, Group *g1) : ProceduralAction(pcVizIn, dataSetIn)
    {
        this->g1 = g1;
    }

    void specifyG1Min(int min)
    {
        selectAbsMin1 = min;
    }

    void specifyG1Max(int max)
    {
        selectAbsMax1 = max;
    }

    void perform() override
    {
        pcViz->addAxis(g1->dataIndex);
        if (!g1->relative)
            pcViz->selectValRange(g1->dataIndex, g1->min, g1->max);
        else
            pcViz->selectValRelativeRange(g1->dataIndex, g1->min, g1->max);
    }

    string title() override
    {
        return "Select " + g1->name;
    }
};

/*
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
    string title() override { return "Correlate Code Lines with IBS L2 TLB information"; };
    bool applicable() override;
    void perform() override;
    vector<string> tags() override { return {"tlb", "l2", "lookaside", "translation"}; }

private:
    int l2tlbmissIndex;
};

class CorrelateIBSL1TLBMissInstructionLine : public VizAction
{
public:
    CorrelateIBSL1TLBMissInstructionLine(PCVizWidget *pcVizIn, DataObject *dataSetIn) : VizAction(pcVizIn, dataSetIn) {}
    string title() override { return "Correlate Code Lines with IBS L1 TLB information"; };
    bool applicable() override;
    void perform() override;
    vector<string> tags() override { return {"tlb", "l1", "lookaside", "translation"}; }

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
    vector<string> tags() override { return {"cache", "miss", "l1", "dc miss", "select", "source line"}; }
};

class CorrelateL2DCMissInstructionLine : public VizAction
{
public:
    CorrelateL2DCMissInstructionLine(PCVizWidget *pcVizIn, DataObject *dataSetIn) : VizAction(pcVizIn, dataSetIn) {}
    string title() override { return "Correlate Code Lines with L2 Data Cache Misses"; };
    bool applicable() override;
    void perform() override;
    vector<string> tags() override { return {"cache", "miss", "l2", "dc miss", "select", "source line"}; }
};

class CorrelateL1DCHitInstructionLine : public VizAction
{
public:
    CorrelateL1DCHitInstructionLine(PCVizWidget *pcVizIn, DataObject *dataSetIn) : VizAction(pcVizIn, dataSetIn) {}
    string title() override { return "Correlate Code Lines with L1 Data Cache Hits"; };
    bool applicable() override;
    void perform() override;
    vector<string> tags() override { return {"cache hit", "l1", "dc hit", "select", "source line"}; }
};

class CorrelateL2DCHitInstructionLine : public VizAction
{
public:
    CorrelateL2DCHitInstructionLine(PCVizWidget *pcVizIn, DataObject *dataSetIn) : VizAction(pcVizIn, dataSetIn) {}
    string title() override { return "Correlate Code Lines with L2 Data Cache Hits"; };
    bool applicable() override;
    void perform() override;
    vector<string> tags() override { return {"cache hit", "l2", "dc hit", "select", "source line"}; }
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
*/
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
    vector<Group *> groups;
    vector<Phrase *> groupPhrases;
    vector<Phrase *> actionPhrases;
    DataObject *dataSet;
    PCVizWidget *pcViz;
    QLineEdit *searchbar;
    QString inputText;
    QString userInputText;
};

#endif