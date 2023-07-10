#include "action.h"

CorrelateDatasourceInstructionLine::CorrelateDatasourceInstructionLine(PCVizWidget * pcVizIn, DataObject* dataSetIn){
    pcViz = pcVizIn;
    dataSet = dataSetIn;
}

bool CorrelateDatasourceInstructionLine::applicable(){
    return true;
}

void CorrelateDatasourceInstructionLine::perform(){
    pcViz->correlateAxes(2, 18);
}



CorrelateIBSL2TLBMissInstructionLine::CorrelateIBSL2TLBMissInstructionLine(PCVizWidget * pcVizIn, DataObject* dataSetIn){
    pcViz = pcVizIn;
    dataSet = dataSetIn;
}

bool CorrelateIBSL2TLBMissInstructionLine::applicable(){
    l2tlbmissIndex = ibsAxisAvailable("ibs_dc_l2_tlb_miss");
    return l2tlbmissIndex >= 0;
}

void CorrelateIBSL2TLBMissInstructionLine::perform(){
    pcViz->correlateAxes(2, 47);
}



CorrelateIBSL1TLBMissInstructionLine::CorrelateIBSL1TLBMissInstructionLine(PCVizWidget * pcVizIn, DataObject* dataSetIn){
    pcViz = pcVizIn;
    dataSet = dataSetIn;
}

bool CorrelateIBSL1TLBMissInstructionLine::applicable(){
    l1tlbmissIndex = ibsAxisAvailable("ibs_dc_l1_tlb_miss");
    return l1tlbmissIndex >= 0;
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

void CorrelateL1DCMissInstructionLine::perform(){
    pcViz->correlateAxes(2, 18);
    pcViz->selectValRelativeRange(18, .1, 1);
}