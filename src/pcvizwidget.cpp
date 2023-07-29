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

#include "pcvizwidget.h"

#include <QPaintEvent>
#include <QMenu>

#include <iostream>
#include <algorithm>

#include "util.h"
#include "chrono"

#define SNAPPING true
#define SKIP_GL false
#define TIME_LOGGING false

#define MAXIMUM_LINE_THICKNESS .05
#define MINIMUM_LINE_THICKNESS .005

using namespace std::chrono;

PCVizWidget::PCVizWidget(QWidget *parent)
    : VizWidget(parent)
{
    colorMap.push_back(QColor(166, 206, 227));
    colorMap.push_back(QColor(31, 120, 180));
    colorMap.push_back(QColor(178, 223, 138));
    colorMap.push_back(QColor(51, 160, 44));
    colorMap.push_back(QColor(251, 154, 153));
    colorMap.push_back(QColor(227, 26, 28));
    colorMap.push_back(QColor(253, 191, 111));
    colorMap.push_back(QColor(255, 127, 0));
    colorMap.push_back(QColor(202, 178, 214));
    colorMap.push_back(QColor(106, 61, 154));
    colorMap.push_back(QColor(255, 255, 153));
    colorMap.push_back(QColor(177, 89, 40));

    needsCalcHistBins = true;
    needsCalcMinMaxes = true;
    needsProcessData = true;
    needsProcessSelection = true;
    needsRecalcLines = true;
    needsRepaint = true;

    selOpacity = 0.4;
    unselOpacity = 0.1;

    numHistBins = 50;
    showHistograms = true;

    cursorPos.setX(-1);
    selectionAxis = -1;
    animationAxis = -1;
    movingAxis = -1;

    filterLine = -1;

    axisMouseOver = -1;
    binMouseOver = -1;
    binsInitialize = true;
    codeJumping = true;

    binMatrixValid = false;
    lineStyle = allBins;

    highlightRect = QRect(0, 0, 0, 0);
    highlightLifetime = 0;

    hardwareTopoSamples = nullptr;

    sharedMinMax = true;

    // Event Filters
    this->installEventFilter(this);
    this->setMouseTracking(true);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    this->setFocusPolicy(Qt::StrongFocus);

    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showContextMenu(const QPoint &)));
}

#define LINES_PER_DATAPT (numDimensions - 1)
#define POINTS_PER_LINE 2
#define FLOATS_PER_POINT 2
#define FLOATS_PER_COLOR 4

void printVector(QVector<int> v)
{
    for (int i : v)
    {
        std::cerr << i << ", ";
    }
    std::cerr << std::endl;
}

void PCVizWidget::processData()
{
    processed = false;

    if (dataSet->empty())
        return;

    // numDimensions = dataSet->numDimensions;
    numDimensions = NUM_SAMPLE_AXES;
    numDimensions = min(numDimensions, dataSet->getNumberOfAttributes());

    histValMatrix = (float *)malloc(numHistBins * dataSet->getNumberOfAttributes() * sizeof(float));
    binMatrix = (int *)malloc(dataSet->getNumberOfAttributes() * dataSet->getNumberOfSamples() * sizeof(int));

    allDimMaxes = (long long *)malloc(dataSet->getNumberOfAttributes() * sizeof(long long));
    allDimMins = (long long *)malloc(dataSet->getNumberOfAttributes() * sizeof(long long));

    // dimMins.resize(numDimensions);
    // dimMaxes.resize(numDimensions);

    // dimMins.fill(std::numeric_limits<double>::max());
    // dimMaxes.fill(std::numeric_limits<double>::min());

    selMins.resize(numDimensions);
    selMaxes.resize(numDimensions);

    selMins.fill(-1);
    selMaxes.fill(-1);

    axesPositions.resize(numDimensions);
    axesOrder.resize(numDimensions);
    axesDataIndex.resize(numDimensions);

    histVals.resize(numDimensions);
    histMaxVals.resize(numDimensions);
    histMaxVals.fill(0);

    // Initial axis positions and order
    for (int i = 0; i < numDimensions; i++)
    {
        if (!processed)
            axesOrder[i] = i;

        axesDataIndex[i] = i;

        axesPositions[axesOrder[i]] = i * (1.0 / (numDimensions - 1));

        histVals[i].resize(numHistBins);
        histVals[i].fill(0);
    }

    processed = true;

    calcMinMaxes();
    needsCalcHistBins = true;
    needsRecalcLines = true;
}

void PCVizWidget::leaveEvent(QEvent *e)
{
    binMouseOver = -1;
    VizWidget::leaveEvent(e);
    needsRepaint = true;
}

void PCVizWidget::mousePressEvent(QMouseEvent *mouseEvent)
{
    // this->setFocus();
    if (!processed)
        return;

    QPoint mousePos = mouseEvent->pos();

    if (cursorPos.x() != -1 && mousePos.y() > plotBBox.top())
    {
        selectionAxis = cursorPos.x();
        firstSel = mousePos.y();
    }

    if (mousePos.y() > 0 && mousePos.y() < plotBBox.top())
    {
        movingAxis = getClosestAxis(mousePos.x());
        assert(movingAxis < numDimensions);
    }
}

int PCVizWidget::removeAxis(int index)
{
    if (!axesDataIndex.contains(index))
    {
        return -1;
    }

    // TODO
    int indexOfAxis = axesDataIndex.indexOf(index);
    axesDataIndex.removeAt(indexOfAxis);
    // std::cerr << "automatically removed axis with index " << indexOfAxis << std::endl;
    axesOrder.removeAt(indexOfAxis);
    axesPositions.removeAt(indexOfAxis);
    histVals.removeAt(indexOfAxis);
    histMaxVals.removeAt(indexOfAxis);
    // dimMins.removeAt(indexOfAxis);
    // dimMaxes.removeAt(indexOfAxis);
    selMins.removeAt(indexOfAxis);
    selMaxes.removeAt(indexOfAxis);

    numDimensions--;
    // redistribute axes
    orderByPosition();
    distributeAxes();

    needsRepaint = true;
    needsRecalcLines = true;

    return 0;
}

int PCVizWidget::addAxis(int index)
{
    if (axesDataIndex.contains(index))
    {
        return -1;
    }

    axesDataIndex.push_back(index);
    axesOrder.push_back(axesOrder.length());
    axesPositions.push_back(10000);

    QVector<qreal> nBin;
    histVals.push_back(nBin);
    histVals[histVals.length() - 1].resize(numHistBins);
    histVals[histVals.length() - 1].fill(0);

    // dimMins.push_back(std::numeric_limits<double>::max());
    // dimMaxes.push_back(std::numeric_limits<double>::min());

    histMaxVals.push_back(0);

    selMins.push_back(-1);
    selMaxes.push_back(-1);

    numDimensions++;

    needsRecalcLines = true;
    needsCalcHistBins = true;

    orderByPosition();
    distributeAxes();
    needsRepaint = true;

    // highlightAxis(numDimensions - 1);

    return 0;
}

