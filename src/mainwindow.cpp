//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, Lawrence Livermore National Security, LLC. Produced
// at the Lawrence Livermore National Laboratory. Written by Alfredo
// Gimenez (alfredo.gimenez@gmail.com). LLNL-CODE-663358. All rights
// reserved.
//
// This file is part of MemAxes. For details, see
// https://github.com/scalability-tools/MemAxes
//
// Please also read this link – Our Notice and GNU Lesser General Public
// License. This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License (as
// published by the Free Software Foundation) version 2.1 dated February
// 1999.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms and
// conditions of the GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
// OUR NOTICE AND TERMS AND CONDITIONS OF THE GNU GENERAL PUBLIC LICENSE
// Our Preamble Notice
// A. This notice is required to be provided under our contract with the
// U.S. Department of Energy (DOE). This work was produced at the Lawrence
// Livermore National Laboratory under Contract No. DE-AC52-07NA27344 with
// the DOE.
// B. Neither the United States Government nor Lawrence Livermore National
// Security, LLC nor any of their employees, makes any warranty, express or
// implied, or assumes any liability or responsibility for the accuracy,
// completeness, or usefulness of any information, apparatus, product, or
// process disclosed, or represents that its use would not infringe
// privately-owned rights.
//////////////////////////////////////////////////////////////////////////////

#include "ui_form.h"
#include "mainwindow.h"

#include <unistd.h>
#include <iostream>
#include <sys/stat.h>
using namespace std;
using namespace SampleAxes;

#include <QTimer>
#include <QFileDialog>
#include <QInputDialog>

// NEW FEATURES
// Mem topo 1d memory range
// Multiple selections, selection groups (classification)

