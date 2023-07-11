
/*LICENSE_START*/
/*
 *  Copyright (C) 2021 Washington University School of Medicine
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

#define __VOLUME_GRAPHICS_PRIMITIVE_MANAGER_DECLARE__
#include "VolumeGraphicsPrimitiveManager.h"
#undef __VOLUME_GRAPHICS_PRIMITIVE_MANAGER_DECLARE__

#include "CaretAssert.h"
#include "CaretLogger.h"
#include "CaretMappableDataFile.h"
#include "GraphicsPrimitiveV3fT3f.h"
#include "GraphicsUtilitiesOpenGL.h"
#include "VolumeMappableInterface.h"
using namespace caret;


    
/**
 * \class caret::VolumeGraphicsPrimitiveManager
 * \brief Generates graphics primitives for drawing volumes using textures
 * \ingroup Files
 */

/**
 * Constructor.
 * @param mapDataFile
 *    The mappable data file
 * @param volumeInterface
 *    The file's volume interface
 */
VolumeGraphicsPrimitiveManager::VolumeGraphicsPrimitiveManager(CaretMappableDataFile* mapDataFile,
                                                               VolumeMappableInterface* volumeInterface)
: CaretObject(),
m_mapDataFile(mapDataFile),
m_volumeInterface(volumeInterface)
{
    CaretAssert(m_mapDataFile);
    CaretAssert(m_volumeInterface);
}

/**
 * Destructor.
 */
VolumeGraphicsPrimitiveManager::~VolumeGraphicsPrimitiveManager()
{
}

/**
 * Clear this instance
 */
void
VolumeGraphicsPrimitiveManager::clear()
{
    m_mapGraphicsTriangleFanPrimitives.clear();
    m_mapGraphicsTriangleStripPrimitives.clear();
    m_mapGraphicsTrianglesPrimitives.clear();
}

/**
 * Invalidate primitive for the given map
 * @param mapIndex
 *    Index of the map
 */
void
VolumeGraphicsPrimitiveManager::invalidateAllColoring()
{
    for (auto& p : m_mapGraphicsTriangleFanPrimitives) {
        p.reset();
    }
    for (auto& p : m_mapGraphicsTriangleStripPrimitives) {
        p.reset();
    }
    for (auto& p : m_mapGraphicsTrianglesPrimitives) {
        p.reset();
    }
}

/**
 * Invalidate primitive for the given map
 * @param mapIndex
 *    Index of the map
 */
void
VolumeGraphicsPrimitiveManager::invalidateColoringForMap(const int32_t mapIndex)
{
    if ((mapIndex >= 0)
        && (mapIndex < static_cast<int32_t>(m_mapGraphicsTriangleFanPrimitives.size()))) {
        m_mapGraphicsTriangleFanPrimitives[mapIndex].reset();
    }
    if ((mapIndex >= 0)
        && (mapIndex < static_cast<int32_t>(m_mapGraphicsTriangleStripPrimitives.size()))) {
        m_mapGraphicsTriangleStripPrimitives[mapIndex].reset();
    }
    if ((mapIndex >= 0)
        && (mapIndex < static_cast<int32_t>(m_mapGraphicsTrianglesPrimitives.size()))) {
        m_mapGraphicsTrianglesPrimitives[mapIndex].reset();
    }
}

/**
 * Get the graphics primitive for drawing a volume's map
 *
 * @param primitiveShape
 *    Shape for primitive drawing
 * @param mapIndex
 *    Index of the map.
 * @param displayGroup
 *    The selected display group.
 * @param tabIndex
 *    Index of selected tab.
 * @return
 *    Graphics primitive or NULL if unable to draw
 */