int PCVizWidget::correlateAxes(int dataIndex1, int dataIndex2)
{
    if (dataIndex1 == dataIndex2)
        return -1;

    int index1 = axesDataIndex.indexOf(dataIndex1);
    int index2 = axesDataIndex.indexOf(dataIndex2);

    if (index1 < 0)
        addAxis(dataIndex1);
    index1 = axesDataIndex.indexOf(dataIndex1);

    if (index2 < 0)
        addAxis(dataIndex2);
    index2 = axesDataIndex.indexOf(dataIndex2);

    if (axesOrder[index1] > axesOrder[index2])
    {
        // std::cerr << "indexes switched\n";
        int temp = index2;
        index2 = index1;
        index1 = temp;
    }

    // std::cerr << "index1 after switch: " << index1 << " index2 after switch: " << index2 << std::endl;
    for (int i = 0; i < numDimensions; i++)
    {
        // std::cerr << i << std::endl;
        if (axesOrder[i] > axesOrder[index1] && axesOrder[i] < axesOrder[index2])
        {
            axesOrder[i]++;
        }
    }
    axesOrder[index2] = axesOrder[index1] + 1;
    // printVector(axesOrder);

    distributeAxes();
    highlightMultipleAxes(index1, index2);
    needsRecalcLines = true;
    needsRepaint = true;
    return 0;
}
void PCVizWidget::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);

    if (!processed)
        return;

    if (selectionAxis != -1 && event->button() == Qt::LeftButton && lastSel == -1)
    {
        std::cerr << "no selection\n";
        selMins[selectionAxis] = -1;
        selMaxes[selectionAxis] = -1;
    }

    needsProcessSelection = true;

    movingAxis = -1;
    selectionAxis = -1;
    lastSel = -1;

    if (animationAxis != -1)
        endAnimation();
}

float PCVizWidget::yToSelection(float y)
{
    return 1.0 - scale(y, plotBBox.top(), plotBBox.bottom(), 0, 1);
}

int PCVizWidget::sourceLineOfBin(int bin)
{
    for (int s = 0; s < dataSet->getNumberOfSamples(); s++)
    {
        if (binMatrix[2 * dataSet->getNumberOfSamples() + s] == bin)
            return dataSet->GetSampleAttribByIndex(s, 2);
    }

    return bin * allDimMaxes[2] / numHistBins;
}

bool PCVizWidget::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);

    if (!processed)
        return false;

    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
    QPoint mousePos = mouseEvent->pos();
    QPoint mouseDelta = mousePos - prevMousePos;

    if (event->type() == QEvent::MouseMove)
    {
        filterLine = -1;

        // Dragging to create a selection
        if (mouseEvent->buttons() && selectionAxis != -1)
        {
            lastSel = clamp((qreal)mousePos.y(), plotBBox.top(), plotBBox.bottom());

            qreal selmin = std::min(firstSel, lastSel);
            qreal selmax = std::max(firstSel, lastSel);

            selMins[selectionAxis] = yToSelection(selmax);
            selMaxes[selectionAxis] = yToSelection(selmin);

            needsRepaint = true;
        }

        // Set cursor position
        int axis = getClosestAxis(mousePos.x());

        axisMouseOver = axis;
        float ySel = yToSelection(mousePos.y());
        // std::cerr << "ySel: " << ySel << std::endl;
        if (ySel < 1 && ySel >= 0 && mousePos.x() > plotBBox.left() && mousePos.x() < plotBBox.right())
        {
            binMouseOver = ySel * numHistBins;
            hardwareTopoSamples = nullptr;
            // uncomment to show all correlations when mouse hovers over an empty bin. Deactivated because it can be irritating
            // if(histVals[axisMouseOver][binMouseOver] == 0)binMouseOver = -1;
            needsRecalcLines = true;
            // line selection stuff
            if (axesDataIndex[axisMouseOver] == 2 && codeJumping)
            {
                // find first element in bin
                for (int s = 0; s < dataSet->getNumberOfSamples(); s++)
                {
                    if (binMatrix[2 * dataSet->getNumberOfSamples() + s] == binMouseOver)
                    {
                        // find source uid of strongest correlated source file
                        int mat[numHistBins * numHistBins];
                        dataCorrelationMatrix(1, 2, mat);
                        int strongestBin = strongestIncomingCor(mat, binMouseOver);
                        int correlatedFileUID = (strongestBin + 1) * allDimMaxes[1] / numHistBins;

                        emit selectSourceFileByIndex(correlatedFileUID);
                        // std::cerr << "emitted select source file by index signal\n";
                        emit lineSelected(dataSet->GetSampleAttribByIndex(s, 2));
                        // std::cerr << "emitting select signal " << (dataSet->GetSampleAttribByIndex(s, 2)) << std::endl;
                        break;
                    }
                }
            }
            else
            {
                if (codeJumping)
                {
                    // switch to strongest correlated source file
                    int mat[numHistBins * numHistBins];
                    dataCorrelationMatrix(1, axesDataIndex[axisMouseOver], mat);
                    int strongestBin = strongestIncomingCor(mat, binMouseOver);
                    int correlatedFileUID = (strongestBin + 1) * allDimMaxes[1] / numHistBins;
                    emit selectSourceFileByIndex(correlatedFileUID);

                    // colored code highlighting
                    mat[numHistBins * numHistBins];
                    dataCorrelationMatrix(axesDataIndex[axisMouseOver], 2, mat);
                    int *lineCorrelations = &mat[numHistBins * binMouseOver];
                    int max = *std::max_element(lineCorrelations, lineCorrelations + numHistBins);

                    vector<tuple<int, float>> highlightVector;
                    for (int i = 0; i < numHistBins; i++)
                    {
                        if (lineCorrelations[i] != 0)
                        {
                            // TODO add element to highlight vector
                            highlightVector.push_back(make_tuple(sourceLineOfBin(i), (float)lineCorrelations[i] / (float)max));
                        }
                    }

                    emit highlightLines(highlightVector);
                }
            }
            // instruction tooltip
            if (axesDataIndex[axisMouseOver] == 3)
            {
                int instructionUID = scale(binMouseOver, 0, numHistBins - 1, allDimMins[3], allDimMaxes[3]);
                QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
                QToolTip::showText(mouseEvent->globalPos(), dataSet->getInstruction(instructionUID), this, rect());
            }
            else
            {
                QToolTip::hideText();
            }
        }
        else
        {
            if (binMouseOver >= 0)
            {
                binMouseOver = -1;
                needsRecalcLines = true;
            }
        }

        cursorPos.setX(axis);
        cursorPos.setY(std::max((int)mousePos.y(), (int)plotBBox.top()));
        cursorPos.setY(std::min((int)cursorPos.y(), (int)plotBBox.bottom()));

        if (cursorPos != prevCursorPos)
            needsRepaint = true;

        // Move axes
        if (movingAxis != -1 && mouseDelta.x() != 0)
        {
            axesPositions[movingAxis] = ((qreal)mousePos.x() - plotBBox.left()) / plotBBox.width();
            orderByPosition();
            if (SNAPPING)
            {
                distributeAxes();
            }
            needsRecalcLines = true;
        }
    }

    if (event->type() == QEvent::KeyPress)
    {
        std::cerr << "key pressed event\n";
    }

    prevMousePos = mousePos;
    prevCursorPos = cursorPos;

    return false;
}