// APPLICATIONS
// LibNUMA (move_pages(x,x,NULL,...)

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(tr("MemAxes"));
    ui->menuBar->setNativeMenuBar(true);

    dataSet = new DataObject();

    con = new console(this);
    ui->consoleLayout->addWidget(con);

    QPlainTextEdit *console_input = new QPlainTextEdit();
    console_input->setFont(QFont("Consolas"));
    console_input->setMaximumHeight(28);
    ui->consoleLayout->addWidget(console_input);

    con->setConsoleInput(console_input);
    con->setDataSet(dataSet);

    

    /*
     * MainWindow
    */
    qDebug( "main C Style Debug Message" );

    // File buttons
    connect(ui->actionImport_Data, SIGNAL(triggered()),this,SLOT(loadData()));
    connect(ui->actionImport_Data_IBS, SIGNAL(triggered()),this,SLOT(loadDataIBS()));

    // Selection mode
    connect(ui->selectModeXOR, SIGNAL(toggled(bool)), this, SLOT(setSelectModeXOR(bool)));
    connect(ui->selectModeOR, SIGNAL(toggled(bool)), this, SLOT(setSelectModeOR(bool)));
    connect(ui->selectModeAND, SIGNAL(toggled(bool)), this, SLOT(setSelectModeAND(bool)));

    // Selection buttons
    connect(ui->selectAll, SIGNAL(clicked()), this, SLOT(selectAll()));
    connect(ui->deselectAll, SIGNAL(clicked()), this, SLOT(deselectAll()));
    connect(ui->selectAllVisible, SIGNAL(clicked()), this, SLOT(selectAllVisible()));

    // Add Remove Correlate Histograms
    connect(ui->addHistogram, SIGNAL(clicked()), this, SLOT(addAxis()));
    connect(ui->removeHistogram, SIGNAL(clicked()), this, SLOT(removeAxis()));
    connect(ui->correlateButton, SIGNAL(clicked()), this, SLOT(correlateAxes()));

    // Visibility buttons
    connect(ui->hideSelected, SIGNAL(clicked()), this, SLOT(hideSelected()));
    connect(ui->showSelectedOnly, SIGNAL(clicked()), this, SLOT(showSelectedOnly()));
    connect(ui->showAll, SIGNAL(clicked()), this, SLOT(showAll()));

    /*
     * Code Viz
     */

    codeViz = new CodeViz(this);
    ui->codeVizLayout->addWidget(codeViz);
    connect(ui->jumpingCheckbox, SIGNAL(clicked()), codeViz, SLOT(toggleCodeJumping()));

    //connect(codeViz, SIGNAL(sourceFileSelected(QFile*)), this, SLOT(setCodeLabel(QString)));

    vizWidgets.push_back(codeViz);

    /*
     * Variable Viz
     */

    varViz = new VarViz(this);
    ui->varVizLayout->addWidget(varViz);
    varViz->setStyleSheet("background-color: red;");

    vizWidgets.push_back(varViz);

    /*
     * Code Editor
     */

    codeEditor = new CodeEditor(this);
    codeEditor->setFont(QFont("Consolas"));
    codeEditor->setReadOnly(true);
    codeEditor->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
    ui->codeEditorLayout->addWidget(codeEditor);

    connect(codeViz, SIGNAL(sourceFileSelected(QFile*)), this, SLOT(setCodeLabel(QFile*)));
    connect(codeViz, SIGNAL(sourceFileSelected(QFile*)), codeEditor, SLOT(setFile(QFile*)));
    connect(codeViz, SIGNAL(sourceLineSelected(int)), codeEditor, SLOT(setLine(int)));
    

    /*
     * Memory Topology Viz
     */

    memViz = new HWTopoVizWidget(this);
    ui->memoryLayout->addWidget(memViz);

    connect(ui->memTopoColorByCycles,SIGNAL(toggled(bool)),memViz,SLOT(setColorByCycles(bool)));
    connect(ui->memTopoColorBySamples,SIGNAL(toggled(bool)),memViz,SLOT(setColorBySamples(bool)));
    connect(ui->memTopoVizModeIcicle,SIGNAL(toggled(bool)),memViz,SLOT(setVizModeIcicle(bool)));
    connect(ui->memTopoVizModeSunburst,SIGNAL(toggled(bool)),memViz,SLOT(setVizModeSunburst(bool)));
    connect(ui->blueColor, SIGNAL(stateChanged(int)), memViz, SLOT(blueSwitch()));

    vizWidgets.push_back(memViz);

    /*
     * Parallel Coords Viz
     */

    PCVizWidget *parallelCoordinatesViz = new PCVizWidget(this);
    ui->parallelCoordinatesLayout->addWidget(parallelCoordinatesViz);

    //connect(ui->selOpacity, SIGNAL(valueChanged(int)), parallelCoordinatesViz, SLOT(setSelOpacity(int)));
    //connect(ui->unselOpacity, SIGNAL(valueChanged(int)), parallelCoordinatesViz, SLOT(setUnselOpacity(int)));
    connect(ui->histogramBox, SIGNAL(clicked(bool)), parallelCoordinatesViz, SLOT(setShowHistograms(bool)));
    connect(ui->allBins, SIGNAL(toggled(bool)), parallelCoordinatesViz, SLOT(setLineColoringAllBins()));
    connect(ui->firstAxis, SIGNAL(toggled(bool)), parallelCoordinatesViz, SLOT(setLineColoringFirstAxis()));
    connect(ui->secondAxis, SIGNAL(toggled(bool)), parallelCoordinatesViz, SLOT(setLineColoringSecondAxis()));
    connect(codeViz, SIGNAL(sourceLineHover(int)), parallelCoordinatesViz, SLOT(setFilterLine(int)));
    connect(parallelCoordinatesViz, SIGNAL(lineSelected(int)), codeEditor, SLOT(setLine(int)));
    connect(parallelCoordinatesViz, SIGNAL(selectSourceFileByIndex(int)), codeViz, SLOT(selectFileByIndex(int)));
    connect(parallelCoordinatesViz, SIGNAL(highlightLines(vector<tuple<int, float>>)), codeEditor, SLOT(highlightLines(vector<tuple<int, float>>)));
    connect(ui->numHistBinsSlider, SIGNAL(valueChanged(int)), parallelCoordinatesViz, SLOT(setNumHistBins(int)));
    connect(ui->jumpingCheckbox, SIGNAL(clicked()), parallelCoordinatesViz, SLOT(toggleCodeJumping()));
    connect(ui->SharedMinMax, SIGNAL(clicked()), parallelCoordinatesViz, SLOT(toggleSharedMinMax()));
    connect(memViz, SIGNAL(hoverHardwareTopoSamples(vector<int> *)), parallelCoordinatesViz, SLOT(setHardwareTopologySampleSet(vector<int>*)));
    connect(ui->UnselOpacity, SIGNAL(valueChanged(int)), parallelCoordinatesViz, SLOT(setUnselOpacity(int)));

    vizWidgets.push_back(parallelCoordinatesViz);
    pcViz = parallelCoordinatesViz;

    /*
     * All VizWidgets
     */

    // Set viz widgets to use new data
    for(int i=0; i<vizWidgets.size(); i++)
    {
        vizWidgets[i]->setDataSet(dataSet);
        vizWidgets[i]->setConsole(con);
    }

    dataSet->setConsole(con);

    connect(con, SIGNAL(selectionChangedSig()), this, SLOT(selectionChangedSlot()));

    for(int i=0; i<vizWidgets.size(); i++)
    {
        connect(vizWidgets[i], SIGNAL(selectionChangedSig()), this, SLOT(selectionChangedSlot()));
        connect(this, SIGNAL(selectionChangedSig()), vizWidgets[i], SLOT(selectionChangedSlot()));
        connect(vizWidgets[i], SIGNAL(visibilityChangedSig()), this, SLOT(visibilityChangedSlot()));
        connect(this, SIGNAL(visibilityChangedSig()), vizWidgets[i], SLOT(visibilityChangedSlot()));
    }

    actionManager = new ActionManager(pcViz, ui->searchbar);
    vizWidgets.push_back(actionManager);
    connect(ui->searchbar, SIGNAL(returnPressed()), actionManager, SLOT(returnPressed()));
    connect(ui->searchbar, SIGNAL(textEdited(QString)), actionManager, SLOT(textEdited(QString)));
    connect(ui->searchbar, SIGNAL(textChanged(QString)), actionManager, SLOT(textChanged(QString)));


    frameTimer = new QTimer(this);
    frameTimer->setInterval(1000/60); // 60fps
    connect(frameTimer,SIGNAL(timeout()),this,SLOT(frameUpdateAll()));

    std::cerr << "created all widgets and frame timer\n";

    frameTimer->start();
}