GraphicsPrimitiveV3fT3f*
VolumeGraphicsPrimitiveManager::getVolumeDrawingPrimitiveForMap(const PrimitiveShape primitiveShape,
                                                                const int32_t mapIndex,
                                                                const DisplayGroupEnum::Enum displayGroup,
                                                                const int32_t tabIndex) const
{
    if (m_mapDataFile->getNumberOfMaps() != static_cast<int32_t>(m_mapGraphicsTriangleFanPrimitives.size())) {
        m_mapGraphicsTriangleFanPrimitives.resize(mapIndex + 1);
    }
    if (m_mapDataFile->getNumberOfMaps() != static_cast<int32_t>(m_mapGraphicsTriangleStripPrimitives.size())) {
        m_mapGraphicsTriangleStripPrimitives.resize(mapIndex + 1);
    }
    if (m_mapDataFile->getNumberOfMaps() != static_cast<int32_t>(m_mapGraphicsTrianglesPrimitives.size())) {
        m_mapGraphicsTrianglesPrimitives.resize(mapIndex + 1);
    }

    GraphicsPrimitiveV3fT3f* primitiveOut(NULL);
    switch (primitiveShape) {
        case PrimitiveShape::TRIANGLE_FAN:
            CaretAssertVectorIndex(m_mapGraphicsTriangleFanPrimitives, mapIndex);
            primitiveOut = m_mapGraphicsTriangleFanPrimitives[mapIndex].get();
            break;
        case PrimitiveShape::TRIANGLE_STRIP:
            CaretAssertVectorIndex(m_mapGraphicsTriangleStripPrimitives, mapIndex);
            primitiveOut = m_mapGraphicsTriangleStripPrimitives[mapIndex].get();
            break;
        case PrimitiveShape::TRIANGLES:
            CaretAssertVectorIndex(m_mapGraphicsTrianglesPrimitives, mapIndex);
            primitiveOut = m_mapGraphicsTrianglesPrimitives[mapIndex].get();
            break;
    }
    
    if (primitiveOut == NULL) {
        AString errorMessage;
        primitiveOut = VolumeGraphicsPrimitiveManager::createPrimitive(primitiveShape,
                                                                       mapIndex,
                                                                       displayGroup,
                                                                       tabIndex,
                                                                       errorMessage);
        if (primitiveOut != NULL) {
            switch (primitiveShape) {
                case PrimitiveShape::TRIANGLE_FAN:
                    CaretAssertVectorIndex(m_mapGraphicsTriangleFanPrimitives, mapIndex);
                    m_mapGraphicsTriangleFanPrimitives[mapIndex].reset(primitiveOut);
                    break;
                case PrimitiveShape::TRIANGLE_STRIP:
                    CaretAssertVectorIndex(m_mapGraphicsTriangleStripPrimitives, mapIndex);
                    m_mapGraphicsTriangleStripPrimitives[mapIndex].reset(primitiveOut);
                    break;
                case PrimitiveShape::TRIANGLES:
                    CaretAssertVectorIndex(m_mapGraphicsTrianglesPrimitives, mapIndex);
                    m_mapGraphicsTrianglesPrimitives[mapIndex].reset(primitiveOut);
                    break;
            }
        }
        else {
            CaretLogSevere(m_mapDataFile->getFileNameNoPath()
                           + errorMessage);
        }
    }
    
    return primitiveOut;
}

/**
 * Get a description of this object's content.
 * @return String describing this object's content.
 */
AString 
VolumeGraphicsPrimitiveManager::toString() const
{
    return "VolumeGraphicsPrimitiveManager";
}

/**
 * Generate a graphics primitive for drawing the volumes as RGBA or as identification
 * @param primitiveShape
 *    Shape for primitive drawing
 * @param mapIndex
 *    Map index for creating the primitive
 * @param displayGroup
 *    Display gtroup selected
 * @param tabIndex
 *    Index of tab
 * @param errorMessageOut
 *    Contains information if error occurs
 * @return
 *    Pointer to graphics primitive or NULL if failure
 */
