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

#include "dataobject.h"
#include "parseUtil.h"
#include "chrono"
#include "fstream"
#include "musa-parser.h"


#include <iostream>
#include <algorithm>
#include <functional>

#include <QFile>
#include <QTextStream>

using namespace chrono;

DataObject::DataObject()
{
    // numDimensions = 0;
    numElements = 0;
    numSelected = 0;
    numVisible = 0;

    node = NULL;

    selMode = MODE_NEW;
    selGroup = 1;

}

int DataObject::loadHardwareTopology(QString directory)
{
    int err;
    node = new Node(0);

    //
    QDir qd(directory);
    QStringList topoFiles = qd.entryList(QStringList() << "hardware.xml",QDir::Files);
    if(topoFiles.length() > 0){
        QString filename = directory+QString("/hardware.xml");
        qDebug() << QString("Parsing topology file " + filename);
        err = parseHwlocOutput(node, filename.toUtf8().constData()); //adds topo to a next node
    }
    else{
        topoFiles = qd.entryList(QStringList() << "*.conf",QDir::Files);
        if(topoFiles.length() > 0){
            Chip * socket = new Chip(/*parent*/ node, /*id*/ 0, /*name*/ "MUSA CPU", /*chip type*/ SYS_SAGE_CHIP_TYPE_CPU_SOCKET);
            QString filename = directory+QString("/")+topoFiles[0];
            qDebug() << QString("Parsing MUSA topology file " + filename);
            err = parseMusa(socket, filename.toUtf8().constData());
        }
    }
    qDebug() << QString("Parsing completed");
    //TODO temporary CPU
    //cpu = (Chip*)(node->GetChild(1));

    //create empty metadata items "sampleSets" and "transactions" for each Compoenent
    vector<Component*> allComponents;
    node->GetSubtreeNodeList(&allComponents);

    for(int i=0; i<allComponents.size(); i++)
    {
        // allComponents[i]->attrib["sampleSets"] = (void*) new QMap<DataObject*,SampleSet>();
        allComponents[i]->attrib["transactions"] = (void*) new int();
    }
    qDebug() << QString("loadHardwareTopology completed");
    return err;
}

int DataObject::loadHardwareTopologyIBS(QString filename)
{
    node = new Node(0);
    int err = parseHwlocOutput(node, filename.toUtf8().constData()); //adds topo to a next node

    //TODO temporary CPU
    //cpu = (Chip*)(node->GetChild(1));

    //create empty metadata items "sampleSets" and "transactions" for each Compoenent
    vector<Component*> allComponents;
    node->GetSubtreeNodeList(&allComponents);

    for(int i=0; i<allComponents.size(); i++)
    {
        // allComponents[i]->attrib["sampleSets"] = (void*) new QMap<DataObject*,SampleSet>();
        allComponents[i]->attrib["transactions"] = (void*) new int();
    }
    return err;
}

int DataObject::loadData(QString filename)
{
    int err = parseCSVFile(filename);
    if(err)
        return err;

    calcStatistics();
    // constructSortedLists();
    qDebug() << QString("loadData completed");
    return 0;
}

void DataObject::allocate()
{
    numElements = samples.size();
    // numDimensions = meta.size();
    // numElements = vals.size() / numDimensions;
    //
    // begin = vals.begin();
    // end = vals.end();

    // visibility.resize(numElements);
    // visibility.fill(VISIBLE);

    selectionGroup.resize(numElements);
    selectionGroup.fill(0); // all belong to 0 (unselected)

    selectionSets.push_back(ElemSet());
    selectionSets.push_back(ElemSet());
}

int DataObject::selected(ElemIndex index)
{
    return selectionGroup.at((int)index);
}

bool DataObject::visible(ElemIndex index)
{
    return samples[(int)index].visible;
    // return visibility.at((int)index);
}

bool DataObject::selectionDefined()
{
    return numSelected > 0;
}

void DataObject::selectData(ElemIndex index, int group)
{
    if(visible(index))
    {
        selectionGroup[index] = group;
        selectionSets.at(group).insert(index);
        numSelected++;
    }
}

void DataObject::selectAll(int group)
{
    selectionGroup.fill(group);

    for(ElemIndex i=0; i<numElements; i++)
    {
        selectionSets.at(group).insert(i);
    }

    numSelected = numElements;
}

void DataObject::deselectAll()
{
    selectionGroup.fill(0);

    for(unsigned int i=0; i<selectionSets.size(); i++)
        selectionSets.at(i).clear();

    numSelected = 0;
}

void DataObject::selectAllVisible(int group)
{
    ElemIndex elem;
    for(elem=0; elem<numElements; elem++)
    {
        if(visible(elem))
            selectData(elem,group);
    }
}

void DataObject::showData(unsigned int index)
{
    if(!visible(index))
    {
        samples[(int)index].visible = VISIBLE;
        //visibility[index] = VISIBLE;
        numVisible++;
    }
}

void DataObject::hideData(unsigned int index)
{
    if(visible(index))
    {
        samples[(int)index].visible = INVISIBLE;
        //visibility[index] = INVISIBLE;
        numVisible--;
    }
}

void DataObject::showAll()
{
    for(Sample s: samples)
        s.visible = VISIBLE;
    // visibility.fill(VISIBLE);
    numVisible = numElements;
}