int PCVizWidget::getClosestAxis(int xval)
{
    qreal dist;
    int closestDistance = plotBBox.width();
    int closestAxis = -1;
    for (int i = 0; i < numDimensions; i++)
    {
        dist = abs(40 + axesPositions[i] * plotBBox.width() - xval);
        if (dist < closestDistance)
        {
            closestDistance = dist;
            closestAxis = i;
        }
    }
    return closestAxis;
}

void PCVizWidget::selectValRange(int dataIndex, float rangeMin, float rangeMax)
{
    selMins[axesDataIndex.indexOf(dataIndex)] = scale(rangeMin, allDimMins[dataIndex], allDimMaxes[dataIndex], 0, 1);
    selMaxes[axesDataIndex.indexOf(dataIndex)] = scale(rangeMax, allDimMins[dataIndex], allDimMaxes[dataIndex], 0, 1);

    needsProcessSelection = true;
    needsCalcHistBins = true;
    needsRecalcLines = true;
    needsRepaint = true;
}

void PCVizWidget::selectValRelativeRange(int dataIndex, float rangeMin, float rangeMax)
{
    selMins[axesDataIndex.indexOf(dataIndex)] = rangeMin;
    selMaxes[axesDataIndex.indexOf(dataIndex)] = rangeMax;

    needsProcessSelection = true;
    needsCalcHistBins = true;
    needsRecalcLines = true;
    needsRepaint = true;
}

void PCVizWidget::processSelection()
{
    QVector<int> selDims;
    QVector<qreal> dataSelMins;
    QVector<qreal> dataSelMaxes;

    std::cerr << "processing selection\n";

    if (!processed)
        return;

    for (int i = 0; i < selMins.size(); i++)
    {
        if (selMins[i] != -1)
        {
            selDims.push_back(axesDataIndex[i]);
            dataSelMins.push_back(lerp(selMins[i], allDimMins[axesDataIndex[i]], allDimMaxes[axesDataIndex[i]]));
            dataSelMaxes.push_back(lerp(selMaxes[i], allDimMins[axesDataIndex[i]], allDimMaxes[axesDataIndex[i]]));
        }
    }

    if (selDims.isEmpty())
    {
        // animationAxis = -1;
        // anselMins.fill(-1);
        // anselMaxes.fill(-1);
        // dataSet->deselectAll();
    }
    else
    {
        if (animationAxis != -1)
        {
            selection_mode s = dataSet->selectionMode();
            dataSet->setSelectionMode(MODE_NEW, true);
            dataSet->selectSet(animSet);
            dataSet->setSelectionMode(MODE_FILTER, true);
            dataSet->selectByMultiDimRange(selDims, dataSelMins, dataSelMaxes);
            dataSet->setSelectionMode(s, true);
        }
        else
        {
            std::cerr << "attempting selectByMultiDimRange\n";
            dataSet->selectByMultiDimRange(selDims, dataSelMins, dataSelMaxes);
        }
    }
    /*
    if (animationAxis == -1)
    {
        selMins.fill(-1);
        selMaxes.fill(-1);
    }*/

    needsRecalcLines = true;

    emit selectionChangedSig();
}

void PCVizWidget::calcMinMaxes()
{
    if (!processed)
        return;

    // dimMins.fill(std::numeric_limits<double>::max());
    // dimMaxes.fill(std::numeric_limits<double>::min());

    std::fill_n(allDimMins, dataSet->getNumberOfAttributes(), std::numeric_limits<long long>::max());
    std::fill_n(allDimMaxes, dataSet->getNumberOfAttributes(), std::numeric_limits<long long>::min());

    for (int s = 0; s < dataSet->getNumberOfSamples(); s++)
    {
        if (!dataSet->visible(s))
            continue;
        for (int i = 0; i < dataSet->getNumberOfAttributes(); i++)
        {
            long long val = dataSet->GetSampleAttribByIndex(s, i);

            allDimMins[i] = std::min(allDimMins[i], val);
            allDimMaxes[i] = std::max(allDimMaxes[i], val);
        }
    }

    binMatrixValid = true;
    std::fill_n(binMatrix, dataSet->getNumberOfAttributes() * dataSet->getNumberOfSamples(), -1);

    for (int s = 0; s < dataSet->getNumberOfSamples(); s++)
    {
        for (int i = 0; i < dataSet->getNumberOfAttributes(); i++)
        {
            long long val = dataSet->GetSampleAttribByIndex(s, i);

            int histBin = floor(scale(val, allDimMins[i], allDimMaxes[i], 0, numHistBins));

            if (histBin >= numHistBins)
                histBin = numHistBins - 1;
            if (histBin < 0)
                histBin = 0;

            binMatrix[dataSet->getNumberOfSamples() * i + s] = histBin;
            // histMaxVals[i] = std::max(histMaxVals[i], histVals[i][histBin]);
        }
    }
}

bool PCVizWidget::axisInteresting(int axis)
{
    int populatedAxes = 0;
    for (int i = 0; i < numHistBins; i++)
    {
        if (histVals[axis][i] != 0)
            populatedAxes++;
    }
    return populatedAxes > 1;
}

void PCVizWidget::eliminateEmptyAxes()
{
    int i = 0;
    while (i < numDimensions)
    {
        if (axisInteresting(i))
            i++;
        else
            removeAxis(axesDataIndex[i]);
        // std::cerr << "this goes on forever i: "<< i << " numDimensions: " << numDimensions << "\n";
    }
}

