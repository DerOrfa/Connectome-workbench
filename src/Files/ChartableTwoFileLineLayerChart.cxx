
/*LICENSE_START*/
/*
 *  Copyright (C) 2017 Washington University School of Medicine
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*LICENSE_END*/

#define __CHARTABLE_TWO_FILE_LINE_LAYER_CHART_DECLARE__
#include "ChartableTwoFileLineLayerChart.h"
#undef __CHARTABLE_TWO_FILE_LINE_LAYER_CHART_DECLARE__

#include "BoundingBox.h"
#include "CaretAssert.h"
#include "CaretLogger.h"
#include "ChartTwoDataCartesian.h"
#include "ChartTwoLineSeriesHistory.h"
#include "CiftiMappableDataFile.h"
#include "CiftiScalarDataSeriesFile.h"
#include "EventChartTwoLoadLineSeriesData.h"
#include "EventManager.h"
#include "SceneClass.h"
#include "SceneClassAssistant.h"
#include "SceneObjectMapIntegerKey.h"

using namespace caret;
    
/**
 * \class caret::ChartableTwoFileLineLayerChart
 * \brief Implementation of base chart delegate for line layer charts.
 * \ingroup Files
 */

/**
 * Constructor.
 *
 * @param lineLayerContentType
 *     Content type of the line series data.
 * @param parentCaretMappableDataFile
 *     Parent caret mappable data file that this delegate supports.
 */
ChartableTwoFileLineLayerChart::ChartableTwoFileLineLayerChart(const ChartTwoLineLayerContentTypeEnum::Enum lineLayerContentType,
                                                                                 CaretMappableDataFile* parentCaretMappableDataFile)
: ChartableTwoFileBaseChart(ChartTwoDataTypeEnum::CHART_DATA_TYPE_LINE_LAYER,
                                    parentCaretMappableDataFile),
m_lineLayerContentType(lineLayerContentType)
{
    CaretUnitsTypeEnum::Enum xAxisUnits = CaretUnitsTypeEnum::NONE;
    int32_t xAxisNumberOfElements = 0;
    
    m_defaultColor = ChartableTwoFileLineLayerChart::generateDefaultColor();
    validateDefaultColor();
    
    m_defaultLineWidth = ChartTwoDataCartesian::getDefaultLineWidth();

    CaretMappableDataFile* cmdf = getCaretMappableDataFile();
    CaretAssert(cmdf);
    
    int64_t numberOfChartMaps(0);
    
    const bool brainordinateDataSupportedFlag(true);
    switch (lineLayerContentType) {
        case ChartTwoLineLayerContentTypeEnum::LINE_LAYER_CONTENT_UNSUPPORTED:
            break;
        case ChartTwoLineLayerContentTypeEnum::LINE_LAYER_CONTENT_ROW_BRAINORDINATE_DATA:
        {
            const CiftiMappableDataFile* ciftiMapFile = getCiftiMappableDataFile();
            if (ciftiMapFile != NULL) {
                CaretAssert(ciftiMapFile);
                std::vector<int64_t> dims;
                ciftiMapFile->getMapDimensions(dims);
                CaretAssertVectorIndex(dims, 1);
                const int32_t numCols = dims[0];
                const int32_t numRows = dims[1];
                
                if ((numRows > 0)
                    && (numCols > 1)) {
                    numberOfChartMaps = numRows;
                    
                    const NiftiTimeUnitsEnum::Enum mapUnits = ciftiMapFile->getMapIntervalUnits();
                    xAxisUnits = CaretUnitsTypeEnum::NONE;
                    switch (mapUnits) {
                        case NiftiTimeUnitsEnum::NIFTI_UNITS_HZ:
                            xAxisUnits = CaretUnitsTypeEnum::HERTZ;
                            break;
                        case NiftiTimeUnitsEnum::NIFTI_UNITS_MSEC:
                            xAxisUnits = CaretUnitsTypeEnum::SECONDS;
                            break;
                        case NiftiTimeUnitsEnum::NIFTI_UNITS_PPM:
                            xAxisUnits = CaretUnitsTypeEnum::PARTS_PER_MILLION;
                            break;
                        case NiftiTimeUnitsEnum::NIFTI_UNITS_SEC:
                            xAxisUnits = CaretUnitsTypeEnum::SECONDS;
                            break;
                        case NiftiTimeUnitsEnum::NIFTI_UNITS_USEC:
                            xAxisUnits = CaretUnitsTypeEnum::SECONDS;
                            break;
                        case NiftiTimeUnitsEnum::NIFTI_UNITS_UNKNOWN:
                            break;
                    }
                    xAxisNumberOfElements = numCols;
                }
            }
        }
            break;
    }

    /*
     * Must have two or more elements
     */
    if (xAxisNumberOfElements <= 1) {
        m_lineLayerContentType = ChartTwoLineLayerContentTypeEnum::LINE_LAYER_CONTENT_UNSUPPORTED;
    }
    
    updateChartTwoCompoundDataTypeAfterFileChanges(ChartTwoCompoundDataType::newInstanceForLineLayer(xAxisUnits,
                                                                                                   xAxisNumberOfElements));
    CaretMappableDataFile* mapFile = getCaretMappableDataFile();
    CaretAssert(mapFile);
    m_mapLineCharts.resize(numberOfChartMaps);
    
    m_sceneAssistant = std::unique_ptr<SceneClassAssistant>(new SceneClassAssistant());
    m_sceneAssistant->add<CaretColorEnum, CaretColorEnum::Enum>("m_defaultColor",
                                                                &m_defaultColor);
    m_sceneAssistant->add("m_defaultLineWidth",
                          &m_defaultLineWidth);

}