MainWindow::~MainWindow()
{
}

void MainWindow::frameUpdateAll()
{
    for(int i=0; i<vizWidgets.size(); i++)
    {
        vizWidgets[i]->frameUpdate();
    }
}



void MainWindow::selectionChangedSlot()
{
    dataSet->selectionChanged();
    emit selectionChangedSig();
}

void MainWindow::visibilityChangedSlot()
{
    dataSet->visibilityChanged();
    emit visibilityChangedSig();
}

void errdiag(QString str)
{
        QErrorMessage errmsg;
        errmsg.showMessage(str);
        errmsg.exec();
}

bool directoryExists(QString dir){
    struct stat sb;
    return stat(dir.toLocal8Bit().data(), &sb);
}

int MainWindow::loadData()
{
    
    int err = 0;
    err = selectDataDirectory();
    if(err != 0)
        return err;

    QString sourceDir(dataDir+QString("/src/"));
    codeViz->setSourceDir(sourceDir);
    //QString topoDir(dataDir+QString("/hardware.xml"));
    err = dataSet->loadHardwareTopology(dataDir);
    if(err != 0)
    {
        errdiag("Error loading hardware: "+dataDir);
        return err;
    }
    //QString dataSetDir(dataDir+QString("/data/samples.out"));
    QString dataSetDir(dataDir+QString("/data/samples.csv"));
    err = dataSet->loadData(dataSetDir);
    if(err != 0)
    {
        errdiag("Error loading dataset: "+dataSetDir);
        return err;
    }
    for(int i=0; i<vizWidgets.size(); i++)
    {
        vizWidgets[i]->processData();
        vizWidgets[i]->update();
    }
    visibilityChangedSlot();
    return 0;
}



int MainWindow::loadDataIBS()
{

    int err = 0;


    err = selectDirectory(&opDataDir, "op data");
    if(err != 0)
        return err;

    
    
    QString opSourceDir(opDataDir+QString("/src"));
    codeViz->setSourceDir(opSourceDir);
    QString opTopoDir(opDataDir+QString("/hardware.xml"));
    err = dataSet->loadHardwareTopologyIBS(opTopoDir);
    if(err != 0)
    {
        errdiag("Error loading hardware: "+opTopoDir);
        return err;
    }

    //ask for base latency
    int baseLat = 0;
    err = selectInt(&baseLat, "Set assumed latency of L1 cache", "please enter a number >= 0", 0, 1000);
    if(err != 0){
        errdiag("Error setting base latency");
        return err;
    }
    dataSet->setIBSBaseLatency(baseLat);
    
    //QString dataSetDir(dataDir+QString("/data/samples.out"));
    QString dataSetDir(opDataDir+QString("/data/samples.csv"));

    //load the data
    err = dataSet->loadData(dataSetDir);
    if(err != 0)
    {
        errdiag("Error loading dataset: "+dataSetDir);
        return err;
    }

    setHistogramsComboBoxes();

    

    for(int i=0; i<vizWidgets.size(); i++)
    {
        vizWidgets[i]->processData();
        vizWidgets[i]->update();
    }
    visibilityChangedSlot();
    actionManager->loadDataset(dataSet);
    return 0;
}



