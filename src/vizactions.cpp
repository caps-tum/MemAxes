#include "action.h"

CorrelateDatasourceInstructionLine::CorrelateDatasourceInstructionLine(PCVizWidget * pcVizIn, DataObject* dataSetIn){
    pcViz = pcVizIn;
    dataSet = dataSetIn;
}

bool CorrelateDatasourceInstructionLine::applicable(){
    return true;
}

float CorrelateDatasourceInstructionLine::heuristic(){
    return 0;
}

void CorrelateDatasourceInstructionLine::perform(){
    pcViz->correlateAxes(2, 18);
}



CorrelateIBSL2TLBMissInstructionLine::CorrelateIBSL2TLBMissInstructionLine(PCVizWidget * pcVizIn, DataObject* dataSetIn){
    pcViz = pcVizIn;
    dataSet = dataSetIn;
}

bool CorrelateIBSL2TLBMissInstructionLine::applicable(){
    return true;
}

float CorrelateIBSL2TLBMissInstructionLine::heuristic(){
    return 1;
}

void CorrelateIBSL2TLBMissInstructionLine::perform(){
    pcViz->correlateAxes(2, 47);
}



CorrelateIBSL1TLBMissInstructionLine::CorrelateIBSL1TLBMissInstructionLine(PCVizWidget * pcVizIn, DataObject* dataSetIn){
    pcViz = pcVizIn;
    dataSet = dataSetIn;
}

bool CorrelateIBSL1TLBMissInstructionLine::applicable(){
    return true;
}

float CorrelateIBSL1TLBMissInstructionLine::heuristic(){
    return .8;
}

void CorrelateIBSL1TLBMissInstructionLine::perform(){
    pcViz->correlateAxes(2, 46);
}



CorrelateL1DCMissInstructionLine::CorrelateL1DCMissInstructionLine(PCVizWidget * pcVizIn, DataObject* dataSetIn){
    pcViz = pcVizIn;
    dataSet = dataSetIn;
}

bool CorrelateL1DCMissInstructionLine::applicable(){
    return true;
}

float CorrelateL1DCMissInstructionLine::heuristic(){
    return 1;
}

void CorrelateL1DCMissInstructionLine::perform(){
    pcViz->correlateAxes(2, 18);
    pcViz->selectValRelativeRange(18, .1, 1);
}