void PCVizWidget::calcHistBins()
{

    milliseconds msStart = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    if (!processed)
        return;

    histMaxVals.fill(0);

    for (int i = 0; i < dataSet->getNumberOfAttributes() * dataSet->getNumberOfSamples(); i++)
    {
        if (binMatrix[i] < 0)
            std::cerr << "this is very bad" << i << "\n";
    }

    for (int s = 0; s < dataSet->getNumberOfSamples(); s++)
    {
        if (dataSet->selectionDefined() && !dataSet->selected(s))
            continue;

        for (int i = 0; i < numDimensions; i++)
        {
            histVals[i][binMatrix[axesDataIndex[i] * dataSet->getNumberOfSamples() + s]]++;
            if (histVals[i][binMatrix[axesDataIndex[i] * dataSet->getNumberOfSamples() + s]] > histMaxVals[i])
                histMaxVals[i]++;
        }
    }

    // int elem;
    // QVector<qreal>::Iterator p;
    // for(elem=0, p=dataSet->begin; p!=dataSet->end; elem++, p+=numDimensions)
    // {
    //     if(dataSet->selectionDefined() && !dataSet->selected(elem))
    //         continue;
    //
    //     for(int i=0; i<numDimensions; i++)
    //     {
    //         int histBin = floor(scale(*(p+i),dimMins[i],dimMaxes[i],0,numHistBins));
    //
    //         if(histBin >= numHistBins)
    //             histBin = numHistBins-1;
    //         if(histBin < 0)
    //             histBin = 0;
    //
    //         histVals[i][histBin] += 1;
    //         histMaxVals[i] = std::max(histMaxVals[i],histVals[i][histBin]);
    //     }
    // }

    // Scale hist values to [0,1]
    for (int i = 0; i < numDimensions; i++)
        for (int j = 0; j < numHistBins; j++)
            histVals[i][j] = scale(histVals[i][j], 0, histMaxVals[i], 0, 1);

    if (TIME_LOGGING)
    {
        milliseconds msEnd = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
        auto elapsed = msEnd - msStart;
        std::cerr << "calculating hist bins took " << elapsed.count() << " milliseconds\n";
    }
}

void matrixSpeedTest(long long *matrix, long numElements)
{
    long long sum = 0;
    milliseconds msStart = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    for (long i = 0; i < numElements; i++)
    {
        sum += matrix[i];
    }
    if (TIME_LOGGING)
    {
        milliseconds msEnd = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
        auto elapsed = msEnd - msStart;
        std::cerr << "iterating " << numElements << " elements took " << elapsed.count() << " milliseconds\n";
    }
}

bool orderByFirst(tuple<float, float, float, int> a, tuple<float, float, float, int> b)
{
    return (get<0>(a) < get<0>(b));
}

void PCVizWidget::dataCorrelationMatrix(int dataIndex1, int dataIndex2, int *mat)
{
    std::fill(mat, mat + numHistBins * numHistBins, 0);

    int *bins1Start = &binMatrix[dataIndex1 * dataSet->getNumberOfSamples()];
    int *bins2Start = &binMatrix[dataIndex2 * dataSet->getNumberOfSamples()];

    for (int s = 0; s < dataSet->getNumberOfSamples(); s++)
    {
        mat[bins1Start[s] * numHistBins + bins2Start[s]]++;
    }
}

int PCVizWidget::strongestOutgoingCor(int *mat, int outgoingBin)
{
    int max = 0;
    int strongestLine = -1;
    for (int j = 0; j < numHistBins; j++)
    {
        if (mat[numHistBins * outgoingBin + j] > max)
            strongestLine = j;
    }
    return strongestLine;
}

int PCVizWidget::strongestIncomingCor(int *mat, int incomingBin)
{
    int max = 0;
    int strongestColumn = -1;
    for (int i = 0; i < numHistBins; i++)
    {
        if (mat[numHistBins * i + incomingBin] > max)
            strongestColumn = i;
    }
    return strongestColumn;
}