/**
 * Destructor.
 */
ChartableTwoFileLineLayerChart::~ChartableTwoFileLineLayerChart()
{
    /*
     * If events added, remove only events add by this instance
     * since parent also receives events
     */
}

/**
 * Clear the chart lines
 */
void
ChartableTwoFileLineLayerChart::clearChartLines()
{
    for (auto& ptr : m_mapLineCharts) {
        ptr.reset();
    }
}

/**
 * Receive an event.
 *
 * @param event
 *    An event for which this instance is listening.
 */
void
ChartableTwoFileLineLayerChart::receiveEvent(Event* event)
{
    ChartableTwoFileBaseChart::receiveEvent(event);
}

/**
 * Get a bounding box for map data displayed within this overlay.
 * Bounds are provided for histogram and line-series charts only.
 * @param mapIndex
 *     Index of map
 * @param boundingBox
 *     Upon exit contains bounds for data within this overlay
 * @return
 *     True if the bounds are valid, else false.
 */bool
ChartableTwoFileLineLayerChart::getBounds(const int32_t mapIndex,
                                          BoundingBox& boundingBoxOut) const
{
    boundingBoxOut.resetForUpdate();
    ChartableTwoFileLineLayerChart* nonConst = const_cast<ChartableTwoFileLineLayerChart*>(this);
    CaretAssert(nonConst);
    const ChartTwoDataCartesian* cd = nonConst->getChartMapLine(mapIndex);
    if (cd != NULL) {
        cd->getBounds(boundingBoxOut);
        return true;
    }
    return false;
}

/**
 * @return Number of chart maps (usually different that number of brainmapped maps).
 */
int32_t
ChartableTwoFileLineLayerChart::getNumberOfChartMaps() const
{
    return m_mapLineCharts.size();
}

/**
 * Get map names for a CIFTI Brain Models map
 * @param brainModelsMap
 *    The brain models map
 * @param mapNames
 *   Output with map names
 */
