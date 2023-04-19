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

#include "hwtopodrawable.h"

#include <QPainter>

HWTopoPainter::HWTopoPainter(Node *r)
{
    root = r;
    rect = QRectF(0,0,10,10);

    dataMode = COLORBY_CYCLES;
    vizMode = SUNBURST;
    colorMap = gradientColorMap(QColor(255,237,160),QColor(240,59 ,32), 256);
}

HWTopoPainter::~HWTopoPainter()
{

}

void HWTopoPainter::calcMinMaxes()
{
    if(root == NULL)
        return;

       RealRange limits;
    limits.first = 99999999;
    limits.second = 0;

    Node* node = root;

    depthValRanges.resize(node->GetTopoTreeDepth()+1);
    depthValRanges.fill(limits);

    depthTransRanges.resize(node->GetTopoTreeDepth()+1);
    depthTransRanges.fill(limits);

    for(int r=0, i=0; i<node->GetTopoTreeDepth()+1; r++, i++)
    {
        vector<Component*> componentsAtDepth;
        node->GetComponentsNLevelsDeeper(&componentsAtDepth, i);
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
            depthValRanges[i].second=max(node->GetTopoTreeDepth()+1,val);

            qreal trans = *(int*)(c->attrib["transactions"]);
            depthTransRanges[i].first=0;//min(depthTransRanges[i].first,trans);
            depthTransRanges[i].second=max(node->GetTopoTreeDepth()+1,trans);
        }
    }

    needsConstructNodeBoxes = true;
    needsRepaint = true;
}

void HWTopoPainter::resize(QRectF r)
{
    if(root == NULL)
        return;

    rect = r;

    nodeBoxes.clear();
    linkBoxes.clear();

    float nodeMarginX = rect.width() / 140.0f;
    float nodeMarginY = rect.height() / 40.0f;

    int maxTopoDepth = node->GetTopoTreeDepth()+1;
    float deltaX = 0;
    //float deltaY = rect.height() / topo->hardwareResourceMatrix.size();
    float deltaY = rect.height() / maxTopoDepth;
    
    // Adjust boxes to fill the rect space
    for(int i=0; i<maxTopoDepth; i++)
    {
        vector<Component*> componentsAtDepth;
        node->GetComponentsNLevelsDeeper(&componentsAtDepth, i);
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
                           depthValRanges.at(i).first,
                           depthValRanges.at(i).second,
                           0, 1);

            if(i==0)
                nb.box.adjust(0,0,0,-nodeMarginY);
            else
                nb.box.adjust(nodeMarginX,nodeMarginY,-nodeMarginX,-nodeMarginY);

            nbout.push_back(nb);

            // Create Link Box
            if(i > 0)
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
}


void HWTopoPainter::draw(QPainter *painter)
{
    if(topo == NULL)
        return;

    // Draw background
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(248,248,255));
    painter->drawEllipse(rect);

    // Draw node outlines
    painter->setPen(QPen(Qt::black));
    for(int b=0; b<nodeBoxes.size(); b++)
    {
        Component *c = nodeBoxes.at(b).component;
        QRectF box = nodeBoxes.at(b).box;
        QString text = QString::number(c->GetId());

        // Color by value
        QColor col = valToColor(nodeBoxes.at(b).val,colorMap);
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
    for(int b=0; b<linkBoxes.size(); b++)
    {
        QRectF box = linkBoxes.at(b).box;

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

Component *HWTopoPainter::nodeAtPosition(QPoint p)
{
    if(root == NULL)
        return NULL;

    for(int b=0; b<nodeBoxes.size(); b++)
    {
        Component *c = nodeBoxes.at(b).component;
        QRectF box = nodeBoxes.at(b).box;

        bool containsP = false;
        if(vizMode == SUNBURST)
        {
            QPointF radp = reverseRadialTransform(p,rect);
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