// calculates vertex positions for drawing connection lines between neighboring histograms
void PCVizWidget::recalcLines(int dirtyAxis)
{

    // std::cerr << "recalc with filter "<<filterLine<<std::endl;
    // std::cerr << "entering recalcLines\n";
    if (SKIP_GL)
        return;

    if (!processed)
    {
        // std::cerr << "exiting recalcLines: Data not processed\n";
        return;
    }

    verts.clear();
    colors.clear();

    milliseconds msStart = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

    QVector4D col;
    QVector2D a, b;
    int i, axis, nextAxis; //, elem;
    // QVector<double>::Iterator p;

    int *condAxisStart = binMatrix;
    int condAxis = 0;
    if (axisMouseOver >= 0)
        condAxis = axisMouseOver;
    condAxisStart = &binMatrix[dataSet->getNumberOfSamples() * axesDataIndex[condAxis]];
    // std::cerr << axisMouseOver << " : " << binMouseOver << std::endl;

    for (int dim = 0; dim < numDimensions - 1; dim++)
    {
        if (dirtyAxis != -1 && i != dirtyAxis && i != dirtyAxis - 1)
            continue;

        axis = axesOrder.indexOf(dim);
        int *axisStart = &binMatrix[dataSet->getNumberOfSamples() * axesDataIndex[axis]];
        float axisPos = axesPositions[axis];

        int nextAxis = axesOrder.indexOf(dim + 1);
        int *nextAxisStart = &binMatrix[dataSet->getNumberOfSamples() * axesDataIndex[nextAxis]];
        float nextAxisPos = axesPositions[nextAxis];

        // printVector(axesOrder);
        // if(axis == axesOrder.indexOf(axisMouseOver))std::cerr << "axisMouseOver" << axisMouseOver << "axis: "<< axis << " nextAxis: " << nextAxis << "condAxis: "<< condAxis << "data at Axis: " << axesDataIndex[axis] << " data at nextAxis: " << axesDataIndex[nextAxis] << " binMatrix: " << binMatrix << std::endl;

        int correlationSelected[numHistBins * numHistBins];
        int correlationUnselected[numHistBins * numHistBins];

        std::fill(correlationSelected, correlationSelected + numHistBins * numHistBins, 0);
        std::fill(correlationUnselected, correlationUnselected + numHistBins * numHistBins, 0);

        // std::cerr << "number of samples to iterate "<<(dataSet->getNumberOfSamples()) << std::endl;
        if (hardwareTopoSamples == nullptr)
        {
            for (int s = 0; s < dataSet->getNumberOfSamples(); s++)
            {
                if ((binMouseOver < 0 || condAxisStart[s] == binMouseOver) && (filterLine < 0 || filterLine == dataSet->GetSampleAttribByIndex(s, 2)) && !(dataSet->selectionDefined() && !dataSet->selected(s)))
                {
                    // std::cerr << "attempting to access element " << (axisStart[s] * numHistBins + axisStart[s]) << std::endl;
                    correlationSelected[axisStart[s] * numHistBins + nextAxisStart[s]]++;
                }
                else
                {
                    correlationUnselected[axisStart[s] * numHistBins + nextAxisStart[s]]++;
                }
                // if(axis == 14)std::cerr << "value at 0 to 0: " << correlation[0] << " visibility : " << dataSet->selected(s) << "\n";
            }
        }
        else
        {
            int hardwareTopoSamplesIndex = 0;
            for (int s = 0; s < dataSet->getNumberOfSamples(); s++)
            {
                if (hardwareTopoSamplesIndex < hardwareTopoSamples->size() && hardwareTopoSamples->at(hardwareTopoSamplesIndex) == s)
                {
                    // std::cerr << "attempting to access element " << (axisStart[s] * numHistBins + axisStart[s]) << std::endl;
                    correlationSelected[axisStart[s] * numHistBins + nextAxisStart[s]]++;
                    hardwareTopoSamplesIndex++;
                }
                else
                {
                    correlationUnselected[axisStart[s] * numHistBins + nextAxisStart[s]]++;
                }
            }
        }

        // calculation of transformation parameters
        float scaleTopSelected[numHistBins * numHistBins];
        float scaleBottomSelected[numHistBins * numHistBins];

        float scaleTopUnselected[numHistBins * numHistBins];
        float scaleBottomUnselected[numHistBins * numHistBins];
        switch (lineStyle)
        {
        case allBins:
        {
            // calculate min and max of all correlations
            int allCorrelationsMax = 0;
            int allCorrelationsMin = INT_MAX;
            for (int i = 0; i < numHistBins * numHistBins; i++)
            {
                allCorrelationsMax = std::max(correlationSelected[i], allCorrelationsMax);
                if (correlationSelected[i] != 0)
                    allCorrelationsMin = std::min(correlationSelected[i], allCorrelationsMin);
            }
            if (allCorrelationsMin == allCorrelationsMax)
                allCorrelationsMin = 0;

            for (int k = 0; k < numHistBins * numHistBins; k++)
            {
                scaleTopSelected[k] = allCorrelationsMax;
                scaleBottomSelected[k] = allCorrelationsMin;
            }
        }
        break;
        case firstAxis:
        {
            // outgoing bin id
            for (int i = 0; i < numHistBins; i++)
            {
                if (histVals[axis][i] == 0)
                    continue;
                // calculate min and max
                int corMax = 0;
                int corMin = INT_MAX;
                for (int j = 0; j < numHistBins; j++)
                {
                    corMax = std::max((int)correlationSelected[i * numHistBins + j], corMax);
                    if (correlationSelected[i * numHistBins + j] != 0)
                        corMin = std::min((int)correlationSelected[i * numHistBins + j], corMin);
                }
                if (corMin == corMax)
                    corMin = 0;
                float fillMax = corMax;
                float fillMin = corMin;
                std::fill_n(scaleTopSelected + numHistBins * i, numHistBins, corMax);
                std::fill_n(scaleBottomSelected + numHistBins * i, numHistBins, corMin);
            }
        }
        break;
        case secondAxis:
        {
            // incoming bin id
            for (int i = 0; i < numHistBins; i++)
            {
                if (histVals[nextAxis][i] == 0)
                    continue;
                // calculate min and max
                int corMax = 0;
                int corMin = INT_MAX;
                for (int j = 0; j < numHistBins; j++)
                {
                    corMax = std::max((int)correlationSelected[j * numHistBins + i], corMax);
                    if (correlationSelected[j * numHistBins + i] != 0)
                        corMin = std::min((int)correlationSelected[j * numHistBins + i], corMin);
                }
                if (corMin == corMax)
                    corMin = 0;

                for (int j = 0; j < numHistBins; j++)
                {
                    scaleTopSelected[j * numHistBins + i] = corMax;
                    scaleBottomSelected[j * numHistBins + i] = corMin;
                }

                if (corMin == corMax)
                    corMin = 0;
            }
        }
        break;

        default:
            std::cerr << "something went horribly wrong\n";
            break;
        }

        // line tuple: progress, y1, y2, total strength
        vector<tuple<float, float, float, int>> lines;
        vector<tuple<float, float, float, int>> ulines;

        for (int out = 0; out < numHistBins; out++)
        {
            if (histVals[axis][out] == 0)
                continue;
            float aVal = (float)out / (float)numHistBins + .5 / numHistBins;

            for (int into = 0; into < numHistBins; into++)
            {
                float bVal = (float)into / (float)numHistBins + .5 / numHistBins;

                if (correlationSelected[out * numHistBins + into] != 0)
                {

                    float progress = scale(correlationSelected[out * numHistBins + into], scaleBottomSelected[out * numHistBins + into], scaleTopSelected[out * numHistBins + into], 0, 1);
                    /*
                    if (axis == 14)
                        std::cerr << "bottom: " << scaleBottom[out * numHistBins + into] << " top: " << scaleTop[out * numHistBins + into] << " correlation strength: " << correlation[out * numHistBins + into] << " progress: " << progress << std::endl;
                    */
                    lines.push_back(make_tuple(progress, aVal, bVal, correlationSelected[out * numHistBins + into]));
                }

                if (correlationUnselected[out * numHistBins + into] != 0)
                {
                    float progress = scale(correlationUnselected[out * numHistBins + into], scaleBottomUnselected[out * numHistBins + into], scaleTopUnselected[out * numHistBins + into], 0, 1);
                    ulines.push_back(make_tuple(1, aVal, bVal, correlationUnselected[out * numHistBins + into]));
                }
            }
        }

        std::sort(lines.begin(), lines.end(), orderByFirst);

        for (tuple<float, float, float, int> line : ulines)
        {
            float progress = std::get<0>(line);

            float thickness = ((((float)std::get<3>(line) / (float)dataSet->getNumberOfSamples())) * (MAXIMUM_LINE_THICKNESS - MINIMUM_LINE_THICKNESS) + MINIMUM_LINE_THICKNESS) * glViewPortHeight / 2;

            float firstPos = axisPos * glViewportWidth;
            float secondPos = nextAxisPos * glViewportWidth;

            float firstHeight = std::get<1>(line) * glViewPortHeight;
            float secondHeight = std::get<2>(line) * glViewPortHeight;

            float xDist = secondPos - firstPos;
            float yDist = std::abs(firstHeight - secondHeight);

            float hypothenuseSquared = xDist * xDist + yDist * yDist;
            float hypothenuse = std::sqrt(hypothenuseSquared);
            float inverseSin = hypothenuse / xDist;

            float offset = thickness * inverseSin;

            // first triangle
            verts.push_back(firstPos);
            verts.push_back(firstHeight - offset);

            verts.push_back(secondPos);
            verts.push_back(secondHeight - offset);

            verts.push_back(secondPos);
            verts.push_back(secondHeight + offset);

            // second triangle
            verts.push_back(firstPos);
            verts.push_back(firstHeight + offset);

            verts.push_back(firstPos);
            verts.push_back(firstHeight - offset);

            verts.push_back(secondPos);
            verts.push_back(secondHeight + offset);

            int h = 0;
            int s = 0;
            int l = 150;

            // l = progress;

            // if(axis = axisMouseOver)std::cerr<< "hue: " << h << "\n";

            QColor lineCol = QColor(0, 0, 0);
            lineCol.setHsl(h, s, l);

            // if(axis = axisMouseOver)std::cerr << "r: " << lineCol.red() << std::endl;

            colors.push_back((float)lineCol.red() / 255);
            colors.push_back((float)lineCol.green() / 255);
            colors.push_back((float)lineCol.blue() / 255);
            colors.push_back(1);

            colors.push_back((float)lineCol.red() / 255);
            colors.push_back((float)lineCol.green() / 255);
            colors.push_back((float)lineCol.blue() / 255);
            colors.push_back(1);

            colors.push_back((float)lineCol.red() / 255);
            colors.push_back((float)lineCol.green() / 255);
            colors.push_back((float)lineCol.blue() / 255);
            colors.push_back(1);

            colors.push_back((float)lineCol.red() / 255);
            colors.push_back((float)lineCol.green() / 255);
            colors.push_back((float)lineCol.blue() / 255);
            colors.push_back(1);

            colors.push_back((float)lineCol.red() / 255);
            colors.push_back((float)lineCol.green() / 255);
            colors.push_back((float)lineCol.blue() / 255);
            colors.push_back(1);

            colors.push_back((float)lineCol.red() / 255);
            colors.push_back((float)lineCol.green() / 255);
            colors.push_back((float)lineCol.blue() / 255);
            colors.push_back(1);
        }

        for (tuple<float, float, float, int> line : lines)
        {
            float progress = std::get<0>(line);

            float thickness = ((((float)std::get<3>(line) / (float)dataSet->getNumberOfSamples())) * (MAXIMUM_LINE_THICKNESS - MINIMUM_LINE_THICKNESS) + MINIMUM_LINE_THICKNESS) * glViewPortHeight / 2;

            float firstPos = axisPos * glViewportWidth;
            float secondPos = nextAxisPos * glViewportWidth;

            float firstHeight = std::get<1>(line) * glViewPortHeight;
            float secondHeight = std::get<2>(line) * glViewPortHeight;

            float xDist = secondPos - firstPos;
            float yDist = std::abs(firstHeight - secondHeight);

            float hypothenuseSquared = xDist * xDist + yDist * yDist;
            float hypothenuse = std::sqrt(hypothenuseSquared);
            float inverseSin = hypothenuse / xDist;

            float offset = thickness * inverseSin;

            // first triangle
            verts.push_back(firstPos);
            verts.push_back(firstHeight - offset);

            verts.push_back(secondPos);
            verts.push_back(secondHeight - offset);

            verts.push_back(secondPos);
            verts.push_back(secondHeight + offset);

            // second triangle
            verts.push_back(firstPos);
            verts.push_back(firstHeight + offset);

            verts.push_back(firstPos);
            verts.push_back(firstHeight - offset);

            verts.push_back(secondPos);
            verts.push_back(secondHeight + offset);

            int h = -20.f * progress + 20.f;
            int s = 255;
            int l = -150.f * progress + 220.f;

            // l = progress;

            // if(axis = axisMouseOver)std::cerr<< "hue: " << h << "\n";

            QColor lineCol = QColor(0, 0, 0);
            lineCol.setHsl(h, s, l);

            // if(axis = axisMouseOver)std::cerr << "r: " << lineCol.red() << std::endl;

            colors.push_back((float)lineCol.red() / 255);
            colors.push_back((float)lineCol.green() / 255);
            colors.push_back((float)lineCol.blue() / 255);
            colors.push_back(1);

            colors.push_back((float)lineCol.red() / 255);
            colors.push_back((float)lineCol.green() / 255);
            colors.push_back((float)lineCol.blue() / 255);
            colors.push_back(1);

            colors.push_back((float)lineCol.red() / 255);
            colors.push_back((float)lineCol.green() / 255);
            colors.push_back((float)lineCol.blue() / 255);
            colors.push_back(1);

            colors.push_back((float)lineCol.red() / 255);
            colors.push_back((float)lineCol.green() / 255);
            colors.push_back((float)lineCol.blue() / 255);
            colors.push_back(1);

            colors.push_back((float)lineCol.red() / 255);
            colors.push_back((float)lineCol.green() / 255);
            colors.push_back((float)lineCol.blue() / 255);
            colors.push_back(1);

            colors.push_back((float)lineCol.red() / 255);
            colors.push_back((float)lineCol.green() / 255);
            colors.push_back((float)lineCol.blue() / 255);
            colors.push_back(1);
        }
    }

    if (TIME_LOGGING)
    {
        milliseconds msEnd = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
        auto elapsed = msEnd - msStart;
        std::cerr << "recalc lines took " << elapsed.count() << " milliseconds created " << verts.size() << " vertices\n";
        // matrixSpeedTest(dataSet->GetSampleMatrix(), dataSet->getNumberOfSamples() * dataSet->getNumberOfAttributes());
    }
}

