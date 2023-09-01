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

#ifndef PCVIZWIDGET_H
#define PCVIZWIDGET_H

#include "vizwidget.h"

#include <QVector2D>
#include <QVector4D>
#include <QToolTip>

enum LineColorMode{allBins, firstAxis, secondAxis};

class PCVizWidget
        : public VizWidget
{
    Q_OBJECT

public:
    PCVizWidget(QWidget *parent = 0);

public:
    void dataCorrelationMatrix(int dataIndex1, int dataIndex2, int *mat);
    void recalcLines(int dirtyAxis = -1);
    int strongestOutgoingCor(int* mat, int outgoingBin);
    int strongestIncomingCor(int* mat, int incomingBin);
    void selectValRange(int dataIndex, float rangeMin, float rangeMax);
    void selectValRelativeRange(int dataIndex, float rangeMin, float rangeMax);
    int removeAxis(int index);
    int addAxis(int index);
    int correlateAxes(int dataIndex1, int dataIndex2);
    float dataMax(int dataIndex);
    float dataMin(int dataIndex);
    bool hasAxis(int dataIndex);
    int populatedBins(int dataIndex);

    void selectAll();

signals:
    void lineSelected(int line);
    void selectSourceFileByIndex(int index);
    void highlightLines(vector<tuple<int, float>> lines);

public slots:
    void frameUpdate();
    void selectionChangedSlot();
    void visibilityChangedSlot();

    void setNumHistBins(int n);

    void showContextMenu(const QPoint &);
    void setSelOpacity(int val);
    void setUnselOpacity(int val);
    void setShowHistograms(bool checked);
    void toggleSharedMinMax();
    void resetSelection();
    void beginAnimation();
    void endAnimation();

    void setLineColoringAllBins();
    void setLineColoringFirstAxis();
    void setLineColoringSecondAxis();

    void setFilterLine(int);

    void toggleCodeJumping();

    void setHardwareTopologySampleSet(vector<int>* indices);

protected:
    void processData();
    void paintGL();
    void highlightAxis(int index);
    void highlightMultipleAxes(int indexStart, int indexEnd);
    void drawQtPainter(QPainter *painter);

    void distributeAxes(int exception);

    void orderByPosition();

    

    void leaveEvent(QEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void keyPressEvent(QKeyEvent *e) override
    {
        std::cerr << "key pressed\n";
    }
    void mouseReleaseEvent(QMouseEvent *e);
    bool eventFilter(QObject *obj, QEvent *event);

private:
    int getClosestAxis(int xval);
    void processSelection();
    void calcMinMaxes();
    bool axisInteresting(int axis);
    void eliminateEmptyAxes();
    void calcHistBins();
    float yToSelection(float y);

    int sourceLineOfBin(int bin);

private:
    bool needsRecalcLines;
    bool needsCalcHistBins;
    bool needsCalcMinMaxes;
    bool needsProcessData;
    bool needsProcessSelection;

    ElemSet animSet;
    bool emptySet;

    int numDimensions;
    int numHistBins;

    QRectF plotBBox;
    ColorMap colorMap;

    QVector<QVector<qreal> > histVals;
    float* histValMatrix;
    QVector<qreal> histMaxVals;

    //QVector<qreal> dimMins;
    long long* allDimMins;
    //QVector<qreal> dimMaxes;
    long long* allDimMaxes;

    QVector<qreal> selMins;
    QVector<qreal> selMaxes;

    QVector<int> axesOrder;
    QVector<int> axesDataIndex;
    QVector<qreal> axesPositions;

    QPoint contextMenuMousePos;
    QPoint prevMousePos;

    QPointF cursorPos;
    QPointF prevCursorPos;

    int selectionAxis;
    int animationAxis;
    int movingAxis;

    int binMouseOver;
    int axisMouseOver;

    bool showHistograms;
    bool codeJumping;

    int* binMatrix;
    bool binMatrixValid;
    bool binsInitialize;

    qreal firstSel;
    qreal lastSel;

    int filterLine;
    float glViewportWidth;
    float glViewPortHeight;

    qreal selOpacity;
    float unselOpacity;

    // OpenGL
    QVector<GLfloat> verts;
    QVector<GLfloat> colors;

    //line coloring
    LineColorMode lineStyle;
    bool sharedMinMax;

    QRect highlightRect;
    float highlightLifetime;

    vector<int> * hardwareTopoSamples;
};

#endif // PARALLELCOORDINATESVIZ_H
