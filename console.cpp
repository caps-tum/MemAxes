#include "console.h"

static QString titleText(
    "---- MemAxes Console ----\n"
);

static QString helpText(
    "Commands : \n"
    "    "
    "    select [--mode={new,append,filter}] <query>\n"
    "    hide <query>\n"
    "    show <query>\n"
    "        \n"
    "    <query> is of the form:\n"
    "           [DIMRANGE dim=vmin:vmax]\n"
    "           [RESOURCE resource=id]\n"
    "    \n"
    "    inspect\n"
    "    \n"
    "Examples : \n"
    "    select DIMRANGE latency=30:40 cpu=4:5\n"
    "    select DIMRANGE dim6=20:200 dim8=40:45\n"
    "    \n"
    "    select RESOURCE socket=1\n"
    "    select RESOURCE cpu=4 cache=L3\n"
    "    select RESOURCE NUMA=2\n"
);

console::console(QWidget *parent) :
    QTextBrowser(parent)
{
    dataSet = NULL;
    sb = this->verticalScrollBar();

    QFont font("Consolas");
    this->setCurrentFont(font);
    this->setReadOnly(true);

    log(titleText);
    helpCommand(NULL);
}

void console::setConsoleInput(QPlainTextEdit *in)
{
    console_input = in;
    console_input->setLayoutDirection(Qt::LeftToRight);
    connect(console_input,SIGNAL(blockCountChanged(int)),this,SLOT(command(int)));
}

void console::setDataSet(DataSetObject *dsobj)
{
    dataSet = dsobj;
}

void console::helpCommand(QStringList *args)
{
    Q_UNUSED(args);
    log(helpText);
}

void console::inspectCommand(QStringList *args)
{
    Q_UNUSED(args);

    // Print out some info about the current selection
    int numSel = dataSet->numSelected();
    int numTot = dataSet->numTotal();
    QVector<qreal> means = dataSet->means();

    log("Selected Samples : ");
    log(QString::number(numSel));
    log("Total Samples : ");
    log(QString::number(numTot));
    log("Average Values : ");
    for(int d=0; d<means.size(); d++)
    {
        log(dataSet->meta[d]+QString(" : ")+QString::number(means.at(d)));
    }
}

void console::selectCommand(QStringList *args)
{
    if(dataSet == NULL)
    {
        log("Unable to select from the void, please load data first");
        return;
    }

    if(args == NULL || args->size() < 3)
    {
        log("Invalid arguments");
        return;
    }

    QUERY_TYPE qt = getQueryType(args->at(1));

    if(qt == QUERY_UNKNOWN)
    {
        log("Invalid arguments");
        return;
    }

    if(qt == QUERY_DIMRANGE)
    {
        struct dimRangeQuery drq = createDimRangeQuery(args);
        dataSet->selectByMultiDimRange(drq.dims,drq.mins,drq.maxes);
        emit selectionChangedSig();
        return;
    }
}

CMD_TYPE console::getCommandType(QString cmd)
{
    cmd = cmd.toLower();
    if(cmd == "help" || cmd == "h")
        return CMD_HELP;
    else if(cmd == "select" || cmd == "sel")
        return CMD_SELECT;
    else if(cmd == "inspect" || cmd == "ins")
        return CMD_INSPECT;
    return CMD_UNKNOWN;
}

QUERY_TYPE console::getQueryType(QString qtype)
{
    qtype = qtype.toLower();
    if(qtype == "dimrange")
        return QUERY_DIMRANGE;
    else if(qtype == "resource")
        return QUERY_RESOURCE;
    return QUERY_UNKNOWN;
}

int console::dimFromString(QString dstr)
{
    return dataSet->meta.indexOf(dstr);
}

dimRange console::createDimRange(QString str)
{
    QStringList eqSplit = str.split("=");
    QStringList rangeStrs = eqSplit[1].split(":");

    struct dimRange dr;
    dr.dim = eqSplit[0].toInt();
    dr.min = rangeStrs[0].toDouble();
    dr.max = rangeStrs[1].toDouble();

    return dr;
}

struct dimRangeQuery console::createDimRangeQuery(QStringList *args)
{
    struct dimRangeQuery drq;

    for(int i=2; i<args->size(); i++)
    {
        struct dimRange dr = createDimRange(args->at(i));
        drq.dims.push_back(dr.dim);
        drq.mins.push_back(dr.min);
        drq.maxes.push_back(dr.max);
    }

    return drq;
}

void console::command(int i)
{
    Q_UNUSED(i);

    QString cmdLine = console_input->toPlainText().simplified();
    if(cmdLine.isEmpty())
        return;

    console_input->clear();
    console_input->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);

    QStringList cmdArgs = cmdLine.split(" ");
    QString cmd = cmdArgs.first();
    QString printCmdLine("$ "+cmdLine);
    log(printCmdLine);

    CMD_TYPE cmdType = getCommandType(cmd);
    switch(cmdType)
    {
    case(CMD_HELP):
        helpCommand(&cmdArgs);
        break;
    case(CMD_SELECT):
        selectCommand(&cmdArgs);
        break;
    case(CMD_INSPECT):
        inspectCommand(&cmdArgs);
        break;
    default:
        log("Command unrecognized, type 'help' or 'h' for a list of commands");
        break;
    }
}

void console::log(const char *msg)
{
    log(QString(msg));
}

void console::log(QString msg)
{
    this->append(msg);

    // Scroll to bottom
    sb->setValue(sb->maximum());
}