void PCVizWidget::showContextMenu(const QPoint &pos)
{
    contextMenuMousePos = pos;

    QMenu contextMenu(tr("Axis Menu"), this);

    QAction actionAnimate("Animate!", this);
    connect(&actionAnimate, SIGNAL(triggered()), this, SLOT(beginAnimation()));
    contextMenu.addAction(&actionAnimate);

    contextMenu.exec(mapToGlobal(pos));
}

void PCVizWidget::selectionChangedSlot()
{
    needsCalcHistBins = true;
    needsRecalcLines = true;
    needsRepaint = true;
}

void PCVizWidget::visibilityChangedSlot()
{
    needsProcessData = true;
    needsCalcMinMaxes = true;
    needsRepaint = true;
}

void PCVizWidget::setSelOpacity(int val)
{
    selOpacity = (qreal)val / 1000.0;
    needsRecalcLines = true;
    needsRepaint = true;
}

void PCVizWidget::setUnselOpacity(int val)
{
    unselOpacity = (qreal)val / 1000.0;
    needsRecalcLines = true;
    needsRepaint = true;
}

void PCVizWidget::setShowHistograms(bool checked)
{
    showHistograms = checked;
    needsRepaint = true;
}

void PCVizWidget::resetSelection()
{
    selMins.fill(-1);
    selMaxes.fill(-1);
}