void DataObject::hideAll()
{
    for(Sample s: samples)
        s.visible = INVISIBLE;
    // visibility.fill(INVISIBLE);
    numVisible = 0;
}

void DataObject::selectBySourceFileName(QString str, int group)
{
    ElemSet selSet;
    for(Sample s: samples)
    {
        if(s.source == str)
            selSet.insert(s.sampleId);
    }
    // ElemIndex elem;
    // QVector<qreal>::Iterator p;
    // for(elem=0, p=this->begin; p!=this->end; elem++, p+=this->numDimensions)
    // {
    //     if(fileNames[elem] == str)
    //         selSet.insert(elem);
    // }
    selectSet(selSet, group);
}

// struct indexedValueLtFunctor
// {
//     indexedValueLtFunctor(const struct indexedValue _it) : it(_it) {}
//
//     struct indexedValue it;
//
//     bool operator()(const struct indexedValue &other)
//     {
//         return it.val < other.val;
//     }
// };

void DataObject::selectByLineRange(qreal vmin, qreal vmax, int group)
{
    ElemSet selSet;
    for(Sample sample : samples)
    {
        if(sample.line >= vmin && sample.line < vmax)
            selSet.insert(sample.sampleId);
    }
    std::cerr << "selected by line range: " << selSet.size()<< " vmin: " << vmin << " vmax: " << vmax << std::endl;
    selectSet(selSet,group);
}

// void DataObject::selectByDimRange(int dim, qreal vmin, qreal vmax, int group)
// {
//     ElemSet selSet;
//     std::vector<indexedValue>::iterator itMin;
//     std::vector<indexedValue>::iterator itMax;
//
//     struct indexedValue ivMinQuery;
//     ivMinQuery.val = vmin;
//
//     struct indexedValue ivMaxQuery;
//     ivMaxQuery.val = vmax;
//
//     if(ivMinQuery.val <= this->minimumValues[dim])
//     {
//         itMin = dimSortedLists.at(dim).begin();
//     }
//     else
//     {
//         itMin = std::find_if(dimSortedLists.at(dim).begin(),
//                              dimSortedLists.at(dim).end(),
//                              indexedValueLtFunctor(ivMinQuery));
//     }
//
//     if(ivMaxQuery.val >= this->maximumValues[dim])
//     {
//         itMax = dimSortedLists.at(dim).end();
//     }
//     else
//     {
//         itMax = std::find_if(dimSortedLists.at(dim).begin(),
//                              dimSortedLists.at(dim).end(),
//                              indexedValueLtFunctor(ivMaxQuery));
//     }
//
//     for(/*itMin*/; itMin != itMax; itMin++)
//     {
//         selSet.insert(itMin->idx);
//     }
//
//     selectSet(selSet,group);
// }

void DataObject::selectByMultiDimRange(QVector<int> dims, QVector<qreal> mins, QVector<qreal> maxes, int group)
{
    ElemSet selSet;

    std::cerr << "selecting data: \n";
    for(int i = 0; i < dims.size(); i++){
        std::cerr <<"axis: " << dims[i] << " starting at: " << mins[i] << " up to: " << maxes[i] << std::endl;
    }

    for(int s = 0; s < numSamples; s++){
        bool partOf = true;
        for(int i = 0; i < dims.size(); i++){
            if(GetSampleAttribByIndex(s, dims[i]) > maxes[i] || GetSampleAttribByIndex(s, dims[i]) < mins[i]){
                partOf = false;
                break;
            }
        }
        if(partOf)selSet.insert(s);
    }
    std::cerr << selSet.size() << " elements of select set\n";
    selectSet(selSet);
}

void DataObject::selectByVarName(QString str, int group)
{
    ElemSet selSet;

    // ElemIndex elem;
    // QVector<qreal>::Iterator p;
    // for(elem=0, p=this->begin; p!=this->end; elem++, p+=this->numDimensions)
    // {
    //     if(varNames[elem] == str)
    //         selSet.insert(elem);
    // }

    for(Sample sample: samples)
    {
        if(sample.variable == str)
            selSet.insert(sample.sampleId);
    }
    selectSet(selSet,group);
}

void DataObject::selectByResource(Component *c, int group)
{
    ElemSet resource_samples;
    vector<DataPath*> dp_vec;
    c->GetAllDpByType(&dp_vec, SYS_SAGE_MITOS_SAMPLE, SYS_SAGE_DATAPATH_INCOMING | SYS_SAGE_DATAPATH_OUTGOING);
    for(DataPath* dp : dp_vec) {
        resource_samples.insert(((SampleSet*)dp->attrib["sample_set"])->totSamples.begin(), ((SampleSet*)dp->attrib["sample_set"])->totSamples.end());
    }
    selectSet(resource_samples, group);
    //selectSet( (*((QMap<DataObject*,SampleSet>*)c->attrib["sampleSets"]))[this].totSamples, group);
    //selectSet(node->sampleSets[this].totSamples,group);
}

void DataObject::hideSelected()
{
    ElemIndex elem;
    for(elem=0; elem<numElements; elem++)
    {
        if(selected(elem))
            hideData(elem);
    }
}

void DataObject::hideUnselected()
{
    ElemIndex elem;
    for(elem=0; elem<numElements; elem++)
    {
        if(!selected(elem) && visible(elem))
            hideData(elem);
    }
}

