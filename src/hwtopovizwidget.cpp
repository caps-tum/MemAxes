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
    dataMode = COLORBY_CYCLES;
    vizMode = SUNBURST;

    colorMap = gradientColorMap(QColor(255,237,160),
                                QColor(240,59 ,32 ),
                                256);

    this->installEventFilter(this);
    setMouseTracking(true);
}

void HWTopoVizWidget::frameUpdate()
{
    QRectF drawBox = this->rect();
    drawBox.adjust(margin,margin,-margin,-margin);

    if(needsCalcMinMaxes)
    {
        calcMinMaxes();
        needsCalcMinMaxes = false;
    }
    if(needsConstructNodeBoxes)
    {
        constructNodeBoxes(drawBox,
                           dataSet->node,
                           depthValRanges,
                           depthTransRanges,
                           dataMode,
                           nodeBoxes,linkBoxes);
        needsConstructNodeBoxes = false;
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

    //TODO at the moment for one CPU
    Chip* cpu = (Chip*)dataSet->node->GetChild(1);
    int maxTopoDepth = cpu->GetTopoTreeDepth();

    depthRange = IntRange(0,maxTopoDepth);
    for(int i=depthRange.first; i<(int)depthRange.second; i++)
    {
        vector<Component*> componentsAtDepth;
        cpu->GetComponentsNLevelsDeeper(&componentsAtDepth, i);
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

void HWTopoVizWidget::drawTopo(QPainter *painter, QRectF rect, ColorMap &cm, QVector<NodeBox> &nb, QVector<LinkBox> &lb)
{
    // Draw node outlines
    painter->setPen(QPen(Qt::black));
    for(int b=0; b<nb.size(); b++)
    {
        Component *c = nb.at(b).component;
        QRectF box = nb.at(b).box;
        QString text = QString::number(c->GetId());

        // Color by value
        QColor col = valToColor(nb.at(b).val,cm);
        painter->setBrush(col);

        // Draw rect (radial or regular)
        if(vizMode == SUNBURST)
        {
            QVector<QPointF> segmentPoly = rectToRadialSegment(box,rect);
            painter->drawPolygon(segmentPoly.constData(),segmentPoly.size());
        }
        else if(vizMode == ICICLE)
        {
            painter->drawRect(box);
            QPointF center = box.center() - QPointF(4,-4);
            painter->drawText(center,text);
        }
    }

    // Draw links
    painter->setBrush(Qt::black);
    painter->setPen(Qt::NoPen);
    for(int b=0; b<lb.size(); b++)
    {
        QRectF box = lb[b].box;

        if(vizMode == SUNBURST)
        {
            QVector<QPointF> segmentPoly = rectToRadialSegment(box,rect);
            painter->drawPolygon(segmentPoly.constData(),segmentPoly.size());
        }
        else if(vizMode == ICICLE)
        {
            painter->drawRect(box);
        }
    }
}

void HWTopoVizWidget::drawQtPainter(QPainter *painter)
{
    if(!processed)
        return;

    QRectF drawBox = this->rect();
    drawBox.adjust(margin,margin,-margin,-margin);

    drawTopo(painter,drawBox,colorMap,nodeBoxes,linkBoxes);

}

void HWTopoVizWidget::mousePressEvent(QMouseEvent *e)
{
    if(!processed)
        return;

    Component *c = nodeAtPosition(e->pos());

    if(c)
    {
        selectSamplesWithinNode(c);
    }

}

void HWTopoVizWidget::mouseMoveEvent(QMouseEvent* e)
{
    if(!processed)
        return;

    Component *c = nodeAtPosition(e->pos());

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

    needsConstructNodeBoxes = true;
    frameUpdate();
}

void HWTopoVizWidget::calcMinMaxes()
{
    RealRange limits;
    limits.first = 99999999;
    limits.second = 0;

    depthValRanges.resize(depthRange.second - depthRange.first);
    depthValRanges.fill(limits);

    depthTransRanges.resize(depthRange.second - depthRange.first);
    depthTransRanges.fill(limits);

    Chip* cpu = (Chip*)dataSet->node->GetChild(1);
    for(int r=0, i=depthRange.first; i<depthRange.second; r++, i++)
    {
        vector<Component*> componentsAtDepth;
        cpu->GetComponentsNLevelsDeeper(&componentsAtDepth, i);
        // Get min/max for this row
        for(int j=widthRange[r].first; j<widthRange[r].second; j++)
        {
            Component * c = componentsAtDepth[j];
            // QMap<DataObject*,SampleSet>*sampleSets = (QMap<DataObject*,SampleSet>*)c->attrib["sampleSets"];
            // if(!sampleSets->contains(dataSet) )
            //     continue;
            // ElemSet &samples = (*sampleSets)[dataSet].selSamples;
            // int numCycles = (*sampleSets)[dataSet].selCycles;

            ElemSet samples;
            int numCycles = 0;
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
                samples.insert(ss->selSamples.begin(),ss->selSamples.end());
                numCycles += ss->selCycles;
            }


            qreal val = (dataMode == COLORBY_CYCLES) ? numCycles : samples.size();
            //val = (qreal)(*numCycles) / (qreal)samples->size();

            depthValRanges[i].first=0;//min(depthValRanges[i].first,val);
            depthValRanges[i].second=max(depthValRanges[i].second,val);

            qreal trans = *(int*)(c->attrib["transactions"]);
            depthTransRanges[i].first=0;//min(depthTransRanges[i].first,trans);
            depthTransRanges[i].second=max(depthTransRanges[i].second,trans);
        }
    }

    needsConstructNodeBoxes = true;
    needsRepaint = true;
}

void HWTopoVizWidget::constructNodeBoxes(QRectF rect,
                                    Node *node,
                                    QVector<RealRange> &valRanges,
                                    QVector<RealRange> &transRanges,
                                    DataMode m,
                                    QVector<NodeBox> &nbout,
                                    QVector<LinkBox> &lbout)
{
    nbout.clear();
    lbout.clear();

    if(node == NULL)
        return;

    float nodeMarginX = 2.0f;
    float nodeMarginY = 10.0f;

    //TODO at the moment for one CPU
    Chip* cpu = (Chip*)node->GetChild(1);
    int maxTopoDepth = cpu->GetTopoTreeDepth();

    float deltaX = 0;
    float deltaY = rect.height() / maxTopoDepth;

    // Adjust boxes to fill the rect space
    for(int i=0; i<maxTopoDepth; i++)
    {
        vector<Component*> componentsAtDepth;
        cpu->GetComponentsNLevelsDeeper(&componentsAtDepth, i);
        deltaX = rect.width() / (float)componentsAtDepth.size();
        for(int j=0; j<componentsAtDepth.size(); j++)
        {
            // Create Node Box
            NodeBox nb;
            nb.component = componentsAtDepth[j];
            nb.box.setRect(rect.left()+j*deltaX,
                           rect.top()+i*deltaY,
                           deltaX,
                           deltaY);

            // Get value by cycles or samples
            int numCycles = 0;
            int numSamples = 0;
            // QMap<DataObject*,SampleSet>* sampleSets = (QMap<DataObject*,SampleSet>*)nb.component->attrib["sampleSets"];
            // numSamples += (*sampleSets)[dataSet].selSamples.size();
            // numCycles += (*sampleSets)[dataSet].selCycles;
            int direction;
            if(nb.component->GetComponentType() == SYS_SAGE_COMPONENT_THREAD){
                direction = SYS_SAGE_DATAPATH_INCOMING;
            }else {
                direction = SYS_SAGE_DATAPATH_OUTGOING;
            }
            vector<DataPath*> dp_vec;
            nb.component->GetAllDpByType(&dp_vec, SYS_SAGE_MITOS_SAMPLE, direction);
            for(DataPath* dp : dp_vec) {
                SampleSet *ss = (SampleSet*)dp->attrib["sample_set"];
                numSamples += ss->selSamples.size();
                numCycles += ss->selCycles;
            }

            qreal unscaledval = (m == COLORBY_CYCLES) ? numCycles : numSamples;
            nb.val = scale(unscaledval,
                           valRanges.at(i).first,
                           valRanges.at(i).second,
                           0, 1);

            if(i==0)
                nb.box.adjust(0,0,0,-nodeMarginY);
            else
                nb.box.adjust(nodeMarginX,nodeMarginY,-nodeMarginX,-nodeMarginY);

            nbout.push_back(nb);

            // Create Link Box
            if(i-1 >= 0)
            {
                LinkBox lb;
                lb.parent = nb.component->GetParent();
                lb.child = nb.component;

                // scale width by transactions
                lb.box.setRect(rect.left()+j*deltaX,
                               rect.top()+i*deltaY,
                               deltaX,
                               nodeMarginY);

                lb.box.adjust(nodeMarginX,-nodeMarginY,-nodeMarginX,0);

                float linkWidth = scale(*(int*)(nb.component->attrib["transactions"]),
                                        transRanges.at(i).first,
                                        transRanges.at(i).second,
                                        1.0f,
                                        lb.box.width());
                float deltaWidth = (lb.box.width()-linkWidth)/2.0f;

                lb.box.adjust(deltaWidth,0,-deltaWidth,0);

                lbout.push_back(lb);
            }
        }
    }

    needsRepaint = true;
}

Component *HWTopoVizWidget::nodeAtPosition(QPoint p)
{
    QRectF drawBox = this->rect();
    drawBox.adjust(margin,margin,-margin,-margin);

    for(int b=0; b<nodeBoxes.size(); b++)
    {
        Component *c = nodeBoxes[b].component;
        QRectF box = nodeBoxes[b].box;

        bool containsP = false;
        if(vizMode == SUNBURST)
        {
            QPointF radp = reverseRadialTransform(p,drawBox);
            containsP = box.contains(radp);
        }
        else if(vizMode == ICICLE)
        {
            containsP = box.contains(p);
        }

        if(containsP)
            return c;
    }

    return NULL;
}

void HWTopoVizWidget::selectSamplesWithinNode(Component *c)
{
    dataSet->selectByResource(c);
    emit selectionChangedSig();
}