void PCVizWidget::frameUpdate()
{
    // Animate
    if (animationAxis != -1)
    {
        selMins[animationAxis] += 0.005;
        selMaxes[animationAxis] += 0.005;

        needsProcessSelection = true;
        needsRepaint = true;

        if (selMaxes[animationAxis] >= 1)
        {
            endAnimation();
            needsProcessSelection = false;
        }
    }

    // Necessary updates
    if (needsProcessData)
    {
        processData();
        needsProcessData = false;
    }
    if (needsProcessSelection)
    {
        processSelection();
        needsProcessSelection = false;
    }
    if (needsCalcMinMaxes)
    {
        calcMinMaxes();
        needsCalcMinMaxes = false;
    }
    if (needsCalcHistBins)
    {
        calcHistBins();
        needsCalcHistBins = false;
    }
    if (binsInitialize && processed)
    {
        eliminateEmptyAxes();
        binsInitialize = false;
    }
    if (needsRecalcLines)
    {
        recalcLines();
        needsRecalcLines = false;
    }
    if (needsRepaint)
    {
        needsRepaint = false;
        repaint();
    }
}

void PCVizWidget::beginAnimation()
{
    animSet.clear();

    ElemSet selSet = dataSet->getSelectionSet();
    emptySet = false;

    if (selSet.empty())
    {
        emptySet = true;

        selection_mode s = dataSet->selectionMode();
        dataSet->setSelectionMode(MODE_NEW, true);
        dataSet->selectAll();
        dataSet->setSelectionMode(s, true);

        selSet = dataSet->getSelectionSet();
    }

    std::copy(selSet.begin(), selSet.end(), std::inserter(animSet, animSet.begin()));

    animationAxis = getClosestAxis(contextMenuMousePos.x());
    movingAxis = -1;

    qreal selDelta = selMaxes[animationAxis] - selMins[animationAxis];
    if (selDelta == 0)
        selDelta = 0.1;

    selMins[animationAxis] = 0;
    selMaxes[animationAxis] = selDelta;

    needsProcessSelection = true;
    needsRepaint = true;
}

void PCVizWidget::endAnimation()
{
    animationAxis = -1;

    selection_mode s = dataSet->selectionMode();
    dataSet->setSelectionMode(MODE_NEW, true);

    if (emptySet)
    {
        dataSet->deselectAll();
    }
    else
    {
        dataSet->selectSet(animSet);
    }

    dataSet->setSelectionMode(s, true);
    animSet.clear();
}

void PCVizWidget::paintGL()
{
    milliseconds msStart = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

    // uncomment for grey parallel histograms background
    // glClearColor(.4,.4,.4,1.);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!processed)
        return;

    makeCurrent();

    int mx = 40;
    int my = 30;

    glViewportWidth = width() - 2 * mx;
    glViewPortHeight = height() - 2 * my;

    glViewport(mx,
               my,
               glViewportWidth,
               glViewPortHeight);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, glViewportWidth, 0.0, glViewPortHeight, 0, 1);

    glShadeModel(GL_FLAT);
    // keep enabled to use the alpha channel for reducing the brightness of graph links
    // glDisable(GL_DEPTH_TEST);

    // make lines nicely antialiased
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glDisable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glVertexPointer(FLOATS_PER_POINT, GL_FLOAT, 0, verts.constData());
    glColorPointer(FLOATS_PER_COLOR, GL_FLOAT, 0, colors.constData());

    if (!SKIP_GL)
        glDrawArrays(GL_TRIANGLES, 0, verts.size() / 2);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    if (TIME_LOGGING)
    {
        milliseconds msEnd = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
        auto elapsed = msEnd - msStart;
        if (!SKIP_GL)
            std::cerr << "paintGL took " << elapsed.count() << " milliseconds\n";
    }
}

void PCVizWidget::highlightAxis(int index)
{
    float widthPerElement = plotBBox.width() / numDimensions;

    float x1 = plotBBox.left() + axesPositions[index] * plotBBox.width() - .2 * widthPerElement;
    float x2 = x1 + widthPerElement;

    highlightRect = QRect(qreal(x1), plotBBox.top(), widthPerElement, plotBBox.bottom());
    highlightLifetime = 20;
}

void PCVizWidget::highlightMultipleAxes(int indexStart, int indexEnd)
{
    /*
    std::cerr << indexStart << " : " << indexEnd << std::endl;
    std::cerr << axesOrder[indexStart] << " ; " << axesOrder[indexEnd] << std::endl << std::endl;
    std::cerr << axesPositions[indexStart] << " e " << axesPositions[indexEnd] << std::endl << std::endl;
    */
    float widthPerElement = plotBBox.width() / numDimensions;

    float x1 = plotBBox.left() + axesPositions[indexStart] * plotBBox.width() - .2 * widthPerElement;
    float x2 = plotBBox.left() + axesPositions[indexEnd] * plotBBox.width() + .8 * widthPerElement;

    highlightRect = QRect(x1, plotBBox.top(), x2 - x1, plotBBox.height());
    highlightLifetime = 20;
}

