#ifndef VIZACTIONS_H
#define VIZACTIONS_H
#include "pcvizwidget.h"
#include <QPushButton>
#include <QLineEdit>
#include <QCompleter>
#include <QMessageBox>

#define DEBUG_OUTPUT true

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

class HelpAction : public VizAction{
public:
    HelpAction(PCVizWidget * pcVizIn, DataObject * dataSetIn) : VizAction(pcVizIn, dataSetIn){}
    bool applicable() override {return true;};
    void perform() override {
        QMessageBox help;
        help.setText("MemAxes accepted commands:\nSelect: select a value range\nHide: hide an axis in the parallel coordinates view\nCorrelate: move two axes in the parallel coordinates next to one another\nHelp: display this message");
        help.exec();
    }
    string title() override {return "Help";}
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
    void select(PCVizWidget * pcViz, int customMin, int customMax){
        if(customLimits()){
            if(relative)pcViz->selectValRelativeRange(dataIndex, min, max);
            else pcViz->selectValRange(dataIndex, min, max);
        }else{
            if(customMin >= 0 && customMax >= 0){
                pcViz->selectValRange(dataIndex, min, max);
            }else{
                if(customMin >= 0){
                    pcViz->selectValRange(dataIndex, customMin, pcViz->dataMax(dataIndex));
                }

                if(customMax >= 0){
                    pcViz->selectValRange(dataIndex, pcViz->dataMin(dataIndex), customMax);
                }
            }
        }
    }
};

class ProceduralAction : public VizAction
{
public:
    ProceduralAction(PCVizWidget *pcVizIn, DataObject *dataSetIn) : VizAction(pcVizIn, dataSetIn)
    {

    }

    bool applicable() override
    {
        return true;
    }

    

    bool customGroup(){return g1->customLimits();};


protected:
    Group *g1;
    Group *g2;
    

    
};

class SelectAction : public ProceduralAction
{
public:
    SelectAction(PCVizWidget *pcVizIn, DataObject *dataSetIn, Group *g1) : ProceduralAction(pcVizIn, dataSetIn)
    {
        this->g1 = g1;
        selectMin = -1;
        selectMax = -1;
    }

    void specifyG1Min(int min)
    {
        selectMin = min;
    }

    void specifyG1Max(int max)
    {
        selectMax = max;
    }

    
    void perform() override
    {
        if(DEBUG_OUTPUT)std::cerr << "selecting " << g1->name << " minimum: " << selectMin << " maximum " << selectMax << std::endl;
        pcViz->addAxis(g1->dataIndex);
        g1->select(pcViz, selectMin, selectMax);
    }

    string title() override
    {
        if(selectMax < 0)return "Select " + g1->name;
        return "Select " + g1->name + " minimum: " + std::to_string(selectMin) + " maximum: " + std::to_string(selectMax);
    }

    private:
    int selectMin;
    int selectMax;
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

class CorrelateAction : public ProceduralAction
{
public:
    CorrelateAction(PCVizWidget *pcVizIn, DataObject *dataSetIn, Group *g1, Group *g2) : ProceduralAction(pcVizIn, dataSetIn)
    {
        this->g1 = g1;
        this->g2 = g2;

        selectAbsMax1 = -1;
        selectAbsMax2 = -1;
        selectAbsMin1 = -1;
        selectAbsMin2 = -1;
    }

    void specifyG1Min(int min)
    {
        selectAbsMin1 = min;
    }

    void specifyG1Max(int max)
    {
        selectAbsMax1 = max;
    }

    void specifyG2Min(int min)
    {
        selectAbsMin2 = min;
    }

    void specifyG2Max(int max)
    {
        selectAbsMax2 = max;
    }

    
    void perform() override
    {
        if(DEBUG_OUTPUT)std::cerr << "correlating " << g1->name << " minimum: " << selectAbsMin1 << " maximum " << selectAbsMax1 << " with " << g2->name << " minimum: "<< selectAbsMin2 << " maximum: " << selectAbsMax2 << std::endl;
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
            }else{
                if(selectAbsMin1 >= 0){
                    pcViz->selectValRange(g1->dataIndex, selectAbsMin1, pcViz->dataMax(g1->dataIndex));
                }
            }
        }

        pcViz->addAxis(g2->dataIndex);
        if (g2->customLimits())
        {
            if (!g2->relative)
                pcViz->selectValRange(g2->dataIndex, g2->min, g2->max);
            else
                pcViz->selectValRelativeRange(g2->dataIndex, g2->min, g2->max);
        }else{
            if(selectAbsMax2 >= 0 && selectAbsMin2 >= 0){
                pcViz->selectValRange(g2->dataIndex, selectAbsMin2, selectAbsMax2);
            }else{
                if(selectAbsMin2 >= 0){
                    pcViz->selectValRange(g2->dataIndex, selectAbsMin2, pcViz->dataMax(g2->dataIndex));
                }
            }
        }

        pcViz->correlateAxes(g1->dataIndex, g2->dataIndex);
    }

    string title() override
    {
        string output = "Correlate " + g1->name;
        if(selectAbsMin1 >= 0)output = output + " min: " + std::to_string(selectAbsMin1);
        if(selectAbsMax1 >= 0)output = output + " max: " + std::to_string(selectAbsMax1);
        output = output + " with " + g2->name;
        if(selectAbsMin2 >= 0)output = output + " min: " + std::to_string(selectAbsMin2);
        if(selectAbsMax2 >= 0)output = output + " max: " + std::to_string(selectAbsMax2); 
        return output;
    }
    private:
    int selectAbsMin1;
    int selectAbsMax1;
    int selectAbsMin2;
    int selectAbsMax2;
};

class ShowAction : public ProceduralAction{
    public:
    ShowAction(PCVizWidget *pcVizIn, DataObject *dataSetIn, Group *g1) : ProceduralAction(pcVizIn, dataSetIn)
    {
        this->g1 = g1;
    }

    void perform()
    {
        
        pcViz->addAxis(g1->dataIndex);
    }

    string title() override
    {
        return "Show " + g1->name;
    }
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
    void completerActivated(QString text);
    void textChanged(QString text);

private:
    void sortActions();
    void generateActions();

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