void
ChartableTwoFileLineLayerChart::getMapNamesFromCiftiBrainMap(const CiftiBrainModelsMap& brainModelsMap,
                             std::vector<AString>& mapNames)
{
    const int64_t numNames = brainModelsMap.getLength();
    if (numNames == static_cast<int64_t>(mapNames.size())) {
        std::vector<StructureEnum::Enum> surfaceStructures = brainModelsMap.getSurfaceStructureList();
        for (auto s : surfaceStructures) {
            const AString name(StructureEnum::toGuiName(s));
            std::vector<CiftiBrainModelsMap::SurfaceMap> surfaceMap = brainModelsMap.getSurfaceMap(s);
            for (auto sm : surfaceMap) {
                CaretAssertVectorIndex(mapNames, sm.m_ciftiIndex);
                mapNames[sm.m_ciftiIndex] = (name
                                             + " Vertex "
                                             + AString::number(sm.m_surfaceNode + 1));
            }
            
        }
        
        std::vector<StructureEnum::Enum> volumeStructures = brainModelsMap.getVolumeStructureList();
        for (auto vs : volumeStructures) {
            const AString name(StructureEnum::toGuiName(vs));
            std::vector<CiftiBrainModelsMap::VolumeMap> volumeMap = brainModelsMap.getVolumeStructureMap(vs);
            for (auto vm : volumeMap) {
                CaretAssertVectorIndex(mapNames, vm.m_ciftiIndex);
                mapNames[vm.m_ciftiIndex] = (name
                                             + " Voxel IJK "
                                             + AString::fromNumbers(vm.m_ijk, 3, ","));
            }
        }
    }
}

/**
 * Get the name of the maps for the line charts
 */
void
ChartableTwoFileLineLayerChart::getChartMapNames(std::vector<AString>& mapNamesOut)
{
    if (m_mapLineChartNames.size() != m_mapLineCharts.size()) {
        const int32_t numMaps = static_cast<int32_t>(m_mapLineCharts.size());
        if (numMaps > 0) {
            m_mapLineChartNames.resize(numMaps);
            switch (m_lineLayerContentType) {
                case ChartTwoLineLayerContentTypeEnum::LINE_LAYER_CONTENT_UNSUPPORTED:
                    break;
                case ChartTwoLineLayerContentTypeEnum::LINE_LAYER_CONTENT_ROW_BRAINORDINATE_DATA:
                {
                    CiftiMappableDataFile* ciftiFile = getCiftiMappableDataFile();
                    if (ciftiFile != NULL) {
                        const CiftiXML& ciftiXML = ciftiFile->getCiftiXML();
                        const int mapDirection(CiftiXML::ALONG_COLUMN);
                        
                        if (ciftiXML.getMappingType(CiftiXML::ALONG_COLUMN) == CiftiMappingType::BRAIN_MODELS) {
                            getMapNamesFromCiftiBrainMap(ciftiXML.getBrainModelsMap(CiftiXML::ALONG_COLUMN),
                                                         m_mapLineChartNames);
                        }
                        else {
                            const CiftiMappingType* cmt = ciftiXML.getMap(mapDirection);
                            if (cmt != NULL) {
                                const int64_t len = cmt->getLength();
                                CaretAssert(numMaps == len);
                                if (numMaps == len) {
                                    for (int64_t i = 0; i < len; i++) {
                                        m_mapLineChartNames[i] = cmt->getIndexName(i);
                                    }
                                }
                            }
                        }
                    }
                }
                    break;
            }
        }
        else {
            m_mapLineChartNames.clear();
        }
    }
    
    mapNamesOut = m_mapLineChartNames;
}

/**
 * Get the chart lines for a map
 * @param chartMapIndex
 * Index of the map
 * @retrurn Line chart for map or NULL if not available.
 */