void PCVizWidget::drawQtPainter(QPainter *painter)
{
    if (!processed)
        return;

    // painter->fillRect(this->rect(), Qt::white);

    int mx = 40;
    int my = 30;

    plotBBox = QRectF(mx, my,
                      width() - mx - mx,
                      height() - my - my);

    float xSpacePerAxis = plotBBox.width() / numDimensions;

    // Draw axes
    QPointF a = plotBBox.bottomLeft();
    QPointF b = plotBBox.topLeft();

    QFontMetrics fm = painter->fontMetrics();
    painter->setBrush(QColor(0, 0, 0));
    for (int i = 0; i < numDimensions; i++)
    {
        a.setX(plotBBox.left() + axesPositions[i] * plotBBox.width());
        b.setX(a.x());

        painter->drawLine(a, b);

        QString text = QString::fromStdString(dataSet->GetAttributeName(axesDataIndex[i]));
        // debugging text
        // text = QString("%1 : %2 : %3").arg(i).arg(axesDataIndex[i]).arg(axesOrder[i]);
        QPointF center = b - QPointF(fm.horizontalAdvance(text) / 2, 15);

        painter->drawText(center, text);

        text = QString::number(allDimMins[axesDataIndex[i]], 'g', 2);
        center = a - QPointF(fm.horizontalAdvance(text) / 2, -10);
        painter->drawText(center, text);

        text = QString::number(allDimMaxes[axesDataIndex[i]], 'g', 2);
        center = b - QPointF(fm.horizontalAdvance(text) / 2, 0);
        painter->drawText(center, text);
    }

    // Draw cursor
    painter->setOpacity(1);
    if (cursorPos.x() != -1)
    {
        a.setX(plotBBox.left() + axesPositions[(int)cursorPos.x()] * plotBBox.width() - 10);
        a.setY(cursorPos.y());

        b.setX(plotBBox.left() + axesPositions[(int)cursorPos.x()] * plotBBox.width() + 10);
        b.setY(cursorPos.y());

        // std::cerr << cursorPos.y() << std::endl;
        painter->drawLine(a, b);
    }

    // Draw selection boxes
    int cursorWidth = 28;
    int halfCursorWidth = cursorWidth / 2;

    painter->setPen(QPen(Qt::yellow, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->setBrush(Qt::NoBrush);
    for (int i = 0; i < numDimensions; i++)
    {
        if (selMins[i] != -1)
        {
            a = QPointF(plotBBox.left() + axesPositions[i] * plotBBox.width() - halfCursorWidth,
                        plotBBox.top() + plotBBox.height() * (1.0 - selMins[i]));
            b = QPointF(a.x() + cursorWidth,
                        plotBBox.top() + plotBBox.height() * (1.0 - selMaxes[i]));

            painter->drawRect(QRectF(a, b));
        }
    }

    // draw highlight box
    painter->setPen(QPen(Qt::magenta, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    if (highlightLifetime > 0)
    {
        painter->drawRect(highlightRect);
        highlightRect = QRect(highlightRect.left() + 1, highlightRect.top() + 1, highlightRect.width() - 2, highlightRect.height() - 2);
        needsRepaint = true;
        highlightLifetime -= 1;
    }

    if (showHistograms)
    {
        // Draw histograms
        a = plotBBox.bottomLeft();
        b = plotBBox.topLeft();

        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(31, 120, 180));
        painter->setOpacity(0.7);

        for (int i = 0; i < numDimensions; i++)
        {
            a.setX(plotBBox.left() + axesPositions[i] * plotBBox.width());
            b.setX(a.x());

            for (int j = 0; j < numHistBins; j++)
            {
                qreal histTop = a.y() - (j + 1) * (plotBBox.height() / numHistBins);
                qreal histLeft = a.x(); //-30*histVals[i][j];
                qreal histBottom = a.y() - (j) * (plotBBox.height() / numHistBins);
                qreal histRight = a.x() + xSpacePerAxis * .9 * histVals[i][j];
                painter->drawRect(QRectF(QPointF(histLeft, histTop), QPointF(histRight, histBottom)));
            }

            painter->drawLine(a, b);
        }
    }
}

void PCVizWidget::distributeAxes()
{
    for (int i = 0; i < numDimensions; i++)
    {
        axesPositions[i] = axesOrder[i] * (1.0 / (numDimensions - 1));
    }
}

void PCVizWidget::orderByPosition()
{

    for (int i = 0; i < numDimensions; i++)
    {
        axesOrder[i] = -1;
    }

    for (int i = 0; i < numDimensions; i++)
    {
        int smallestIndex = -1;
        float smallestX = INFINITY;

        for (int j = 0; j < numDimensions; j++)
        {
            if (axesOrder[j] < 0)
            {
                if (axesPositions[j] < smallestX)
                {
                    smallestX = axesPositions[j];
                    smallestIndex = j;
                }
            }
        }

        axesOrder[smallestIndex] = i;
    }
}

void PCVizWidget::setLineColoringAllBins()
{
    lineStyle = allBins;
    needsRecalcLines = true;
    needsRepaint = true;
}

void PCVizWidget::setLineColoringFirstAxis()
{
    lineStyle = firstAxis;
    needsRecalcLines = true;
    needsRepaint = true;
}

void PCVizWidget::setLineColoringSecondAxis()
{
    lineStyle = secondAxis;
    needsRecalcLines = true;
    needsRepaint = true;
}

void PCVizWidget::setFilterLine(int filter)
{
    filterLine = filter;
    // std::cerr << "filtering by: "<< filter << std::endl;
    needsRecalcLines = true;
    needsRepaint = true;
}

void PCVizWidget::setNumHistBins(int n)
{
    numHistBins = n;

    for (int i = 0; i < numDimensions; i++)
    {
        histVals[i].resize(numHistBins);
        histVals[i].fill(0);
    }

    needsCalcMinMaxes = true;
    needsCalcHistBins = true;
    needsRecalcLines = true;
    needsRepaint = true;
}

void PCVizWidget::toggleCodeJumping()
{
    codeJumping = !codeJumping;
}

float PCVizWidget::dataMax(int index)
{
    return allDimMaxes[index];
}

float PCVizWidget::dataMin(int index)
{
    return allDimMins[index];
}

void PCVizWidget::setHardwareTopologySampleSet(vector<int> *indices)
{
    // std::cerr << "receiving hardware topology sample set\n";
    hardwareTopoSamples = indices;
    needsRecalcLines = true;
    needsRepaint = true;
}

bool PCVizWidget::hasAxis(int dataIndex)
{
    return axesDataIndex.contains(dataIndex);
}

void PCVizWidget::selectAll()
{
    dataSet->selectAll();

    for (int i = 0; i < numDimensions; i++)
    {
        selMins[i] = -1;
        selMaxes[i] = -1;
    }

    needsRecalcLines = true;
    needsCalcHistBins = true;
    needsRepaint = true;
}

int PCVizWidget::populatedBins(int dataIndex)
{
    int acc = 0;
    vector<bool> populated;
    populated.resize(numHistBins, false);
    for (int i = 0; i < dataSet->getNumberOfSamples(); i++)
    {
        int bin = binMatrix[dataIndex * dataSet->getNumberOfSamples() + i];
        if (!populated[bin])
        {
            populated[bin] = true;
            acc++;
        }
    }
    return acc;
}

void PCVizWidget::toggleSharedMinMax(){
    sharedMinMax = !sharedMinMax;
    std::cerr << "toggled shared min max\n";
    needsRecalcLines = true;
    needsRepaint = true;
}