int MainWindow::selectDataDirectory()
{
    dataDir = QFileDialog::getExistingDirectory(this,
                                                  tr("Select Data Directory"),
                                                  "/Users/chai/Sources/MemAxes/example_data/",
                                                  QFileDialog::ShowDirsOnly
                                                  | QFileDialog::DontResolveSymlinks);
    if(dataDir.isNull())
        return -1;

    con->append("Selected Data Directory : "+dataDir);

    return 0;
}

int MainWindow::selectDirectory(QString *dest, QString directory_name){
    *dest = QFileDialog::getExistingDirectory(this,
                                                  tr(("Select " + directory_name + " directory").toLocal8Bit().data()),
                                                  "/Users/chai/Sources/MemAxes/example_data/",
                                                  QFileDialog::ShowDirsOnly
                                                  | QFileDialog::DontResolveSymlinks);
    if((*dest).isNull())
        return -1;

    con->append("Selected " + directory_name + " Directory : " + dest);

    return 0;
}

void MainWindow::setHistogramsComboBoxes(){
    //MemAxes builtin axes
    for(int i = 0; i < 19; i++){
        ui->axisComboBox1->addItem(QString("%1").arg(i, 2, 10, QLatin1Char('0')) + ": " + SampleAxesNames.at(i));
        ui->axisComboBox2->addItem(QString("%1").arg(i, 2, 10, QLatin1Char('0')) + ": " + SampleAxesNames.at(i));
    }
    //IBS additional axes
    for(int i = 19; i < dataSet->numberOfColumns(); i++){
        ui->axisComboBox1->addItem(QString("%1").arg(i, 2, 10, QLatin1Char('0')) + ": " + dataSet->titleOfColumn(i));
        ui->axisComboBox2->addItem(QString("%1").arg(i, 2, 10, QLatin1Char('0')) + ": " + dataSet->titleOfColumn(i));
    }

}

int MainWindow::selectInt(int *dest, QString wName, QString prompt, int rangeLow, int rangeHigh){
    bool success = false;
    while(!success){
        *dest = QInputDialog::getInt(this, wName, prompt, rangeLow, rangeLow, rangeHigh, 1, &success);
    }
    
    return 0;
}



void MainWindow::showSelectedOnly()
{
    dataSet->hideUnselected();
    visibilityChangedSlot();
}

void MainWindow::selectAllVisible()
{
    dataSet->selectAllVisible();
    selectionChangedSlot();
}

void MainWindow::selectAll()
{
    dataSet->selectAll();
    pcViz->resetSelection();
    selectionChangedSlot();
}

void MainWindow::deselectAll()
{
    dataSet->deselectAll();
    selectionChangedSlot();
}

void MainWindow::addAxis(){
    int err = pcViz->addAxis(ui->axisComboBox1->currentIndex());
    if(err == -1){
        errdiag("Histogram for " + (ui->axisComboBox1->itemData(ui->axisComboBox1->currentIndex())).toString() + " already present");
    }
}

void MainWindow::removeAxis(){
    int err = pcViz->removeAxis(ui->axisComboBox1->currentIndex());
    
    if(err == -1){
        errdiag("Histogram for " + (ui->axisComboBox1->itemData(ui->axisComboBox1->currentIndex())).toString() + " can not be deleted: Histogram not present");
    }
}

void MainWindow::correlateAxes(){
    int err = pcViz->correlateAxes(ui->axisComboBox1->currentIndex(), ui->axisComboBox2->currentIndex());

    if(err == -1){
        errdiag("Correlating an Axis with itself is not allowed");
    }
}

void MainWindow::showAll()
{
    dataSet->showAll();
    visibilityChangedSlot();
}

void MainWindow::hideSelected()
{
    dataSet->hideSelected();
    visibilityChangedSlot();
}

void MainWindow::setSelectModeAND(bool on)
{
    if(on)
        dataSet->setSelectionMode(MODE_FILTER);
}

void MainWindow::setSelectModeOR(bool on)
{
    if(on)
        dataSet->setSelectionMode(MODE_APPEND);
}

void MainWindow::setSelectModeXOR(bool on)
{
    if(on)
        dataSet->setSelectionMode(MODE_NEW);
}

void MainWindow::setCodeLabel(QFile *file)
{
    if(file != nullptr){
        ui->codeLabel->setText(file->fileName());
    }else{
        ui->codeLabel->setText("FILE NOT FOUND");
    }
}

void MainWindow::testSlot(int i){
    std::cerr << "test slot triggered " << i << std::endl;
}