void DataObject::selectSet(ElemSet &s, int group)
{
    ElemSet *newSel = NULL;
    if(selMode == MODE_NEW)
    {
        newSel = &s;
    }
    else if(selMode == MODE_APPEND)
    {
        newSel = new ElemSet();
        std::set_union(selectionSets.at(group).begin(),
                       selectionSets.at(group).end(),
                       s.begin(),
                       s.end(),
                       std::inserter(*newSel,
                                     newSel->begin()));
    }
    else if(selMode == MODE_FILTER)
    {
        newSel = new ElemSet();
        std::set_intersection(selectionSets.at(group).begin(),
                              selectionSets.at(group).end(),
                              s.begin(),
                              s.end(),
                              std::inserter(*newSel,
                                            newSel->begin()));
    }
    else
    {
        std::cerr << "MANATEES! EVERYWHERE!" << std::endl;
        return;
    }

    // Reprocess groups
    selectionSets.at(group).clear();
    selectionGroup.fill(0);

    for(ElemSet::iterator it = newSel->begin();
        it != newSel->end();
        it++)
    {
        selectData(*it,group);
    }
}

void DataObject::collectTopoSamples()
{
    vector<Component*> allComponents;
    node->GetSubtreeNodeList(&allComponents);
    for(Component* c : allComponents)
    {
        *(int*)c->attrib["transactions"] = 0;
        //count incoming DP on a thread (all DPs point to a thread)
        if(c->GetComponentType() == SYS_SAGE_COMPONENT_THREAD)
        {
            vector<DataPath*> dp_in_vec;
            c->GetAllDpByType(&dp_in_vec, SYS_SAGE_MITOS_SAMPLE, SYS_SAGE_DATAPATH_INCOMING);
            if(dp_in_vec.empty())
            {
                qDebug( "No samples on HW thread %d", c->GetId());
                continue;
            }
            for(DataPath* dp_in : dp_in_vec)
            {
                SampleSet *ss = (SampleSet*)dp_in->attrib["sample_set"];
                ss->selSamples.clear();
                ss->selCycles = 0;

                for(ElemIndex elemid : ss->totSamples)
                {
                    if(!selectionDefined() || selected(elemid))
                    {
                        ss->selSamples.insert(elemid);
                        ss->selCycles += samples[elemid].latency;
                    }
                }
            }
        }
    }
    for(Component* c : allComponents)
    {
        if(c->GetComponentType() == SYS_SAGE_COMPONENT_THREAD)
        {
            vector<DataPath*> dp_in_vec;
            c->GetAllDpByType(&dp_in_vec, SYS_SAGE_MITOS_SAMPLE, SYS_SAGE_DATAPATH_INCOMING);
            for(DataPath* dp_in : dp_in_vec){
                //add the number of samples to this thread and then to all parent nodes until the source
                Component* parent = c;
                do {
                    *(int*)parent->attrib["transactions"] += ((SampleSet*)dp_in->attrib["sample_set"])->selSamples.size();
                    parent = parent->GetParent();
                } while(parent != dp_in->GetSource() && parent != NULL && parent->GetComponentType() != SYS_SAGE_COMPONENT_CHIP);
            }
        }
    }

    // // Reset info
    // vector<Component*> allComponents;
    // cpu->GetSubtreeNodeList(&allComponents);
    // qDebug( "cpu [%d] = %p", cpu->GetComponentType(), cpu);
    //
    // map<int,Thread*> threadIdMap;
    // for(int i=0; i<allComponents.size(); i++)
    // {
    //     if(allComponents[i]->GetComponentType() == SYS_SAGE_COMPONENT_THREAD)
    //     {
    //         threadIdMap[allComponents[i]->GetId()] = (Thread*)allComponents[i];
    //         //qDebug( "threadIdMap [%d] = %p", allComponents[i]->GetId(), (Thread*)allComponents[i]);
    //     }
    // }
    //
    // for(int i=0; i<allComponents.size(); i++)
    // {
    //     //qDebug( "collectTopoSamplesxx = %p", allComponents[i]->attrib["sampleSets"]);
    //     QMap<DataObject*,SampleSet>* sampleSets = (QMap<DataObject*,SampleSet>*)allComponents[i]->attrib["sampleSets"];
    //     (*sampleSets)[this].totCycles = 0;
    //     (*sampleSets)[this].selCycles = 0;
    //     (*sampleSets)[this].totSamples.clear();
    //     (*sampleSets)[this].selSamples.clear();
    //     *(int*)(allComponents[i]->attrib["transactions"]) = 0;
    // }
    // qDebug( "collectTopoSamples2");
    //
    // // Go through each sample and add it to the right topo node
    // ElemIndex elem;
    // QVector<qreal>::Iterator p;
    // int i = 0;
    // int j=0;
    //
    // std::chrono::high_resolution_clock::time_point t_start, t_end;
    // t_start = std::chrono::high_resolution_clock::now();
    // for(elem=0, p=this->begin; p!=this->end; elem++, p+=this->numDimensions)
    // {
    //     //qDebug( "collectTopoSamples loop2 %d", elem);
    //
    //     // Get vars
    //     int dse = *(p+dataSourceDim);
    //     int cpu = *(p+cpuDim);
    //     int cycles = *(p+latencyDim);
    //
    //
    //     // Search for nodes
    //     Thread* t = threadIdMap[cpu];
    //
    //     // Update data for serving resource
    //     QMap<DataObject*,SampleSet>*sampleSets = (QMap<DataObject*,SampleSet>*)t->attrib["sampleSets"];
    //     //qDebug( "--%llu collectTopoSamples cpu= %d cpuIndex= %d samples: %d cycles %d dse: %d", elem, cpu, cpuDim, (*sampleSets)[this].totSamples.size(), (*sampleSets)[this].totCycles, dse );
    //     (*sampleSets)[this].totSamples.insert(elem);
    //     (*sampleSets)[this].totCycles += cycles;
    //
    //     if(!selectionDefined() || selected(elem))
    //     {
    //         (*sampleSets)[this].selSamples.insert(elem);
    //         (*sampleSets)[this].selCycles += cycles;
    //     }
    //
    //     if(dse == -1)
    //         continue;
    //     //dse== 1 ->L1 Cache
    //     //      2 ->L2
    //     //      3 ->L3
    //     //      4 ->main mem.
    //
    //     // Go up to data source
    //     Component* c;
    //     for(c = (Component*)t; c->GetParent() && c->GetComponentType() != SYS_SAGE_COMPONENT_CHIP ; c=c->GetParent())
    //     {
    //         if(!selectionDefined() || selected(elem))
    //         {
    //             *(int*)(c->attrib["transactions"])+= 1;
    //             //qDebug( "---%llu chip: %d  transactions: %d dse %d", elem, c->GetComponentType(), *(int*)(c->attrib["transactions"]), dse);
    //         }
    //
    //         volatile int type = c->GetComponentType();
    //
    //         if((type == SYS_SAGE_COMPONENT_CACHE && ((Cache*)c)->GetCacheLevel() == 1 && dse == 1) ||
    //                 (type == SYS_SAGE_COMPONENT_CACHE && ((Cache*)c)->GetCacheLevel() == 2 && dse == 2) ||
    //                 (type == SYS_SAGE_COMPONENT_CACHE && ((Cache*)c)->GetCacheLevel() == 3 && dse == 3) ||
    //                 (type == SYS_SAGE_COMPONENT_NUMA && dse == 4))
    //         {
    //             break;
    //         }
    //     }
    //
    //     // Update data for core
    //     sampleSets = (QMap<DataObject*,SampleSet>*)c->attrib["sampleSets"];
    //     (*sampleSets)[this].totSamples.insert(elem);
    //     (*sampleSets)[this].totCycles += cycles;
    //
    //     i++;
    //     if(!selectionDefined() || selected(elem))
    //     {
    //         (*sampleSets)[this].selSamples.insert(elem);
    //         (*sampleSets)[this].selCycles += cycles;
    //
    //         j++;
    //     }
    // }
    // t_end = std::chrono::high_resolution_clock::now();
    // qDebug("totSamples; %d; selSamples; %d; time; %llu", i, j, t_end.time_since_epoch().count()-t_start.time_since_epoch().count());
    //
    // qDebug( "collectTopoSamples3");
}

