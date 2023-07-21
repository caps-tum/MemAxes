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

void orderIndicesByMatch(vector<Phrase *> phrases, string query, vector<int> *result)
{
    std::sort(result->begin(), result->end(), [phrases, query](int a, int b) -> int
              { return phrases[a]->matchingLevenshtein(query) < phrases[b]->matchingLevenshtein(query); });
}

void ActionManager::sortActions()
{
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

void ActionManager::generateActions()
{

    while (!actions.empty())
    {
        VizAction *a = actions[actions.size() - 1];
        actions.pop_back();
        delete a;
    }

    string userInput = userInputText.toStdString();

    QStringList split = QString::fromStdString(userInput).split(' ');

    vector<int> numbers;
    vector<int> numberPosition;
    vector<string> sentences;

    string acc = "";
    int cnt = 0;

    float maxActionDistance = 1;
    int actionIdentified = -1;
    int actionWordIndex = -1;

    int mostLikelyAction = -1;
    float smallestDistance = 1;

    for (int i = 0; i < split.size(); i++)
    {
        QString s = split[i];
        vector<float> actionMatches;
        phraseMatch(actionPhrases, s.toStdString(), &actionMatches);

        auto minimumMatchPointer = std::min_element(actionMatches.begin(), actionMatches.end());
        int minIndex = std::distance(actionMatches.begin(), minimumMatchPointer);

        if (actionMatches[minIndex] < smallestDistance)
        {
            mostLikelyAction = minIndex;
            smallestDistance = actionMatches[minIndex];
        }

        if (actionMatches[minIndex] <= .2)
        {
            if (actionMatches[minIndex] < maxActionDistance)
            {
                maxActionDistance = actionMatches[minIndex];
                actionIdentified = minIndex;
                actionWordIndex = i;
            }
        }
    }

    for (int i = 0; i < split.size(); i++)
    {
        QString s = split[i];
        bool isInt = true;
        s.toInt(&isInt);
        if (isInt)
        {
            numberPosition.push_back(sentences.size());
            if (!acc.empty())
                sentences.push_back(acc);
            acc = "";
            numbers.push_back(s.toInt());
        }
        else
        {
            if (s.toStdString() == "with")
            {
                if (!acc.empty())
                    sentences.push_back(acc);
                acc = "";
            }
            else
            {
                string separator = " ";
                if (acc.empty())
                    separator = "";
                if (i != actionWordIndex)
                {
                    acc = acc + separator + s.toStdString();
                }
            }
        }
    }

    if (acc != "")
        sentences.push_back(acc);
    // debug output of input split
    if (DEBUG_OUTPUT)
    {
        if (actionWordIndex < 0)
            std::cerr << "no action selected - ";
        else
            std::cerr << "action " << actionIdentified << ": \"" << actionPhrases[actionIdentified]->phrase() << "\" - \"";

        for (string s : sentences)
            std::cerr << s << "\" - \"";

        std::cerr << std::endl;
    }

    vector<float> groupCompletionFirstSentence;
    if (!sentences.empty())
        phraseMatch(groupPhrases, sentences[0], &groupCompletionFirstSentence);

    // int highestCompletionGroup = std::distance(groupCompletion.begin(), std::max_element(groupCompletion.begin(), groupCompletion.end()));
    vector<int> firstSentenceOrder;
    vector<int> secondSentenceOrder;

    if (actionIdentified < 0)
    {
        switch (sentences.size())
        {
        case 0:
        {
        }
        break;
        case 1:
        {
        }
        break;
        default:
        {
        }
        break;
        }
    }

    switch (actionIdentified)
    {
    case 0:
    {
        // Select action
        for (int i = 0; i < groups.size(); i++)
        {
            SelectAction *sAction = new SelectAction(pcViz, dataSet, groups[i]);

            if (numbers.size() >= 1)
            {
                sAction->specifyG1Min(numbers[0]);
            }

            if (numbers.size() >= 2)
            {
                sAction->specifyG1Max(numbers[1]);
            }

            if (sentences.size() >= 1)
                sAction->heuristic = groupPhrases[i]->matchingLevenshtein(sentences[0]);
            actions.push_back(sAction);
        }
    }
    break;
    case 1:
    {
        // hide action
        for (int i = 0; i < groups.size(); i++)
        {
            if (pcViz->hasAxis(groups[i]->dataIndex) && !groups[i]->customLimits())
            {
                actions.push_back(new HideAction(pcViz, dataSet, groups[i]));
                if (!sentences.empty())
                    actions[actions.size() - 1]->heuristic = groupPhrases[i]->matchingLevenshtein(sentences[0]);
            }
        }
    }
    break;
    case 2:
    {
        // correlate action
        for (int i = 0; i < groupPhrases.size(); i++)
        {
            firstSentenceOrder.push_back(i);
            secondSentenceOrder.push_back(i);
        }

        if (sentences.size() >= 1)
            orderIndicesByMatch(groupPhrases, sentences[0], &firstSentenceOrder);
        if (sentences.size() >= 2)
            orderIndicesByMatch(groupPhrases, sentences[1], &secondSentenceOrder);

        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                CorrelateAction *nAction = new CorrelateAction(pcViz, dataSet, groups[firstSentenceOrder[i]], groups[secondSentenceOrder[j]]);
                actions.push_back(nAction);
                float h1 = 1;
                float h2 = 1;
                if (sentences.size() >= 1)
                    h1 = groupPhrases[firstSentenceOrder[i]]->matchingLevenshtein(sentences[0]);
                if (sentences.size() >= 2)
                    h2 = groupPhrases[secondSentenceOrder[j]]->matchingLevenshtein(sentences[1]);
                nAction->heuristic = h1 + h2;

                bool minimumOne = false;
                bool minimumTwo = false;

                for (int k = 0; k < numbers.size(); k++)
                {
                    if (numberPosition[k] == 0)
                    {
                        if (minimumOne)
                        {
                            nAction->specifyG1Max(numbers[k]);
                        }
                        else
                        {
                            nAction->specifyG1Min(numbers[k]);
                            minimumOne = true;
                        }
                    }
                    else
                    {
                        if (minimumTwo)
                        {
                            nAction->specifyG2Max(numbers[k]);
                        }
                        else
                        {
                            minimumTwo = true;
                            nAction->specifyG2Min(numbers[k]);
                        }
                    }
                }
            }
        }
    }
    break;
    case 3:
    {
        // help action
        HelpAction *help = new HelpAction(pcViz, dataSet);
        actions.push_back(help);
        break;
    }
    case 4:
    {
        // show action
        for (int i = 0; i < groups.size(); i++)
        {
            if (!groups[i]->customLimits())
            {
                actions.push_back(new ShowAction(pcViz, dataSet, groups[i]));
                if (!sentences.empty())
                    actions[actions.size() - 1]->heuristic = groupPhrases[i]->matchingLevenshtein(sentences[0]);
            }
        }
    }
    break;
    default:
    {
        for (int i = 0; i < groups.size(); i++)
        {
            actions.push_back(new SelectAction(pcViz, dataSet, groups[i]));
            if (!sentences.empty())
                actions[actions.size() - 1]->heuristic = groupPhrases[i]->matchingLevenshtein(sentences[0]);

            if (pcViz->hasAxis(groups[i]->dataIndex) && !groups[i]->customLimits())
            {
                actions.push_back(new HideAction(pcViz, dataSet, groups[i]));
                if (!sentences.empty())
                    actions[actions.size() - 1]->heuristic = groupPhrases[i]->matchingLevenshtein(sentences[0]);
            }
        }
    }
    break;
    }
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
    generateActions();
    sortActions();
}

