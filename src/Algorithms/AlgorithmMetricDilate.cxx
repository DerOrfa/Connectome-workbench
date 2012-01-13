/*LICENSE_START*/
/*
 *  Copyright 1995-2002 Washington University School of Medicine
 *
 *  http://brainmap.wustl.edu
 *
 *  This file is part of CARET.
 *
 *  CARET is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  CARET is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CARET; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "AlgorithmMetricDilate.h"
#include "AlgorithmException.h"
#include "SurfaceFile.h"
#include "MetricFile.h"

using namespace caret;
using namespace std;

AString AlgorithmMetricDilate::getCommandSwitch()
{
    return "-metric-dilate";
}

AString AlgorithmMetricDilate::getShortDescription()
{
    return "DILATE A METRIC FILE";
}

OperationParameters* AlgorithmMetricDilate::getParameters()
{
    OperationParameters* ret = new OperationParameters();
    ret->addMetricParameter(1, "metric", "the metric to dilate");
    
    ret->addSurfaceParameter(2, "surface", "the surface to compute on");
    
    ret->addMetricOutputParameter(3, "metric-out", "the output metric");
    
    ret->addDoubleParameter(4, "distance", "distance in mm to dilate");
    
    OptionalParameter* roiOpt = ret->createOptionalParameter(5, "-bad-node-roi", "specify an roi of nodes to overwrite, rather than nodes with value zero");
    roiOpt->addMetricParameter(1, "roi-metric", "metric file, all positive values denote nodes to have their values replaced");
    
    OptionalParameter* columnSelect = ret->createOptionalParameter(6, "-column", "select a single column to dilate");
    columnSelect->addIntegerParameter(1, "column-number", "the column number");
    
    ret->setHelpText(
        AString("For all metric nodes that are designated as bad, if they neighbor a good node or are within the specified distance of a good node, ") +
        "replace the value with a distance weighted average of nearby good nodes, otherwise set the value to zero.  If -bad-node-roi is specified, " +
        "all nodes, including those with value zero, are good, except for nodes with a positive value in the ROI.  If it is not specified, " +
        "only nodes with value zero are bad."
    );
    return ret;
}

void AlgorithmMetricDilate::useParameters(OperationParameters* myParams, ProgressObject* myProgObj)
{
    MetricFile* myMetric = myParams->getMetric(1);
    SurfaceFile* mySurf = myParams->getSurface(2);
    MetricFile* myMetricOut = myParams->getOutputMetric(3);
    float distance = (float)myParams->getDouble(4);
    OptionalParameter* roiOpt = myParams->getOptionalParameter(5);
    MetricFile* badNodeRoi = NULL;
    if (roiOpt->m_present)
    {
        badNodeRoi = roiOpt->getMetric(1);
    }
    OptionalParameter* columnSelect = myParams->getOptionalParameter(6);
    int columnNum = -1;
    if (columnSelect->m_present)
    {//set up to use the single column
        columnNum = (int)columnSelect->getInteger(1);//get the integer argument to the option
        if (columnNum < 0)
        {
            throw AlgorithmException("invalid column specified");
        }
    }
    AlgorithmMetricDilate(myProgObj, myMetric, mySurf, myMetricOut, distance, badNodeRoi, columnNum);//executes the algorithm
}

AlgorithmMetricDilate::AlgorithmMetricDilate(ProgressObject* myProgObj, const MetricFile* myMetric, const SurfaceFile* mySurf, MetricFile* myMetricOut, float distance, const MetricFile* badNodeRoi, int columnNum) : AbstractAlgorithm(myProgObj)
{
    LevelProgress myProgress(myProgObj);
    int numNodes = mySurf->getNumberOfNodes();
    if (numNodes != myMetric->getNumberOfNodes())
    {
        throw AlgorithmException("surface and metric number of nodes do not match");
    }
    if (badNodeRoi != NULL && badNodeRoi->getNumberOfNodes() != numNodes)
    {
        throw AlgorithmException("bad node roi number of nodes does not match");
    }
    if (columnNum < -1 || columnNum >= myMetric->getNumberOfColumns())
    {
        throw AlgorithmException("invalid column specified");
    }
    if (distance < 0.0f)
    {
        throw AlgorithmException("invalid distance specified");
    }
    CaretArray<float> colScratch(numNodes);
    vector<float> myAreas;
    CaretArray<int> markArray(numNodes);
    mySurf->computeNodeAreas(myAreas);
    if (columnNum == -1)
    {
        myMetricOut->setNumberOfNodesAndColumns(numNodes, myMetric->getNumberOfColumns());
        for (int thisCol = 0; thisCol < myMetric->getNumberOfColumns(); ++thisCol)
        {
            const float* myInputData = myMetric->getValuePointerForColumn(thisCol);
            myMetricOut->setColumnName(thisCol, myMetric->getColumnName(thisCol) + " dilated");
            if (badNodeRoi == NULL)
            {
                for (int i = 0; i < numNodes; ++i)
                {
                    if (myInputData[i] == 0.0f)
                    {
                        markArray[i] = 0;
                    } else {
                        markArray[i] = 1;
                    }
                }
            } else {
                const float* myRoiData = badNodeRoi->getValuePointerForColumn(0);
                for (int i = 0; i < numNodes; ++i)
                {
                    if (myRoiData[i] > 0.0f)
                    {
                        markArray[i] = 0;
                    } else {
                        markArray[i] = 1;
                    }
                }
            }
            CaretPointer<TopologyHelper> myTopoHelp = mySurf->getTopologyHelper();
            CaretPointer<GeodesicHelper> myGeoHelp = mySurf->getGeodesicHelper();
            for (int i = 0; i < numNodes; ++i)
            {
                if (markArray[i] == 0)
                {
                    vector<int32_t> nodeList;
                    vector<float> distList;
                    myGeoHelp->getNodesToGeoDist(i, distance, nodeList, distList);
                    int numInRange = (int)nodeList.size();
                    float totalWeight = 0.0f, weightedSum = 0.0f;
                    for (int j = 0; j < numInRange; ++j)
                    {
                        if (markArray[nodeList[j]] == 1)
                        {
                            float weight;
                            const float tolerance = 0.00001f;
                            if (distList[j] > tolerance)
                            {
                                weight = myAreas[nodeList[j]] / distList[j] / distList[j];
                            } else {
                                weight = myAreas[nodeList[j]] / tolerance / tolerance;
                            }
                            totalWeight += weight;
                            weightedSum += myInputData[nodeList[j]] * weight;
                        }
                    }
                    if (totalWeight != 0.0f)
                    {
                        colScratch[i] = weightedSum / totalWeight;
                    } else {
                        myTopoHelp->getNodeNeighbors(i, nodeList);
                        nodeList.push_back(i);
                        myGeoHelp->getGeoToTheseNodes(i, nodeList, distList);//ok, its a little silly to do this
                        numInRange = (int)nodeList.size();
                        totalWeight = 0.0f;//not really needed, but hey
                        weightedSum = 0.0f;
                        for (int j = 0; j < numInRange; ++j)
                        {
                            if (markArray[nodeList[j]] == 1)
                            {
                                float weight;
                                const float tolerance = 0.00001f;
                                if (distList[j] > tolerance)
                                {
                                    weight = myAreas[nodeList[j]] / distList[j] / distList[j];
                                } else {
                                    weight = myAreas[nodeList[j]] / tolerance / tolerance;
                                }
                                totalWeight += weight;
                                weightedSum += myInputData[nodeList[j]] * weight;
                            }
                        }
                        if (totalWeight != 0.0f)
                        {
                            colScratch[i] = weightedSum / totalWeight;
                        } else {
                            colScratch[i] = 0.0f;
                        }
                    }
                } else {
                    colScratch[i] = myInputData[i];
                }
            }
            myMetricOut->setValuesForColumn(thisCol, colScratch.getArray());
        }
    } else {
        myMetricOut->setNumberOfNodesAndColumns(numNodes, 1);
        const float* myInputData = myMetric->getValuePointerForColumn(columnNum);
        myMetricOut->setColumnName(0, myMetric->getColumnName(columnNum) + " dilated");
        if (badNodeRoi == NULL)
        {
            for (int i = 0; i < numNodes; ++i)
            {
                if (myInputData[i] == 0.0f)
                {
                    markArray[i] = 0;
                } else {
                    markArray[i] = 1;
                }
            }
        } else {
            const float* myRoiData = badNodeRoi->getValuePointerForColumn(0);
            for (int i = 0; i < numNodes; ++i)
            {
                if (myRoiData[i] > 0.0f)
                {
                    markArray[i] = 0;
                } else {
                    markArray[i] = 1;
                }
            }
        }
        CaretPointer<TopologyHelper> myTopoHelp = mySurf->getTopologyHelper();
        CaretPointer<GeodesicHelper> myGeoHelp = mySurf->getGeodesicHelper();
        for (int i = 0; i < numNodes; ++i)
        {
            if (markArray[i] == 0)
            {
                vector<int32_t> nodeList;
                vector<float> distList;
                myGeoHelp->getNodesToGeoDist(i, distance, nodeList, distList);
                int numInRange = (int)nodeList.size();
                float totalWeight = 0.0f, weightedSum = 0.0f;
                for (int j = 0; j < numInRange; ++j)
                {
                    if (markArray[nodeList[j]] == 1)
                    {
                        float weight;
                        const float tolerance = 0.00001f;
                        if (distList[j] > tolerance)
                        {
                            weight = myAreas[nodeList[j]] / distList[j] / distList[j];
                        } else {
                            weight = myAreas[nodeList[j]] / tolerance / tolerance;
                        }
                        totalWeight += weight;
                        weightedSum += myInputData[nodeList[j]] * weight;
                    }
                }
                if (totalWeight != 0.0f)
                {
                    colScratch[i] = weightedSum / totalWeight;
                } else {
                    myTopoHelp->getNodeNeighbors(i, nodeList);
                    nodeList.push_back(i);
                    myGeoHelp->getGeoToTheseNodes(i, nodeList, distList);//ok, its a little silly to do this
                    numInRange = (int)nodeList.size();
                    totalWeight = 0.0f;//not really needed, but hey
                    weightedSum = 0.0f;
                    for (int j = 0; j < numInRange; ++j)
                    {
                        if (markArray[nodeList[j]] == 1)
                        {
                            float weight;
                            const float tolerance = 0.00001f;
                            if (distList[j] > tolerance)
                            {
                                weight = myAreas[nodeList[j]] / distList[j] / distList[j];
                            } else {
                                weight = myAreas[nodeList[j]] / tolerance / tolerance;
                            }
                            totalWeight += weight;
                            weightedSum += myInputData[nodeList[j]] * weight;
                        }
                    }
                    if (totalWeight != 0.0f)
                    {
                        colScratch[i] = weightedSum / totalWeight;
                    } else {
                        colScratch[i] = 0.0f;
                    }
                }
            } else {
                colScratch[i] = myInputData[i];
            }
        }
        myMetricOut->setValuesForColumn(0, colScratch.getArray());
    }
}

float AlgorithmMetricDilate::getAlgorithmInternalWeight()
{
    return 1.0f;//override this if needed, if the progress bar isn't smooth
}

float AlgorithmMetricDilate::getSubAlgorithmWeight()
{
    //return AlgorithmInsertNameHere::getAlgorithmWeight();//if you use a subalgorithm
    return 0.0f;
}
