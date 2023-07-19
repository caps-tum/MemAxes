#include "action.h"
#include <string>

bool orderByHeuristic(VizAction *a, VizAction *b)
{
    return a->heuristic < b->heuristic;
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

void phraseMatch(vector<Phrase *> phrases, string query, vector<float> *result)
{
    for (Phrase *p : phrases)
    {
        result->push_back(p->matchingLevenshtein(query));
    }
}

void ActionManager::sortActions()
{
    string userInput = userInputText.toStdString();

    QStringList split = QString::fromStdString(userInput).split(' ');

    vector<int> numbers;
    vector<string> sentences;

    string acc = "";
    int cnt = 0;

    for (QString s : split)
    {
        bool ok = true;
        s.toInt(&ok);
        if (ok)
        {
            sentences.push_back(acc);
            numbers.push_back(s.toInt());
        }
        else
        {
            acc = acc + " " + s.toStdString();
        }
    }

    if (acc != "")
        sentences.push_back(acc);

    while (!actions.empty())
    {
        VizAction *a = actions[actions.size() - 1];
        actions.pop_back();
        delete a;
    }

    int actionIdentified = -1;

    vector<float> actionsCompletion;
    if (!split.empty())
        phraseMatch(actionPhrases, split[0].toStdString(), &actionsCompletion);

    for (int i = 0; i < actionsCompletion.size(); i++)
    {
        if (actionsCompletion[i] < 2)
            actionIdentified = i;
    }

    vector<float> groupCompletionFirstSentence;
    if (!sentences.empty())
        phraseMatch(groupPhrases, sentences[0], &groupCompletionFirstSentence);

    // int highestCompletionGroup = std::distance(groupCompletion.begin(), std::max_element(groupCompletion.begin(), groupCompletion.end()));

    if (actionIdentified < 0)
    {
        for (int i = 0; i < groups.size(); i++)
        {
            actions.push_back(new SelectAction(pcViz, dataSet, groups[i]));
            actions[actions.size() - 1]->heuristic = groupPhrases[i]->matchingLevenshtein(userInput);

            if (pcViz->hasAxis(groups[i]->dataIndex) && !groups[i]->customLimits())
            {
                actions.push_back(new HideAction(pcViz, dataSet, groups[i]));
                actions[actions.size() - 1]->heuristic = groupPhrases[i]->matchingLevenshtein(userInput);
            }
        }
    }
    else
    {
        for (int i = 0; i < groups.size(); i++)
        {
            actions.push_back(new SelectAction(pcViz, dataSet, groups[i]));
            actions[actions.size() - 1]->heuristic = groupPhrases[i]->matchingLevenshtein(userInput);
        }
    }

    if (numbers.size() > 1)
    {
        for (ProceduralAction *a : actions)
        {
            if(!a->customGroup())a->specifyG1Min(numbers[0]);
        }

        for (ProceduralAction *a : actions)
        {
            if(!a->customGroup())a->specifyG1Max(numbers[1]);
        }
    }

    std::sort(actions.begin(), actions.end(), orderByHeuristic);

    // set up QStringList and QCompleter
    QStringList titles;
    for (VizAction *action : actions)
    {
        titles.push_back(QString::fromStdString(action->title()));
    }

    QStringListModel *model = new QStringListModel(titles);

    // QCompleter *completer = new QCompleter(titles);
    // completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    // completer->setCaseSensitivity(Qt::CaseInsensitive);

    if (searchbar->completer() != nullptr)
        searchbar->completer()->setModel(model);
}

VizAction *ActionManager::findActionByTitle(string title)
{
    for (VizAction *action : actions)
    {
        if (action->title() == title)
            return action;
    }
    return nullptr;
}

void ActionManager::textEdited(QString text)
{
    // connection text code
    // std::cerr << "textedit: " << text.toStdString() << std::endl;
    userInputText = text;
    sortActions();
}

void ActionManager::textChanged(QString text)
{
    // std::cerr << "text change: " << text.toStdString() << std::endl;
    inputText = text;
}

void ActionManager::returnPressed()
{
    VizAction *action = findActionByTitle(inputText.toStdString());
    if (action != nullptr)
    {
        action->perform();
    }
    searchbar->clear();
}

void ActionManager::loadDataset(DataObject *dataSetIn)
{
    dataSet = dataSetIn;

    actionPhrases.push_back(new Phrase("select"));
    actionPhrases.push_back(new Phrase("hide"));

    // create builin axis groups
    for (int i = 0; i < dataSet->getNumberOfAttributes() && i < 19; i++)
    {
        groups.push_back(new Group(i, SampleAxes::SampleAxesNames[i].toStdString()));
        groupPhrases.push_back(new Phrase(SampleAxes::SampleAxesNames[i].toStdString()));
    }

    // create additional axis groups
    for (int i = 19; i < dataSet->getNumberOfAttributes(); i++)
    {
        groups.push_back(new Group(i, dataSet->GetAttributeName(i)));
        groupPhrases.push_back(new Phrase(dataSet->GetAttributeName(i)));
    }

    // L1 cache miss group
    Group *l1MissGroup = new Group(18, "l1 cache miss");
    l1MissGroup->relative = false;
    l1MissGroup->min = 2;
    l1MissGroup->max = pcViz->dataMax(18);
    groups.push_back(l1MissGroup);
    groupPhrases.push_back(new Phrase("l1 cache miss"));

    sortActions();

    QStringList wordlist;
    for (VizAction *action : actions)
    {
        wordlist.push_back(QString::fromStdString(action->title()));
    }
    QStringListModel *model = new QStringListModel(wordlist);
    QCompleter *completer = new QCompleter(wordlist, this);
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    completer->setCaseSensitivity(Qt::CaseInsensitive);

    searchbar->setCompleter(completer);
}