void ActionManager::textChanged(QString text)
{
    // std::cerr << "text change: " << text.toStdString() << std::endl;
    inputText = text;
}

void ActionManager::returnPressed()
{

    if (!actions.empty())
    {
        actions[0]->perform();
    }
    searchbar->clear();
}

void ActionManager::completerActivated(QString text)
{
    VizAction *action = findActionByTitle(text.toStdString());
    if (action != nullptr)
    {
        action->perform();
    }
    searchbar->clear();
    QStringList list;
    QStringListModel *model = new QStringListModel(list);
    searchbar->completer()->setModel(model);
}

int ActionManager::findAxis(string axisName)
{
    for (int i = 0; i < dataSet->getNumberOfAttributes(); i++)
    {
        if (dataSet->GetAttributeName(i) == axisName)
            return i;
    }
    return -1;
}

void ActionManager::loadDataset(DataObject *dataSetIn)
{
    dataSet = dataSetIn;

    actionPhrases.push_back(new Phrase("select"));
    actionPhrases.push_back(new Phrase("hide"));
    actionPhrases.push_back(new Phrase("correlate"));
    actionPhrases.push_back(new Phrase("help"));
    actionPhrases.push_back(new Phrase("show"));

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

    // L2 cache miss group
    Group *l2MissGrooup = new Group(18, "l2 cache miss");
    l2MissGrooup->relative = false;
    l2MissGrooup->min = 3;
    l2MissGrooup->max = pcViz->dataMax(18);
    groups.push_back(l2MissGrooup);
    groupPhrases.push_back(new Phrase("l2 cache miss"));

    // L1 data TLB miss
    int l1DataTLBMissAxis = findAxis("ibs_dc_l1_tlb_miss");
    if(l1DataTLBMissAxis >= 0){
        Group *l1TlbMissGroup = new Group(l1DataTLBMissAxis, "l1 tlb miss");
        l1TlbMissGroup->min = 1;
        groups.push_back(l1TlbMissGroup);
        groupPhrases.push_back(new Phrase("l1 tlb miss"));
    }


    QStringList wordlist;
    for (VizAction *action : actions)
    {
        wordlist.push_back(QString::fromStdString(action->title()));
    }
    QStringListModel *model = new QStringListModel(wordlist);
    QCompleter *completer = new QCompleter(wordlist, this);
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    completer->setCaseSensitivity(Qt::CaseInsensitive);

    connect(completer, SIGNAL(activated(QString)), this, SLOT(completerActivated(QString)));

    searchbar->setCompleter(completer);
}