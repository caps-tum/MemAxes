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
    float matchingLCS(string query)
    {
        str_tolower(&query);
        return (float)lcs(word, query) / (float)word.length();
    }

    float matchingLevenshtein(string query)
    {
        str_tolower(&query);
        // std::cerr << "matching \"" << query << "\" to \"" << word << "\" : " << levenshtein_distance(word, query, 1, .3, 2.) << std::endl;
        return levenshtein_distance(word, query, 1, .3, 2.) / word.length();
    }

    float matchMissingLetters(string query)
    {
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

    float levenshtein_distance(string a, string b, float insertion_cost, float deletion_cost, float substitution_cost)
    {
        int row_length = b.length() + 1;
        int cache_size = (a.length() + 1) * row_length;
        float cache[cache_size];

        for (int j = 0; j < row_length; j++)
            cache[j] = j;

        for (int i = 1; i < a.length() + 1; i++)
            cache[i * row_length] = i;

        for (int j = 1; j < row_length; j++)
        {
            for (int i = 1; i < a.length() + 1; i++)
            {
                float this_sub_cost = substitution_cost;
                if (a.at(i - 1) == b.at(j - 1))
                    this_sub_cost = 0;

                float nValue = std::min(cache[(i - 1) * row_length + j] + deletion_cost, cache[i * row_length + (j - 1)] + insertion_cost);
                nValue = std::min(nValue, cache[(i - 1) * row_length + (j - 1)] + this_sub_cost);

                cache[i * row_length + j] = nValue;
            }
        }
        return cache[row_length * a.length() + b.length()];
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
    bool customLimits() { return max != 1 || min != 0 || !relative; }
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

    void specifyG1Min(int min)
    {
        selectAbsMin1 = min;
    }

    void specifyG1Max(int max)
    {
        selectAbsMax1 = max;
    }

    bool customGroup(){return g1->customLimits();};


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

    
    void perform() override
    {
        std::cerr << "selecting " << g1->name << " minimum: " << selectAbsMin1 << " maximum " << selectAbsMax1 << std::endl;
        pcViz->addAxis(g1->dataIndex);
        if (g1->customLimits())
        {
            if (!g1->relative)
                pcViz->selectValRange(g1->dataIndex, g1->min, g1->max);
            else
                pcViz->selectValRelativeRange(g1->dataIndex, g1->min, g1->max);
        }else{
            if(selectAbsMax1 >= 0 && selectAbsMin1 >= 0){
                pcViz->selectValRange(g1->dataIndex, selectAbsMin1, selectAbsMax1);
            }
        }
    }

    string title() override
    {
        if(selectAbsMax1 < 0)return "Select " + g1->name;
        return "Select " + g1->name + " minimum: " + std::to_string(selectAbsMin1) + " maximum: " + std::to_string(selectAbsMax1);
    }
};

class HideAction : public ProceduralAction
{
public:
    HideAction(PCVizWidget *pcVizIn, DataObject *dataSetIn, Group *g1) : ProceduralAction(pcVizIn, dataSetIn)
    {
        this->g1 = g1;
    }

    void perform()
    {
        if(pcViz->hasAxis(g1->dataIndex))pcViz->selectValRelativeRange(g1->dataIndex, 0, 1);
        pcViz->removeAxis(g1->dataIndex);
    }

    string title() override
    {
        return "Hide " + g1->name;
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
    vector<ProceduralAction *> actions;
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