ChartTwoDataCartesian*
ChartableTwoFileLineLayerChart::getChartMapLine(const int32_t chartMapIndex)
{
    ChartTwoDataCartesian* chartDataOut(NULL);
    
    CaretMappableDataFile* mapFile = getCaretMappableDataFile();
    CaretAssert(mapFile);
    const AString mapFileName(mapFile->getFileName());
    
    if ((chartMapIndex >= 0)
        && (chartMapIndex < static_cast<int32_t>(m_mapLineCharts.size()))) {
        CaretAssertVectorIndex(m_mapLineCharts, chartMapIndex);
        if ( ! m_mapLineCharts[chartMapIndex]) {
            MapFileDataSelector mapFileSelector;
            bool loadDataFlag(true);
            switch (getLineLayerContentType()) {
                case ChartTwoLineLayerContentTypeEnum::LINE_LAYER_CONTENT_UNSUPPORTED:
                    break;
                case ChartTwoLineLayerContentTypeEnum::LINE_LAYER_CONTENT_ROW_BRAINORDINATE_DATA:
                    mapFileSelector.setRowIndex(mapFile,
                                                mapFileName,
                                                chartMapIndex);
                    loadDataFlag = true;
                    break;
            }
            
            if (loadDataFlag) {
                ChartTwoDataCartesian* cd = loadChartForMapFileSelector(mapFileSelector);
                if (cd != NULL) {
                    CaretAssertVectorIndex(m_mapLineCharts, chartMapIndex);
                    m_mapLineCharts[chartMapIndex].reset(cd);
                }
            }
        }
        
        CaretAssertVectorIndex(m_mapLineCharts, chartMapIndex);
        chartDataOut = m_mapLineCharts[chartMapIndex].get();
    }
    
    return chartDataOut;
}

/**
 * Get the chart lines for a map file selector
 * @param mapFileDataSelector
 * Map file selector
 * @retrurn Line chart for map or NULL if not available.
 */
ChartTwoDataCartesian*
ChartableTwoFileLineLayerChart::loadChartForMapFileSelector(const MapFileDataSelector& mapFileDataSelector)
{
    ChartTwoDataCartesian* chartDataOut(NULL);
    
    int32_t scalarRowIndex = -1;
    bool loadDataFlag = false;
    switch (m_lineLayerContentType) {
        case ChartTwoLineLayerContentTypeEnum::LINE_LAYER_CONTENT_UNSUPPORTED:
            return NULL;
            break;
        case ChartTwoLineLayerContentTypeEnum::LINE_LAYER_CONTENT_ROW_BRAINORDINATE_DATA:
            switch (mapFileDataSelector.getDataSelectionType()) {
                case MapFileDataSelector::DataSelectionType::INVALID:
                    break;
                case MapFileDataSelector::DataSelectionType::COLUMN_DATA:
                    break;
                case MapFileDataSelector::DataSelectionType::ROW_DATA:
                    loadDataFlag = true;
                    break;
                case MapFileDataSelector::DataSelectionType::SURFACE_VERTEX:
                    break;
                case MapFileDataSelector::DataSelectionType::SURFACE_VERTICES_AVERAGE:
                    break;
                case MapFileDataSelector::DataSelectionType::VOLUME_XYZ:
                    break;
            }
            break;
    }
    
    if (loadDataFlag) {
        std::vector<float> data;
        getCaretMappableDataFile()->getDataForSelector(mapFileDataSelector,
                                                       data);
        if ( ! data.empty()) {
            CaretAssert(getChartTwoCompoundDataType().getLineChartNumberOfElementsAxisX() == static_cast<int32_t>(data.size()));
            chartDataOut = createChartData();
            chartDataOut->setMapFileDataSelector(mapFileDataSelector);
            
            float x = 0.0f;
            float xStep = 0.0f;
            getCaretMappableDataFile()->getMapIntervalStartAndStep(x, xStep);
            
            const int32_t numData = static_cast<int32_t>(data.size());
            for (int32_t i = 0; i < numData; i++) {
                CaretAssertVectorIndex(data, i);
                chartDataOut->addPoint(x, data[i]);
                x += xStep;
            }
        }
    }
    
    return chartDataOut;
}

