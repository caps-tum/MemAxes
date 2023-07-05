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