int DataObject::DecodeDataSource(QString data_src_str)
{
    if(data_src_str == "L1")
        return 1;
    else if(data_src_str == "LFB")
        return 1;
    else if(data_src_str == "L2")
        return 2;
    else if(data_src_str == "L3")
        return 3;
    else if(data_src_str == "Local RAM")
        return 4;
    else if(data_src_str == "Remote RAM 1 Hop")
        return 4;
    else if(data_src_str == "Remote RAM 2 Hops")
        return 4;
    else if(data_src_str == "Remote Cache 1 Hops")
        return 3;
    else if(data_src_str == "Remote Cache 2 Hops")
        return 3;
    else if(data_src_str == "I/O Memory")
        return 4;
    else if(data_src_str == "Uncached Memory")
        return 4;
    return -1;
}

void DataObject::setIBSBaseLatency(int baseLatency){
    ibsBaseLatency = baseLatency;
}

QString DataObject::titleOfColumn(int index){
    return header.at(index);
}

int DataObject::numberOfColumns(){
    return header.size();
}


int DataObject::parseCSVFile(QString dataFileName)
{

    // Open the file
    QFile dataFile(dataFileName);

    

    if (!dataFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return -1;

    // Create text stream
    QTextStream dataStream(&dataFile);
    QString line;
    qint64 elemid = 0;

    // Get metadata from first line
    line = dataStream.readLine();
    // this->meta = line.split(',');
    // this->numDimensions = this->meta.size();
    //
    // sourceDim = this->meta.indexOf("source");
    // lineDim = this->meta.indexOf("line");
    // variableDim = this->meta.indexOf("variable");
    // dataSourceDim = this->meta.indexOf("data_src");
    // indexDim = this->meta.indexOf("index");
    // latencyDim = this->meta.indexOf("latency");
    // nodeDim = this->meta.indexOf("node");
    // cpuDim = this->meta.indexOf("cpu");
    // xDim = this->meta.indexOf("xidx");
    // yDim = this->meta.indexOf("yidx");
    // zDim = this->meta.indexOf("zidx");

    //adapted from https://stackoverflow.com/questions/3482064/counting-the-number-of-lines-in-a-text-file
    numSamples = 0;
    std::string stringline = "";
    std::ifstream lengthReader(dataFileName.toStdString());

    while (std::getline(lengthReader, stringline))
        if(stringline != "")++numSamples;
    //subtract header line
    numSamples--;



    QVector<QString> varVec;
    QVector<QString> sourceVec;
    header = line.split(',');

    //allocate sample matrix
    sampleMatrix = (long long *)malloc(sizeof(long long) * header.length() * numSamples);
    
    //Get attribute names
    for(int i = 0; i < header.length(); i++){
        attributeNames.push_back(header[i].toStdString());
    }

    for(int i = 0; i < SampleAxes::SampleAxesNames.length() && i < attributeNames.size(); i++){
        attributeNames[i] = SampleAxes::SampleAxesNames[i].toStdString();
    }

    QVector<QStringList> lines;

    ElemIndex numHeaderDimensions = header.size();
    numAttributes = header.size();

    int builtinColumns[SampleAxes::BuiltinLoads.size()];
    for(int i = 0; i < SampleAxes::BuiltinLoads.size(); i++){
        builtinColumns[i] = header.indexOf(SampleAxes::BuiltinLoads[i]);
        QString last = SampleAxes::BuiltinLoads[i];
        while(builtinColumns[i] < 0 || builtinColumns[i] >= header.length()){
            bool ok = true;
            QString input = QInputDialog::getText(QApplication::activeWindow(), "Column unavailable", "no axis with name " + last + " specify alternative axis index or name", QLineEdit::Normal, "type here", &ok);
            if(!ok)builtinColumns[i] = header.size() - 1;
            else {
                builtinColumns[i] = input.toInt(&ok);
                if(!ok)builtinColumns[i] = header.indexOf(input);
            }
        }
    }

    milliseconds msStart = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

    // Get data
    while(!dataStream.atEnd())
    {
        line = dataStream.readLine();
        lines.push_back(line.split(','));
    }

    // Close and return
    dataFile.close();

    //parse data
    for(QStringList lineValues : lines){
        if(lineValues.size() != numHeaderDimensions)
        {
            std::cerr << "ERROR: element dimensions do not match headerdata!" << std::endl;
            std::cerr << "At element " << elemid << std::endl;
            return -1;
        }

        Sample s;
        s.sampleId = elemid;
        s.sourceUid = createUniqueID(sourceVec,lineValues[builtinColumns[0]]); 
        s.source = lineValues[builtinColumns[0]]; // source file absolute path
        s.line = lineValues[builtinColumns[1]].toLongLong(); // line in source file
        s.instructionUid = createUniqueID(instrVec,lineValues[builtinColumns[2]]);
        s.instruction = lineValues[builtinColumns[2]]; //instruction type
        s.bytes = lineValues[builtinColumns[3]].toLongLong();
        s.ip = lineValues[builtinColumns[4]].toLongLong(); // instruction pointer
        s.variableUid = createUniqueID(varVec,lineValues[builtinColumns[5]]);
        s.variable = lineValues[builtinColumns[5]];
        s.buffer_size = lineValues[builtinColumns[6]].toLongLong();
        s.dims = lineValues[builtinColumns[7]].toInt();
        s.xidx = lineValues[builtinColumns[8]].toInt();
        s.yidx = lineValues[builtinColumns[9]].toInt();
        s.zidx = lineValues[builtinColumns[10]].toInt();
        s.pid = lineValues[builtinColumns[11]].toInt();
        s.tid = lineValues[builtinColumns[12]].toInt();
        s.time = lineValues[builtinColumns[13]].toLongLong();
        s.addr = lineValues[builtinColumns[14]].toLongLong();
        s.cpu = lineValues[builtinColumns[15]].toInt();
        s.latency = lineValues[builtinColumns[16]].toLongLong();
        //s.data_src = dseDepth(lineValues[header.indexOf("data_src")].toInt(NULL,10));
        
        s.data_src = DecodeDataSource(lineValues[builtinColumns[17]]);
        
        //IBS data src        
        if(s.data_src == -1 && header.indexOf("ibs_dc_miss") >= 0 && header.indexOf("ibs_l2_miss") >= 0){
            if(!lineValues[header.indexOf("ibs_dc_miss")].toInt() == 1){
                s.data_src = 1; // L1 hit
            }else if(!lineValues[header.indexOf("ibs_l2_miss")].toInt() == 1){
                s.data_src = 2; // L2 hit
            }else s.data_src = 4; // something else happened
        }
        
        if(header.indexOf("ibs_op_phy") >= 0){
            s.addr = std::stoll(lineValues[header.indexOf("ibs_op_phy")].toStdString(), nullptr, 16);
            //std::cerr << s.addr << std::endl;
        }

        if(header.indexOf("ibs_dc_miss_lat") >= 0){
            s.latency = lineValues[header.indexOf("ibs_dc_miss_lat")].toLongLong() + ibsBaseLatency;
        }

        //write MemAxes standard info to matrix
        for(int i = 0; i < 19 && i < header.length(); i++){
            sampleMatrix[i * numSamples + elemid] = GetSampleAttribByIndex(&s, i);
        }

        //ibs and additional info to matrix
        for(int i = 19; i < header.length(); i++){
            long long r = 0;
            bool ok = true;
            if(lineValues[i] != "(null)"){
                r = lineValues[i].toLongLong(&ok);

                if(!ok){
                    r = lineValues[i].toLongLong(&ok, 16);
                }

                if(!ok)r = 0;
            }
            sampleMatrix[i*numSamples + elemid] = r;
        }


        
        

        s.visible = VISIBLE;
        samples.push_back(s);

        //add samples as DataPath pointers
        Component * compTarget = node->FindSubcomponentById(s.cpu, SYS_SAGE_COMPONENT_THREAD);
        Component * compSrc = compTarget;
        //connect with the right memory/cache
        while(compSrc != NULL && s.data_src != -1){
            compSrc = compSrc->GetParent();
            if(s.data_src == 1
                && compSrc->GetComponentType() == SYS_SAGE_COMPONENT_CACHE
                && ((Cache*)compSrc)->GetCacheLevel()==1) {
                        //qDebug("creating L1 cache datapath");
                        break;//L1
                    }
            else if(s.data_src == 2
                && compSrc->GetComponentType() == SYS_SAGE_COMPONENT_CACHE
                && ((Cache*)compSrc)->GetCacheLevel()==2) break;//L2
            else if(s.data_src == 3
                && compSrc->GetComponentType() == SYS_SAGE_COMPONENT_CACHE
                && ((Cache*)compSrc)->GetCacheLevel()==3) break;//L3
            else if(s.data_src == 4
                && (compSrc->GetComponentType() == SYS_SAGE_COMPONENT_NUMA
                || compSrc->GetComponentType() == SYS_SAGE_COMPONENT_CHIP)) break;//main memory
        }
        if(compSrc == NULL || compTarget == NULL)
        {
            qDebug( "Source or target component not found (cpu %d %p data source %d %p)", s.cpu, compTarget, s.data_src, compSrc);
        }
        else
        {
            vector<DataPath*> dp_vec;
            compTarget->GetAllDpByType(&dp_vec, SYS_SAGE_MITOS_SAMPLE, SYS_SAGE_DATAPATH_INCOMING);
            bool dp_exists = false;
            DataPath* dp;
            for(DataPath* dp_iter : dp_vec){
                if(dp_iter->GetSource() == compSrc){
                    dp = dp_iter;
                    dp_exists = true;
                    break;
                }
            }
            if(!dp_exists){ //no sample connecting the two components
                dp = NewDataPath(compSrc, compTarget, SYS_SAGE_DATAPATH_ORIENTED, SYS_SAGE_MITOS_SAMPLE);
                dp->attrib["samples"] = (void*)new vector<Sample*>();
                dp->attrib["sel_samples"] = (void*)new vector<Sample*>();
                dp->attrib["sample_set"] = (void*)new SampleSet();
                SampleSet *ss = (SampleSet*)dp->attrib["sample_set"];
                ss->totCycles = 0;
                ss->selCycles = 0;
                ss->totSamples.clear();
                ss->selSamples.clear();
            }
            ((vector<Sample*>*)(dp->attrib["samples"]))->push_back(&(samples[elemid]));
            ((vector<Sample*>*)(dp->attrib["sel_samples"]))->push_back(&(samples[elemid]));
            SampleSet *ss = (SampleSet*)dp->attrib["sample_set"];
            ss->totCycles += s.latency;
            ss->selCycles += s.latency;
            ss->totSamples.insert(elemid);
            ss->selSamples.insert(elemid);
        }
        elemid++;



        //
        //
        // // Process individual dimensions differently
        // for(int i=0; i<lineValues.size(); i++)
        // {
        //     QString tok = lineValues[i];
        //     if(i==variableDim)
        //     {
        //         ElemIndex uid = createUniqueID(varVec,tok);
        //         this->vals.push_back(uid);
        //         varNames.push_back(tok);
        //     }
        //     else if(i==sourceDim)
        //     {
        //         ElemIndex uid = createUniqueID(sourceVec,tok);
        //         this->vals.push_back(uid);
        //         fileNames.push_back(tok);
        //     }
        //     else if(i==dataSourceDim)
        //     {
        //         int dseVal = tok.toInt(NULL,10);
        //         int dse = dseDepth(dseVal);
        //         // qDebug("elem %d dseVal %d dse %d", elemid, dseVal, dse);
        //         this->vals.push_back(dse);
        //     }
        //     else
        //     {
        //         this->vals.push_back(tok.toLongLong());
        //     }
        // }

    }

    this->allocate();

    milliseconds msEnd = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    auto elapsedTotal = msEnd - msStart;
    std::cerr << "parsing CSV file took " << elapsedTotal.count() << "\n";

    return 0;
}

void DataObject::setSelectionMode(selection_mode mode, bool silent)
{
    selMode = mode;

    if(silent)
        return;

    QString selcmd("select MODE=");
    switch(mode)
    {
        case(MODE_NEW):
            selcmd += "filter";
            break;
        case(MODE_APPEND):
            selcmd += "append";
            break;
        case(MODE_FILTER):
            selcmd += "filter";
            break;
    }
    con->log(selcmd);


    
}

long long DataObject::GetSampleAttribByIndex(Sample* s, int attrib_idx)
{
    switch((SampleAxes::SampleAxes)attrib_idx){
        case SampleAxes::sampleId://0
            return s->sampleId;
        case SampleAxes::sourceUid://1
            return s->sourceUid;
        case SampleAxes::line://2
            return s->line;
        case SampleAxes::instructionUid://3
            return s->instructionUid;
        case SampleAxes::bytes: //4
            return s->bytes;
        case SampleAxes::ip: //5
            return s->ip;
        case SampleAxes::variableUid://6
            return s->variableUid;
        case SampleAxes::buffer_size: //7
            return s->buffer_size;
        case SampleAxes::dims: //8
            return s->dims;
        case SampleAxes::xidx://9
            return s->xidx;
        case SampleAxes::yidx://10
            return s->yidx;
        case SampleAxes::zidx://11
            return s->zidx;
        case SampleAxes::pid: //12
            return s->pid;
        case SampleAxes::tid: //13
            return s->tid;
        case SampleAxes::time: //14
            return s->time;
        case SampleAxes::addr: //15
            return s->addr;
        case SampleAxes::cpu://16
            return s->cpu;
        case SampleAxes::latency://17
            return s->latency;
        case SampleAxes::dataSrc://18
            return s->data_src;
        default:
            return -999999999;
    }
}

long long DataObject::GetSampleAttribByIndex(int sampleId, int attrib_idx)
{

    if(sampleId >= numSamples || sampleId < 0 || attrib_idx < 0 || attrib_idx > numAttributes)
        return -999999999;
    
    /*
    Sample s = samples[sampleId];
    return GetSampleAttribByIndex(&s,attrib_idx);
    */
    return sampleMatrix[attrib_idx * numSamples + sampleId];
}

void DataObject::calcStatistics()
{
    sample_sums.resize(NUM_SAMPLE_AXES);
    sample_mins.resize(NUM_SAMPLE_AXES);
    sample_maxes.resize(NUM_SAMPLE_AXES);
    sample_means.resize(NUM_SAMPLE_AXES);
    sample_stdevs.resize(NUM_SAMPLE_AXES);

    sample_sums.fill(0);
    sample_mins.fill(9999999999);
    sample_maxes.fill(0);
    sample_means.fill(0);
    sample_stdevs.fill(0);

    // dimSums.resize(this->numDimensions);
    // minimumValues.resize(this->numDimensions);
    // maximumValues.resize(this->numDimensions);
    // meanValues.resize(this->numDimensions);
    // standardDeviations.resize(this->numDimensions);
    //
    // covarianceMatrix.resize(this->numDimensions*this->numDimensions);
    // correlationMatrix.resize(this->numDimensions*this->numDimensions);
    //
    // qreal firstVal = *(this->begin);
    // dimSums.fill(0);
    // minimumValues.fill(9999999999);
    // maximumValues.fill(0);
    // meanValues.fill(0);
    // standardDeviations.fill(0);
    //
    // // Means and combined means
    // QVector<qreal>::Iterator p;
    // ElemIndex elem;
    // qreal x, y;

    for(Sample s : samples)
    {
        for(int i=0; i<NUM_SAMPLE_AXES; i++)
        {
            long long val = GetSampleAttribByIndex(&s, i);
            sample_sums[i] += val;
            sample_mins[i] = std::min((qreal)val,sample_mins[i]);
            sample_maxes[i] = std::max((qreal)val,sample_maxes[i]);
        }
    }

    int numSamples = samples.size();
    for(int i=0; i<NUM_SAMPLE_AXES; i++)
    {
        sample_means[i] = sample_sums[i]/numSamples;
    }

    for(Sample s: samples)
    {
        for(int i=0; i<NUM_SAMPLE_AXES; i++)
        {
            long long val = GetSampleAttribByIndex(&s, i);
            sample_stdevs[i] += (val-sample_means[i])*(val-sample_means[i]);
        }
    }

    for(int i=0; i<NUM_SAMPLE_AXES; i++)
    {
        sample_stdevs[i] = sqrt(sample_stdevs[i]/numSamples);
    }

    //TODO this part was not refactored

    // QVector<qreal> meanXY;
    // meanXY.resize(this->numDimensions*this->numDimensions);
    // meanXY.fill(0);
    // for(elem=0, p=this->begin; p!=this->end; elem++, p+=this->numDimensions)
    // {
    //     for(int i=0; i<this->numDimensions; i++)
    //     {
    //         x = *(p+i);
    //         dimSums[i] += x;
    //         minimumValues[i] = std::min(x,minimumValues[i]);
    //         maximumValues[i] = std::max(x,maximumValues[i]);
    //
    //         for(int j=0; j<this->numDimensions; j++)
    //         {
    //             y = *(p+j);
    //             meanXY[ROWMAJOR_2D(i,j,this->numDimensions)] += x*y;
    //         }
    //     }
    // }
    //
    // // Divide by this->numElements to get mean
    // for(int i=0; i<this->numDimensions; i++)
    // {
    //     meanValues[i] = dimSums[i] / (qreal)this->numElements;
    //     for(int j=0; j<this->numDimensions; j++)
    //     {
    //         meanXY[ROWMAJOR_2D(i,j,this->numDimensions)] /= (qreal)this->numElements;
    //     }
    // }
    //
    // // Covariance = E(XY) - E(X)*E(Y)
    // for(int i=0; i<this->numDimensions; i++)
    // {
    //     for(int j=0; j<this->numDimensions; j++)
    //     {
    //         covarianceMatrix[ROWMAJOR_2D(i,j,this->numDimensions)] =
    //             meanXY[ROWMAJOR_2D(i,j,this->numDimensions)] - meanValues[i]*meanValues[j];
    //     }
    // }
    //
    // // Standard deviation of each dim
    // for(elem=0, p=this->begin; p!=this->end; elem++, p+=this->numDimensions)
    // {
    //     for(int i=0; i<this->numDimensions; i++)
    //     {
    //         x = *(p+i);
    //         standardDeviations[i] += (x-meanValues[i])*(x-meanValues[i]);
    //     }
    // }
    //
    // for(int i=0; i<this->numDimensions; i++)
    // {
    //     standardDeviations[i] = sqrt(standardDeviations[i]/(qreal)this->numElements);
    // }
    //
    // // Correlation Coeff = cov(xy) / stdev(x)*stdev(y)
    // for(int i=0; i<this->numDimensions; i++)
    // {
    //     for(int j=0; j<this->numDimensions; j++)
    //     {
    //         correlationMatrix[ROWMAJOR_2D(i,j,this->numDimensions)] =
    //                 covarianceMatrix[ROWMAJOR_2D(i,j,this->numDimensions)] /
    //                 (standardDeviations[i]*standardDeviations[j]);
    //     }
    // }
}

// void DataObject::constructSortedLists()
// {
//     dimSortedLists.resize(this->numDimensions);
//     for(int d=0; d<this->numDimensions; d++)
//     {
//         for(int e=0; e<this->numElements; e++)
//         {
//             struct indexedValue di;
//             di.idx = e;
//             di.val = at(e,d);
//             dimSortedLists.at(d).push_back(di);
//         }
//         std::sort(dimSortedLists.at(d).begin(),dimSortedLists.at(d).end());
//     }
// }

qreal distanceHardware(DataObject *d, ElemSet *s1, ElemSet *s2)
{

    int nodeTopoDepth = d->node->GetTopoTreeDepth()+1;
    int dseDepth;

    // Vars
    std::vector<qreal> t1,t2;
    std::vector<qreal> t1means,t2means;
    std::vector<qreal> t1stddev,t2stddev;

    t1.resize(nodeTopoDepth,0);
    t2.resize(nodeTopoDepth,0);
    t1means.resize(nodeTopoDepth,0);
    t1stddev.resize(nodeTopoDepth,0);
    t2means.resize(nodeTopoDepth,0);
    t2stddev.resize(nodeTopoDepth,0);

    qreal n1 = s1->size();
    qreal n2 = s2->size();

    ElemSet::iterator it;
    qreal lat;

    // Collect s1 topo data
    for(it = s1->begin(); it != s1->end(); it++)
    {
        lat = d->samples[*it].latency;
        dseDepth = d->samples[*it].data_src;
        // lat = d->at(*it,d->latencyDim);
        // dseDepth = d->at(*it,d->dataSourceDim);
        t1[nodeTopoDepth] += lat;
        t1[dseDepth] += dseDepth;
    }

    // Collect s2 topo data
    for(it = s2->begin(); it != s2->end(); it++)
    {
        lat = d->samples[*it].latency;
        dseDepth = d->samples[*it].data_src;
        // lat = d->at(*it,d->latencyDim);
        // dseDepth = d->at(*it,d->dataSourceDim);
        t2[nodeTopoDepth] += lat;
        t2[dseDepth] += dseDepth;
    }

    // Compute means
    for(int i=0; i<nodeTopoDepth; i++)
    {
        t1means[i] = t1.at(i) / n1;
        t2means[i] = t2.at(i) / n2;
    }

    // Compute standard deviations
    for(it = s1->begin(); it != s1->end(); it++)
    {
        lat = d->samples[*it].latency;
        dseDepth = d->samples[*it].data_src;
        // lat = d->at(*it,d->latencyDim);
        // dseDepth = d->at(*it,d->dataSourceDim);
        t1stddev[dseDepth] += (lat-t1means.at(dseDepth))*(lat-t1means.at(dseDepth));
    }
    for(it = s2->begin(); it != s2->end(); it++)
    {
        lat = d->samples[*it].latency;
        dseDepth = d->samples[*it].data_src;
        // lat = d->at(*it,d->latencyDim);
        // dseDepth = d->at(*it,d->dataSourceDim);
        t2stddev[dseDepth] += (lat-t2means.at(dseDepth))*(lat-t2means.at(dseDepth));
    }
    for(int i=0; i<nodeTopoDepth; i++)
    {
        t1stddev[i] /= n1;
        t2stddev[i] /= n2;
    }

    // Euclidean length of means vector
    qreal dist = 0;
    for(int i=0; i<nodeTopoDepth; i++)
    {
        dist += (t1means[i]+t2means[i])*(t1means[i]+t2means[i]);
    }
    dist = sqrt(dist);

    return dist;
}

int DataObject::getNumberOfSamples(){
    return numSamples;
}

int DataObject::getNumberOfAttributes(){
    return attributeNames.size();
}

string DataObject::GetAttributeName(int index){
    return attributeNames[index];
}

long long * DataObject::GetSampleMatrix(){
    return sampleMatrix;
}

QString DataObject::getInstruction(int instructionUID){
    return instrVec[instructionUID];
}

