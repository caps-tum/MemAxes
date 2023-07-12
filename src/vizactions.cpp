#include "action.h"



bool CorrelateDatasourceInstructionLine::applicable(){
    return true;
}

void CorrelateDatasourceInstructionLine::perform(){
    pcViz->correlateAxes(2, 18);
}



bool CorrelateIBSL2TLBMissInstructionLine::applicable(){
    l2tlbmissIndex = ibsAxisAvailable("ibs_dc_l2_tlb_miss");
    return l2tlbmissIndex >= 0;
}

void CorrelateIBSL2TLBMissInstructionLine::perform(){
    pcViz->correlateAxes(2, 47);
}



bool CorrelateIBSL1TLBMissInstructionLine::applicable(){
    l1tlbmissIndex = ibsAxisAvailable("ibs_dc_l1_tlb_miss");
    return l1tlbmissIndex >= 0;
}

void CorrelateIBSL1TLBMissInstructionLine::perform(){
    pcViz->correlateAxes(2, 46);
}




bool CorrelateL1DCMissInstructionLine::applicable(){
    return true;
}

void CorrelateL1DCMissInstructionLine::perform(){
    pcViz->correlateAxes(2, 18);
    pcViz->selectValRelativeRange(18, .1, 1);
}



bool CorrelateL2DCMissInstructionLine::applicable(){
    return true;
}

void CorrelateL2DCMissInstructionLine::perform(){
    pcViz->correlateAxes(2, 18);
    pcViz->selectValRange(18, 2.5, pcViz->dataMax(18));
}




bool CorrelateL1DCHitInstructionLine::applicable(){
    return true;
}

void CorrelateL1DCHitInstructionLine::perform(){
    pcViz->correlateAxes(2, 18);
    pcViz->selectValRange(18, 1, 1.5);
}



bool CorrelateL2DCHitInstructionLine::applicable(){
    return true;
}

void CorrelateL2DCHitInstructionLine::perform(){
    pcViz->correlateAxes(2, 18);
    pcViz->selectValRange(18, 1.8, 2.2);
}



bool CorrBrnMispSrcLine::applicable(){
    brnMispDataIndex = ibsAxisAvailable("ibs_op_brn_misp");
    return brnMispDataIndex >= 0;
}

void CorrBrnMispSrcLine::perform(){
    pcViz->correlateAxes(2, brnMispDataIndex);
}



bool CorrBrnMispSelect::applicable(){
    brnMispDataIndex = ibsAxisAvailable("ibs_op_brn_misp");
    return brnMispDataIndex >= 0;
}

void CorrBrnMispSelect::perform(){
    pcViz->correlateAxes(2, brnMispDataIndex);
    pcViz->selectValRelativeRange(brnMispDataIndex, .99, 1);
}


bool CorrTagToRetSrc::applicable(){
    tagToRetDataIndex = ibsAxisAvailable("ibs_tag_to_ret_ctr");
    return tagToRetDataIndex >= 0;
}

void CorrTagToRetSrc::perform(){
    pcViz->correlateAxes(2, tagToRetDataIndex);
}


bool CorrTagToRetIns::applicable(){
    tagToRetDataIndex = ibsAxisAvailable("ibs_tag_to_ret_ctr");
    return tagToRetDataIndex >= 0;
}

void CorrTagToRetIns::perform(){
    pcViz->correlateAxes(3, tagToRetDataIndex);
}