GraphicsPrimitiveV3fT3f*
VolumeGraphicsPrimitiveManager::createPrimitive(const PrimitiveShape primitiveShape,
                                                const int32_t mapIndex,
                                                const DisplayGroupEnum::Enum displayGroup,
                                                const int32_t tabIndex,
                                                AString& errorMessageOut) const
{
    CaretAssert(m_volumeInterface);
    errorMessageOut.clear();
    
    std::vector<int64_t> dims(5);
    m_volumeInterface->getDimensions(dims);

    int64_t numberOfSlices(dims[2]);
    int64_t numberOfRows(dims[1]);
    int64_t numberOfColumns(dims[0]);

    const int64_t maxTextureSize(GraphicsUtilitiesOpenGL::getTextureDepthMaximumDimension());
    AString sizeMsg;
    if (numberOfColumns > maxTextureSize) {
        sizeMsg.appendWithNewLine("Width="
                                  + AString::number(numberOfColumns)
                                  + " is too big for 3D Texture.");
    }
    if (numberOfRows > maxTextureSize) {
        sizeMsg.appendWithNewLine("Height="
                                  + AString::number(numberOfRows)
                                  + " is too big for 3D Texture.");
    }
    if (numberOfSlices > maxTextureSize) {
        sizeMsg.appendWithNewLine("Slices="
                                  + AString::number(numberOfSlices)
                                  + " is too big for 3D Texture.");
    }
    if ( ! sizeMsg.isEmpty()) {
        sizeMsg.appendWithNewLine("Maximum texture dimension="
                                  + AString::number(maxTextureSize));
        errorMessageOut = sizeMsg;
        return NULL;
    }
    
    /*
     * Allocate storage for rgba data that is used by the graphics primitive
     */
    int64_t numberOfTextureBytes(0);
    std::shared_ptr<uint8_t> imageRgbaData = GraphicsTextureSettings::allocateImageRgbaData(numberOfColumns,
                                                                                            numberOfRows,
                                                                                            numberOfSlices,
                                                                                            &numberOfTextureBytes);
    uint8_t* imageRgbaPtr(imageRgbaData.get()); /* simplify access for loading */
    
    const int64_t numSliceBytes = (numberOfRows * numberOfColumns * 4);
    std::vector<uint8_t> rgbaSlice(numSliceBytes);

    for (int64_t k = 0; k < numberOfSlices; k++) {
        int64_t firstVoxelIJK[3] = { 0, 0, k };
        int64_t rowStepIJK[3] = { 0, 1, 0 };
        int64_t columnStepIJK[3] = { 1, 0, 0 };
        
        std::fill(rgbaSlice.begin(), rgbaSlice.end(), 0);
        m_volumeInterface->getVoxelColorsForSliceInMap(mapIndex,
                                                     firstVoxelIJK,
                                                     rowStepIJK,
                                                     columnStepIJK,
                                                     numberOfRows,
                                                     numberOfColumns,
                                                     displayGroup,
                                                     tabIndex,
                                                     &rgbaSlice[0]);
        
        for (int64_t j = 0; j < numberOfRows; j++) {
            for (int64_t i = 0; i < numberOfColumns; i++) {
                const int64_t sliceOffset = ((j * numberOfColumns) + i) * 4;
                const int64_t textureOffset = ((k * numberOfColumns * numberOfRows)
                                               + (j * numberOfColumns) + i) * 4;
                for (int64_t m = 0; m < 4; m++) {
                    CaretAssertArrayIndex(imageRgbaPtr, numberOfTextureBytes, (textureOffset + m));
                    CaretAssertVectorIndex(rgbaSlice, sliceOffset + m);
                    imageRgbaPtr[textureOffset + m] = rgbaSlice[sliceOffset + m];
                }
            }
        }
    }

    const bool useMipMaps(true);
    GraphicsTextureSettings::MipMappingType mipMap(GraphicsTextureSettings::MipMappingType::DISABLED);
    GraphicsTextureMagnificationFilterEnum::Enum magFilter(GraphicsTextureMagnificationFilterEnum::LINEAR);
    GraphicsTextureMinificationFilterEnum::Enum minFilter(GraphicsTextureMinificationFilterEnum::LINEAR);
    if (useMipMaps) {
        mipMap    = GraphicsTextureSettings::MipMappingType::ENABLED;
        minFilter = GraphicsTextureMinificationFilterEnum::LINEAR_MIPMAP_LINEAR;
        
        /*
         * 31 March 2023 Disable mip maps until I (John H) can test on Linux and Windows
         */
        mipMap    = GraphicsTextureSettings::MipMappingType::DISABLED;
        minFilter = GraphicsTextureMinificationFilterEnum::LINEAR;
    }
    
    if (m_mapDataFile->isMappedWithRGBA()
        || m_mapDataFile->isMappedWithLabelTable()) {
        mipMap    = GraphicsTextureSettings::MipMappingType::DISABLED;
        magFilter  = GraphicsTextureMagnificationFilterEnum::NEAREST;
        minFilter  = GraphicsTextureMinificationFilterEnum::NEAREST;
    }
    std::array<float, 4> backgroundColor { 0.0, 0.0, 0.0, 0.0 };
    GraphicsTextureSettings textureSettings(imageRgbaData,
                                            numberOfColumns,
                                            numberOfRows,
                                            numberOfSlices,
                                            GraphicsTextureSettings::DimensionType::FLOAT_STR_3D,
                                            GraphicsTextureSettings::PixelFormatType::RGBA,
                                            GraphicsTextureSettings::PixelOrigin::BOTTOM_LEFT,
                                            GraphicsTextureSettings::WrappingType::CLAMP_TO_BORDER,
                                            mipMap,
                                            GraphicsTextureSettings::CompressionType::DISABLED,
                                            magFilter,
                                            minFilter,
                                            backgroundColor);
    
    GraphicsPrimitive::PrimitiveType primType(GraphicsPrimitive::PrimitiveType::OPENGL_TRIANGLE_STRIP);
    int32_t numVertices(0);
    switch (primitiveShape) {
        case PrimitiveShape::TRIANGLE_FAN:
            primType = GraphicsPrimitive::PrimitiveType::OPENGL_TRIANGLE_FAN;
            numVertices = 8;
            break;
        case PrimitiveShape::TRIANGLE_STRIP:
            primType = GraphicsPrimitive::PrimitiveType::OPENGL_TRIANGLE_STRIP;
            numVertices = 4;
            break;
        case PrimitiveShape::TRIANGLES:
            primType = GraphicsPrimitive::PrimitiveType::OPENGL_TRIANGLES;
            numVertices = 18;
            break;
    }

    GraphicsPrimitiveV3fT3f* primitiveOut(GraphicsPrimitive::newPrimitiveV3fT3f(primType,
                                                                                textureSettings));
    CaretAssert(primitiveOut);
    
    /*
     * Need four vertices and texture coordinates but their values do no matter here.
     * They will be replaced when a volume slice is drawn.
     */
    const float xyz[3] { 0.0, 0.0, 0.0 };
    const float str[3] { 0.0, 0.0, 0.0 };
    for (int32_t i = 0; i < numVertices; i++) {
        primitiveOut->addVertex(xyz, str);
    }

    return primitiveOut;
}