ChartTwoDataCartesian*
ChartableTwoFileLineLayerChart::createChartData() const
{
    const CaretUnitsTypeEnum::Enum xUnits = getChartTwoCompoundDataType().getLineChartUnitsAxisX();
    return new ChartTwoDataCartesian(ChartTwoDataTypeEnum::CHART_DATA_TYPE_LINE_SERIES,
                                     xUnits,
                                     CaretUnitsTypeEnum::NONE,
                                     GraphicsPrimitive::PrimitiveType::POLYGONAL_LINE_STRIP_BEVEL_JOIN);
}

/**
 * @return Content type of the line series data.
 */
ChartTwoLineLayerContentTypeEnum::Enum
ChartableTwoFileLineLayerChart::getLineLayerContentType() const
{
    return m_lineLayerContentType;
}

/**
 * @return Is this charting valid ?
 */
bool
ChartableTwoFileLineLayerChart::isValid() const
{
    return (m_lineLayerContentType != ChartTwoLineLayerContentTypeEnum::LINE_LAYER_CONTENT_UNSUPPORTED);
}

/**
 * @retrurn Is this charting empty (no data at this time)
 */
bool
ChartableTwoFileLineLayerChart::isEmpty() const
{
    if ( ! isValid()) {
        return true;
    }

    return getCaretMappableDataFile()->isEmpty();
}

/**
 * Save subclass data to the scene.
 *
 * @param sceneAttributes
 *    Attributes for the scene.  Scenes may be of different types
 *    (full, generic, etc) and the attributes should be checked when
 *    restoring the scene.
 *
 * @param sceneClass
 *     sceneClass to which data members should be added.  Will always
 *     be valid (non-NULL).
 */
void
ChartableTwoFileLineLayerChart::saveSubClassDataToScene(const SceneAttributes* sceneAttributes,
                                            SceneClass* sceneClass)
{
    m_sceneAssistant->saveMembers(sceneAttributes,
                                  sceneClass);
    
    SceneObjectMapIntegerKey* objMap(NULL);
    
    const int32_t numItems = static_cast<int32_t>(m_mapLineCharts.size());
    for (int32_t i = 0; i < numItems; i++) {
        ChartTwoDataCartesian* cd = getChartMapLine(i);
        if (cd != NULL) {
            if (objMap == NULL) {
                objMap = new SceneObjectMapIntegerKey("m_mapLineCharts",
                                                      SceneObjectDataTypeEnum::SCENE_CLASS);
            }
            const QString name("m_mapLineCharts["
                               + AString::number(i)
                               + "]");
            CaretAssert(objMap);
            objMap->addClass(i,
                             cd->saveToScene(sceneAttributes,
                                             name));
        }
    }
    
    if (objMap != NULL) {
        sceneClass->addChild(objMap);
    }
}

/**
 * Restore file data from the scene.
 *
 * @param sceneAttributes
 *    Attributes for the scene.  Scenes may be of different types
 *    (full, generic, etc) and the attributes should be checked when
 *    restoring the scene.
 *
 * @param sceneClass
 *     sceneClass for the instance of a class that implements
 *     this interface.  Will NEVER be NULL.
 */
void
ChartableTwoFileLineLayerChart::restoreSubClassDataFromScene(const SceneAttributes* sceneAttributes,
                                                 const SceneClass* sceneClass)
{
    clearChartLines();
    
    m_sceneAssistant->restoreMembers(sceneAttributes,
                                     sceneClass);
    
    const SceneObjectMapIntegerKey* objMap = sceneClass->getMapIntegerKey("m_mapLineCharts");
    if (objMap != NULL) {
        if (objMap->getDataType() == SceneObjectDataTypeEnum::SCENE_CLASS) {
            const std::vector<int32_t> indexKeys = objMap->getKeys();
            for (const auto key : indexKeys) {
                const SceneObject* so = objMap->getObject(key);
                if (so != NULL) {
                    const SceneClass* sc = dynamic_cast<const SceneClass*>(so);
                    if (sc != NULL) {
                        ChartTwoDataCartesian* cartData = createChartData();
                        CaretAssert(cartData);
                        cartData->restoreFromScene(sceneAttributes,
                                                   sc);
                        m_mapLineCharts[key].reset(cartData);
                    }
                }
            }
        }
    }
}

