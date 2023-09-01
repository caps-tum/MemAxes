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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QErrorMessage>
#include <QTimer>

#include <QVector>


#include "varvizwidget.h"
#include "pcvizwidget.h"
#include "hwtopovizwidget.h"
#include "action.h"
#include "codevizwidget.h"

#include "hwtopo.h"
#include "codeeditor.h"
#include "console.h"
#include "vizwidget.h"

//#include "volumevizwidget.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
    void selectionChangedSig();
    void visibilityChangedSig();

public slots:
    void frameUpdateAll();
    void selectionChangedSlot();
    void visibilityChangedSlot();
    int loadData();
    int loadDataIBS();
    int selectDataDirectory();
    int selectDirectory(QString *dest, QString directory_name);
    void setHistogramsComboBoxes();
    int selectInt(int *dest, QString wName, QString prompt, int rangeLow, int rangeHigh);
    void showSelectedOnly();
    void removeAxis();
    void correlateAxes();
    void showAll();
    void hideSelected();
    void selectAllVisible();
    void selectAll();
    void deselectAll();
    void addAxis();
    void setSelectModeAND(bool on);
    void setSelectModeOR(bool on);
    void setSelectModeXOR(bool on);
    void setCodeLabel(QFile *file);
    void testSlot(int i);

private:
    Ui::MainWindow *ui;

    QTimer *frameTimer;

    CodeEditor *codeEditor;
    CodeViz *codeViz;
    HWTopoVizWidget *memViz;
    VarViz *varViz;

    QVector<VizWidget*> vizWidgets;
    PCVizWidget *pcViz;
    ActionManager* actionManager;
    //VolumeVizWidget *volumeVizWidget;

    QString dataDir;
    QString fetchDataDir;
    QString opDataDir;
    DataObject *dataSet;
    console *con;
};

#endif // MAINWINDOW_H
