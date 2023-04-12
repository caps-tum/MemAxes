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

#include "hwtopovizwidget.h"

#include <iostream>
#include <cmath>

using namespace std;

HWTopoVizWidget::HWTopoVizWidget(QWidget *parent) :
    VizWidget(parent)
{
    hwPainter = new HWTopoPainter();
    hwPainter->setDataMode(COLORBY_CYCLES);
    hwPainter->setVizMode(SUNBURST);


    this->installEventFilter(this);
    setMouseTracking(true);
}

void HWTopoVizWidget::frameUpdate()
{
    QRectF drawBox = this->rect();
    drawBox.adjust(margin,margin,-margin,-margin);

    if(needsCalcMinMaxes)
    {
        hwPainter->calcMinMaxes();
        hwPainter->resize(drawBox);
        needsRepaint = true;
        needsCalcMinMaxes = false;
    }
    if(needsRepaint)
    {
        repaint();
        needsRepaint = false;
    }
}

void HWTopoVizWidget::processData()
{
    processed = false;

    if(dataSet->node == NULL)
        return;

    hwPainter->setTopo(dataSet->getTopo());
    //TODO at the moment for one CPU
    Node* node = (Node*)dataSet->node;
    //Chip* cpu = (Chip*)dataSet->node->GetChild(1);
    int maxTopoDepth = node->GetTopoTreeDepth()+1;
    qDebug("Node Topology Depth: %d ", maxTopoDepth);

    depthRange = IntRange(0,maxTopoDepth);
    for(int i=depthRange.first; i<(int)depthRange.second; i++)
    {
        vector<Component*> componentsAtDepth;
        node->GetComponentsNLevelsDeeper(&componentsAtDepth, i);
        IntRange wr(0,componentsAtDepth.size());
        widthRange.push_back(wr);
    }

    processed = true;
    needsCalcMinMaxes = true;
}

void HWTopoVizWidget::selectionChangedSlot()
{
    if(!processed)
        return;

    needsCalcMinMaxes = true;
}

void HWTopoVizWidget::visibilityChangedSlot()
{
    if(!processed)
        return;

    needsCalcMinMaxes = true;
}

void HWTopoVizWidget::drawQtPainter(QPainter *painter)
{
    if(!processed)
        return;

    hwPainter->draw(painter);

}

void HWTopoVizWidget::mousePressEvent(QMouseEvent *e)
{
    if(!processed)
        return;

    Component *c = hwPainter->nodeAtPosition(e->pos());

    if(c)
    {
        selectSamplesWithinNode(c);
        needsCalcMinMaxes = true;
    }
}

void HWTopoVizWidget::mouseMoveEvent(QMouseEvent* e)
{
    if(!processed)
        return;

    Component *c = hwPainter->nodeAtPosition(e->pos());

    if(c)
    {
        QString label = QString::fromStdString(c->GetName());
        if(c->GetComponentType() == SYS_SAGE_COMPONENT_CACHE)
            label += "L" + QString::number(((Cache*)c)->GetCacheLevel());
        label += " (" + QString::number(c->GetId()) + ") \n";

        label += "\n";
        if(c->GetComponentType() == SYS_SAGE_COMPONENT_CACHE)
            label += "Size: " + QString::number(((Cache*)c)->GetCacheSize()) + " bytes\n";

        label += "\n";

        int numCycles = 0;
        int numSamples = 0;
        //QMap<DataObject*,SampleSet>*sampleSets = (QMap<DataObject*,SampleSet>*)c->attrib["sampleSets"];
        // numSamples += (*sampleSets)[dataSet].selSamples.size();
        // numCycles += (*sampleSets)[dataSet].selCycles;

        int direction;
        if(c->GetComponentType() == SYS_SAGE_COMPONENT_THREAD)
        {
            direction = SYS_SAGE_DATAPATH_INCOMING;
        }else {
            direction = SYS_SAGE_DATAPATH_OUTGOING;
        }
        vector<DataPath*> dp_vec;
        c->GetAllDpByType(&dp_vec, SYS_SAGE_MITOS_SAMPLE, direction);
        for(DataPath* dp : dp_vec) {
            SampleSet *ss = (SampleSet*)dp->attrib["sample_set"];
            numSamples += ss->selSamples.size();
            numCycles += ss->selCycles;
        }

        label += "Samples: " + QString::number(numSamples) + "\n";
        label += "Cycles: " + QString::number(numCycles) + "\n";

        label += "\n";
        label += "Cycles/Access: " + QString::number((float)numCycles / (float)numSamples) + "\n";

        QToolTip::showText(e->globalPos(),label,this, rect() );
    }
    else
    {
        QToolTip::hideText();
    }
}

void HWTopoVizWidget::resizeEvent(QResizeEvent *e)
{
    VizWidget::resizeEvent(e);

    if(!processed)
        return;

    QRectF drawBox = this->rect();
    drawBox.adjust(margin,margin,-margin,-margin);

    hwPainter->resize(drawBox);

    needsRepaint = true;

    frameUpdate();
}


void HWTopoVizWidget::selectSamplesWithinNode(HWNode *node)
{
    ElemSet es = dataSet->createResourceQuery(node);
    dataSet->selectSet(es);
    emit selectionChangedSig();
}