/**
 * Generate the default color.
 */
CaretColorEnum::Enum
ChartableTwoFileLineLayerChart::generateDefaultColor()
{
    /*
     * No black or white since they are used for backgrounds
     */
    std::vector<CaretColorEnum::Enum> colors;
    CaretColorEnum::getColorEnumsNoBlackOrWhite(colors);
    CaretAssert( ! colors.empty());
    CaretColorEnum::Enum color = colors[0];
    
    const int32_t numColors = static_cast<int32_t>(colors.size());
    CaretAssert(numColors > 0);
    if (s_defaultColorIndexGenerator < 0) {
        s_defaultColorIndexGenerator = 0;
    }
    else if (s_defaultColorIndexGenerator >= numColors) {
        s_defaultColorIndexGenerator = 0;
    }
    
    CaretAssertVectorIndex(colors, s_defaultColorIndexGenerator);
    color = colors[s_defaultColorIndexGenerator];
    
    /* move to next color */
    ++s_defaultColorIndexGenerator;
    
    return color;
}

/**
 * Validate the default color.
 */
void
ChartableTwoFileLineLayerChart::validateDefaultColor()
{
    std::vector<CaretColorEnum::Enum> allEnums;
    CaretColorEnum::getColorAndOptionalEnums(allEnums, (CaretColorEnum::ColorOptions::OPTION_INCLUDE_CUSTOM_COLOR
                                                        | CaretColorEnum::CaretColorEnum::OPTION_INCLUDE_NONE_COLOR));
    if (std::find(allEnums.begin(),
                  allEnums.end(),
                  m_defaultColor) == allEnums.end()) {
        const AString msg("Default color enum is invalid.  Integer value: " + AString::number((int)m_defaultColor));
        CaretLogSevere(msg);
        m_defaultColor = CaretColorEnum::RED;
    }
    
    if (m_defaultColor == CaretColorEnum::CUSTOM) {
        const AString msg("Default color CUSTOM is not allowed");
        CaretLogSevere(msg);
        m_defaultColor = CaretColorEnum::RED;
    }
    else if (m_defaultColor == CaretColorEnum::NONE) {
        const AString msg("Default color NONE is not allowed");
        CaretLogSevere(msg);
        m_defaultColor = CaretColorEnum::RED;
    }
}

/**
 * @return The default color.
 */
CaretColorEnum::Enum
ChartableTwoFileLineLayerChart::getDefaultColor() const
{
    return m_defaultColor;
}

/**
 * Set the default color.
 *
 * @param defaultColor New value for default color.
 */
void
ChartableTwoFileLineLayerChart::setDefaultColor(const CaretColorEnum::Enum defaultColor)
{
    if (defaultColor != m_defaultColor) {
        m_defaultColor = defaultColor;
        validateDefaultColor();
        setModified();
    }
}

/**
 * @return Default width of lines
 */
float
ChartableTwoFileLineLayerChart::getDefaultLineWidth() const
{
    return m_defaultLineWidth;
}

/**
 * Set Default width of lines
 *
 * @param defaultLineWidth
 *    New value for Default width of lines
 */
void
ChartableTwoFileLineLayerChart::setDefaultLineWidth(const float defaultLineWidth)
{
    if (defaultLineWidth != m_defaultLineWidth) {
        m_defaultLineWidth = defaultLineWidth;
        setModified();
    }
}
