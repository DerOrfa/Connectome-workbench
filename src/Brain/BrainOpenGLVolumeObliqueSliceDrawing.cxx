
/*LICENSE_START*/
/*
 *  Copyright (C) 2014 Washington University School of Medicine
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

#include <cmath>

#define __BRAIN_OPEN_GL_VOLUME_OBLIQUE_SLICE_DRAWING_DECLARE__
#include "BrainOpenGLVolumeObliqueSliceDrawing.h"
#undef __BRAIN_OPEN_GL_VOLUME_OBLIQUE_SLICE_DRAWING_DECLARE__

#include "AnnotationCoordinate.h"
#include "AnnotationPercentSizeText.h"
#include "BoundingBox.h"
#include "Brain.h"
#include "BrainOpenGLAnnotationDrawingFixedPipeline.h"
#include "BrainOpenGLPrimitiveDrawing.h"
#include "BrainOpenGLViewportContent.h"
#include "BrainOpenGLVolumeSliceDrawing.h"
#include "BrowserTabContent.h"
#include "CaretAssert.h"
#include "CaretLogger.h"
#include "CaretOpenGLInclude.h"
#include "CaretPreferences.h"
#include "CiftiMappableDataFile.h"
#include "DeveloperFlagsEnum.h"
#include "DisplayPropertiesFoci.h"
#include "DisplayPropertiesLabels.h"
#include "ElapsedTimer.h"
#include "FociFile.h"
#include "Focus.h"
#include "GapsAndMargins.h"
#include "GiftiLabel.h"
#include "GiftiLabelTable.h"
#include "GroupAndNameHierarchyModel.h"
#include "GraphicsEngineDataOpenGL.h"
#include "GraphicsPrimitiveV3fC4f.h"
#include "GraphicsPrimitiveV3fC4ub.h"
#include "IdentificationWithColor.h"
#include "LabelDrawingProperties.h"
#include "MathFunctions.h"
#include "Matrix4x4.h"
#include "ModelVolume.h"
#include "ModelWholeBrain.h"
#include "NodeAndVoxelColoring.h"
#include "SelectionItemFocusVolume.h"
#include "SelectionItemVoxel.h"
#include "SelectionItemVoxelEditing.h"
#include "SelectionManager.h"
#include "SessionManager.h"
#include "Surface.h"
#include "SurfacePlaneIntersectionToContour.h"
#include "VolumeFile.h"
#include "VolumeSurfaceOutlineColorOrTabModel.h"
#include "VolumeSurfaceOutlineModel.h"
#include "VolumeSurfaceOutlineSetModel.h"

using namespace caret;

static const bool debugFlag = false;
    
/**
 * \class caret::BrainOpenGLVolumeObliqueSliceDrawing 
 * \brief Draws volume slices using OpenGL
 * \ingroup Brain
 */

/**
 * Constructor.
 */
BrainOpenGLVolumeObliqueSliceDrawing::BrainOpenGLVolumeObliqueSliceDrawing()
: CaretObject()
{
    
}

/**
 * Destructor.
 */
BrainOpenGLVolumeObliqueSliceDrawing::~BrainOpenGLVolumeObliqueSliceDrawing()
{
}

/**
 * Draw Volume Slices or slices for ALL Stuctures View.
 *
 * @param fixedPipelineDrawing
 *    The OpenGL drawing.
 * @param browserTabContent
 *    Content of browser tab that is to be drawn.
 * @param volumeDrawInfo
 *    Info on each volume layers for drawing.
 * @param sliceDrawingType
 *    Type of slice drawing (montage, single)
 * @param sliceProjectionType
 *    Type of projection for the slice drawing (oblique, orthogonal)
 * @param obliqueSliceMaskingType
 *    Masking for oblique slice drawing
 * @param viewport
 *    The viewport (region of graphics area) for drawing slices.
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::draw(BrainOpenGLFixedPipeline* fixedPipelineDrawing,
                                  BrowserTabContent* browserTabContent,
                                  std::vector<BrainOpenGLFixedPipeline::VolumeDrawInfo>& volumeDrawInfo,
                                  const VolumeSliceDrawingTypeEnum::Enum sliceDrawingType,
                                  const VolumeSliceProjectionTypeEnum::Enum sliceProjectionType,
                                           const VolumeSliceInterpolationEdgeEffectsMaskingEnum::Enum obliqueSliceMaskingType,
                                  const int32_t viewport[4])
{
    CaretAssert(sliceProjectionType == VolumeSliceProjectionTypeEnum::VOLUME_SLICE_PROJECTION_OBLIQUE);
    
    if (volumeDrawInfo.empty()) {
        return;
    }
    
    CaretAssert(fixedPipelineDrawing);
    CaretAssert(browserTabContent);
    m_browserTabContent = browserTabContent;    
    m_fixedPipelineDrawing = fixedPipelineDrawing;
    m_obliqueSliceMaskingType = obliqueSliceMaskingType;
    /*
     * No lighting for drawing slices
     */
    m_fixedPipelineDrawing->disableLighting();
    
    /*
     * Initialize class members which help reduce the number of
     * parameters that are passed to methods.
     */
    m_brain           = NULL;
    m_modelVolume     = NULL;
    m_modelWholeBrain = NULL;
    m_modelType       = ModelTypeEnum::MODEL_TYPE_INVALID;
    if (m_browserTabContent->getDisplayedVolumeModel() != NULL) {
        m_modelVolume = m_browserTabContent->getDisplayedVolumeModel();
        m_brain = m_modelVolume->getBrain();
        m_modelType = m_modelVolume->getModelType();
    }
    else if (m_browserTabContent->getDisplayedWholeBrainModel() != NULL) {
        m_modelWholeBrain = m_browserTabContent->getDisplayedWholeBrainModel();
        m_brain = m_modelWholeBrain->getBrain();
        m_modelType = m_modelWholeBrain->getModelType();
    }
    else {
        CaretAssertMessage(0, "Invalid model for volume slice drawing.");
    }
    CaretAssert(m_brain);
    CaretAssert(m_modelType != ModelTypeEnum::MODEL_TYPE_INVALID);
    
    m_volumeDrawInfo = volumeDrawInfo;
    if (m_volumeDrawInfo.empty()) {
        return;
    }
    m_underlayVolume = m_volumeDrawInfo[0].volumeFile;
    
    const DisplayPropertiesLabels* dsl = m_brain->getDisplayPropertiesLabels();
    m_displayGroup = dsl->getDisplayGroupForTab(m_fixedPipelineDrawing->windowTabIndex);
    
    m_tabIndex = m_browserTabContent->getTabNumber();
    
    /*
     * Cifti files are slow at getting individual voxels since they
     * provide no access to individual voxels.  The reason is that
     * the data may be on a server (Dense data) and accessing a single
     * voxel would require requesting the entire map.  So, for
     * each Cifti file, get the enter map.  This also, eliminate multiple
     * requests for the same map when drawing an ALL view.
     */
    const int32_t numVolumes = static_cast<int32_t>(m_volumeDrawInfo.size());
    for (int32_t i = 0; i < numVolumes; i++) {
        std::vector<float> ciftiMapData;
        m_ciftiMappableFileData.push_back(ciftiMapData);
        
        const CiftiMappableDataFile* ciftiMapFile = dynamic_cast<const CiftiMappableDataFile*>(m_volumeDrawInfo[i].volumeFile);
        if (ciftiMapFile != NULL) {
            ciftiMapFile->getMapData(m_volumeDrawInfo[i].mapIndex,
                                     m_ciftiMappableFileData[i]);
        }
    }
    
    /** END SETUP OF MEMBERS IN PARENT CLASS BrainOpenGLVolumeObliqueSliceDrawing */
    
    
    
    if (browserTabContent->getDisplayedVolumeModel() != NULL) {
        drawVolumeSliceViewPlane(sliceDrawingType,
                                 sliceProjectionType,
                                 browserTabContent->getSliceViewPlane(),
                                 browserTabContent->getSlicePlanesAllViewLayout(),
                                 viewport);
    }
    else if (browserTabContent->getDisplayedWholeBrainModel() != NULL) {
        drawVolumeSlicesForAllStructuresView(sliceProjectionType,
                                             viewport);
    }    
}

/**
 * Draw volume view slices for the given view plane.
 *
 * @param sliceDrawingType
 *    Type of slice drawing (montage, single)
 * @param sliceProjectionType
 *    Type of projection for the slice drawing (oblique, orthogonal)
 * @param sliceViewPlane
 *    The plane for slice drawing.
 * @param allPlanesLayout
 *    The layout in ALL slices view.
 * @param viewport
 *    The viewport (region of graphics area) for drawing slices.
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::drawVolumeSliceViewPlane(const VolumeSliceDrawingTypeEnum::Enum sliceDrawingType,
                                                        const VolumeSliceProjectionTypeEnum::Enum sliceProjectionType,
                                                        const VolumeSliceViewPlaneEnum::Enum sliceViewPlane,
                                                        const VolumeSliceViewAllPlanesLayoutEnum::Enum allPlanesLayout,
                                                        const int32_t viewport[4])
{
    switch (sliceViewPlane) {
        case VolumeSliceViewPlaneEnum::ALL:
        {
            int32_t paraVP[4]    = { 0, 0, 0, 0 };
            int32_t coronalVP[4] = { 0, 0, 0, 0 };
            int32_t axialVP[4]   = { 0, 0, 0, 0 };
            
            BrainOpenGLViewportContent::getSliceAllViewViewport(viewport,
                                                                   VolumeSliceViewPlaneEnum::PARASAGITTAL,
                                                                   allPlanesLayout,
                                                                   paraVP);
            BrainOpenGLViewportContent::getSliceAllViewViewport(viewport,
                                                                   VolumeSliceViewPlaneEnum::CORONAL,
                                                                   allPlanesLayout,
                                                                   coronalVP);
            BrainOpenGLViewportContent::getSliceAllViewViewport(viewport,
                                                                   VolumeSliceViewPlaneEnum::AXIAL,
                                                                   allPlanesLayout,
                                                                   axialVP);
            
            /*
             * Draw parasagittal slice
             */
            glPushMatrix();
            drawVolumeSliceViewType(sliceDrawingType,
                                    sliceProjectionType,
                                    VolumeSliceViewPlaneEnum::PARASAGITTAL,
                                    paraVP);
            glPopMatrix();
            
            
            /*
             * Draw coronal slice
             */
            glPushMatrix();
            drawVolumeSliceViewType(sliceDrawingType,
                                    sliceProjectionType,
                                    VolumeSliceViewPlaneEnum::CORONAL,
                                    coronalVP);
            glPopMatrix();
            
            
            /*
             * Draw axial slice
             */
            glPushMatrix();
            drawVolumeSliceViewType(sliceDrawingType,
                                    sliceProjectionType,
                                    VolumeSliceViewPlaneEnum::AXIAL,
                                    axialVP);
            glPopMatrix();
            
            if (allPlanesLayout == VolumeSliceViewAllPlanesLayoutEnum::GRID_LAYOUT) {
                /*
                 * 4th quadrant is used for axis showing orientation
                 */
                int32_t allVP[4] = { 0, 0, 0, 0 };
                BrainOpenGLViewportContent::getSliceAllViewViewport(viewport,
                                                                       VolumeSliceViewPlaneEnum::ALL,
                                                                       allPlanesLayout,
                                                                       allVP);
                
                switch (sliceProjectionType) {
                    case VolumeSliceProjectionTypeEnum::VOLUME_SLICE_PROJECTION_OBLIQUE:
                        drawOrientationAxes(allVP);
                        break;
                    case VolumeSliceProjectionTypeEnum::VOLUME_SLICE_PROJECTION_ORTHOGONAL:
                        CaretAssert(0);
                        break;
                }
            }
        }
            break;
        case VolumeSliceViewPlaneEnum::AXIAL:
        case VolumeSliceViewPlaneEnum::CORONAL:
        case VolumeSliceViewPlaneEnum::PARASAGITTAL:
            drawVolumeSliceViewType(sliceDrawingType,
                                    sliceProjectionType,
                                    sliceViewPlane,
                                    viewport);
            break;
    }
}

/**
 * Draw slices for the all structures view.
 *
 * @param sliceProjectionType
 *    Type of projection for the slice drawing (oblique, orthogonal)
 * @param viewport
 *    The viewport.
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::drawVolumeSlicesForAllStructuresView(const VolumeSliceProjectionTypeEnum::Enum sliceProjectionType,
                                                                  const int32_t viewport[4])
{
    m_orthographicBounds[0] = m_fixedPipelineDrawing->orthographicLeft;
    m_orthographicBounds[1] = m_fixedPipelineDrawing->orthographicRight;
    m_orthographicBounds[2] = m_fixedPipelineDrawing->orthographicBottom;
    m_orthographicBounds[3] = m_fixedPipelineDrawing->orthographicTop;
    m_orthographicBounds[4] = m_fixedPipelineDrawing->orthographicNear;
    m_orthographicBounds[5] = m_fixedPipelineDrawing->orthographicFar;
    
    /*
     * Enlarge the region
     */
    {
        const float left   = m_fixedPipelineDrawing->orthographicLeft;
        const float right  = m_fixedPipelineDrawing->orthographicRight;
        const float bottom = m_fixedPipelineDrawing->orthographicBottom;
        const float top    = m_fixedPipelineDrawing->orthographicTop;
        
        const float scale = 2.0;
        
        const float centerX  = (left + right) / 2.0;
        const float dx       = (right - left) / 2.0;
        const float newLeft  = centerX - (dx * scale);
        const float newRight = centerX + (dx * scale);
        
        const float centerY    = (bottom + top) / 2.0;
        const float dy         = (top - bottom) / 2.0;
        const float newBottom  = centerY - (dy * scale);
        const float newTop     = centerY + (dy * scale);
        
        m_orthographicBounds[0] = newLeft;
        m_orthographicBounds[1] = newRight;
        m_orthographicBounds[2] = newBottom;
        m_orthographicBounds[3] = newTop;
    }
    
    const float sliceCoordinates[3] = {
        m_browserTabContent->getSliceCoordinateParasagittal(),
        m_browserTabContent->getSliceCoordinateCoronal(),
        m_browserTabContent->getSliceCoordinateAxial()
    };
    
    if (m_browserTabContent->isSliceAxialEnabled()) {
        glPushMatrix();
        drawVolumeSliceViewProjection(VolumeSliceDrawingTypeEnum::VOLUME_SLICE_DRAW_SINGLE,
                                      sliceProjectionType,
                                      VolumeSliceViewPlaneEnum::AXIAL,
                                      sliceCoordinates,
                                      viewport);
        glPopMatrix();
    }
    
    if (m_browserTabContent->isSliceCoronalEnabled()) {
        glPushMatrix();
        drawVolumeSliceViewProjection(VolumeSliceDrawingTypeEnum::VOLUME_SLICE_DRAW_SINGLE,
                                      sliceProjectionType,
                                      VolumeSliceViewPlaneEnum::CORONAL,
                                      sliceCoordinates,
                                      viewport);
        glPopMatrix();
    }
    
    if (m_browserTabContent->isSliceParasagittalEnabled()) {
        glPushMatrix();
        drawVolumeSliceViewProjection(VolumeSliceDrawingTypeEnum::VOLUME_SLICE_DRAW_SINGLE,
                                      sliceProjectionType,
                                      VolumeSliceViewPlaneEnum::PARASAGITTAL,
                                      sliceCoordinates,
                                      viewport);
        glPopMatrix();
    }
}

/**
 * Draw single or montage volume view slices.
 *
 * @param sliceDrawingType
 *    Type of slice drawing (montage, single)
 * @param sliceProjectionType
 *    Type of projection for the slice drawing (oblique, orthogonal)
 * @param sliceViewPlane
 *    The plane for slice drawing.
 * @param viewport
 *    The viewport (region of graphics area) for drawing slices.
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::drawVolumeSliceViewType(const VolumeSliceDrawingTypeEnum::Enum sliceDrawingType,
                                           const VolumeSliceProjectionTypeEnum::Enum sliceProjectionType,
                                           const VolumeSliceViewPlaneEnum::Enum sliceViewPlane,
                                           const int32_t viewport[4])
{
    switch (sliceDrawingType) {
        case VolumeSliceDrawingTypeEnum::VOLUME_SLICE_DRAW_MONTAGE:
            drawVolumeSliceViewTypeMontage(sliceDrawingType,
                                           sliceProjectionType,
                                 sliceViewPlane,
                                 viewport);
            break;
        case VolumeSliceDrawingTypeEnum::VOLUME_SLICE_DRAW_SINGLE:
        {
            const float sliceCoordinates[3] = {
                m_browserTabContent->getSliceCoordinateParasagittal(),
                m_browserTabContent->getSliceCoordinateCoronal(),
                m_browserTabContent->getSliceCoordinateAxial()
            };
            drawVolumeSliceViewProjection(sliceDrawingType,
                                          sliceProjectionType,
                                sliceViewPlane,
                                sliceCoordinates,
                                viewport);
        }
            break;
    }
    
}

/**
 * Draw montage slices.
 *
 * @param sliceDrawingType
 *    Type of slice drawing (montage, single)
 * @param sliceProjectionType
 *    Type of projection for the slice drawing (oblique, orthogonal)
 * @param sliceViewPlane
 *    The plane for slice drawing.
 * @param viewport
 *    The viewport (region of graphics area) for drawing slices.
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::drawVolumeSliceViewTypeMontage(const VolumeSliceDrawingTypeEnum::Enum sliceDrawingType,
                                                            const VolumeSliceProjectionTypeEnum::Enum sliceProjectionType,
                                                  const VolumeSliceViewPlaneEnum::Enum sliceViewPlane,
                                                  const int32_t viewport[4])
{
    const int32_t numRows = m_browserTabContent->getMontageNumberOfRows();
    CaretAssert(numRows > 0);
    const int32_t numCols = m_browserTabContent->getMontageNumberOfColumns();
    CaretAssert(numCols > 0);
    
    const int32_t montageCoordPrecision = m_browserTabContent->getVolumeMontageCoordinatePrecision();
    
    const GapsAndMargins* gapsAndMargins = m_brain->getGapsAndMargins();
    
    const int32_t windowIndex = m_fixedPipelineDrawing->m_windowIndex;
    
    int32_t vpSizeY        = 0;
    int32_t verticalMargin = 0;
    BrainOpenGLFixedPipeline::createSubViewportSizeAndGaps(viewport[3],
                                                           gapsAndMargins->getVolumeMontageVerticalGapForWindow(windowIndex),
                                                           -1,
                                                           numRows,
                                                           vpSizeY,
                                                           verticalMargin);
    
    int32_t vpSizeX          = 0;
    int32_t horizontalMargin = 0;
    BrainOpenGLFixedPipeline::createSubViewportSizeAndGaps(viewport[2],
                                                           gapsAndMargins->getVolumeMontageHorizontalGapForWindow(windowIndex),
                                                           -1,
                                                           numCols,
                                                           vpSizeX,
                                                           horizontalMargin);
    
    /*
     * Voxel sizes for underlay volume
     */
    float originX, originY, originZ;
    float x1, y1, z1;
    m_underlayVolume->indexToSpace(0, 0, 0, originX, originY, originZ);
    m_underlayVolume->indexToSpace(1, 1, 1, x1, y1, z1);
    float sliceThickness = 0.0;
    float sliceOrigin    = 0.0;
    
    AString axisLetter = "";
    
    float sliceCoordinates[3] = {
        m_browserTabContent->getSliceCoordinateParasagittal(),
        m_browserTabContent->getSliceCoordinateCoronal(),
        m_browserTabContent->getSliceCoordinateAxial()
    };
    
    int32_t sliceIndex = -1;
    int32_t maximumSliceIndex = -1;
    int64_t dimI, dimJ, dimK, numMaps, numComponents;
    m_underlayVolume->getDimensions(dimI, dimJ, dimK, numMaps, numComponents);
    const int32_t sliceStep = m_browserTabContent->getMontageSliceSpacing();
    switch (sliceViewPlane) {
        case VolumeSliceViewPlaneEnum::ALL:
            sliceIndex = -1;
            break;
        case VolumeSliceViewPlaneEnum::AXIAL:
            sliceIndex = m_browserTabContent->getSliceIndexAxial(m_underlayVolume);
            maximumSliceIndex = dimK;
            sliceThickness = z1 - originZ;
            sliceOrigin = originZ;
            axisLetter = "Z";
            break;
        case VolumeSliceViewPlaneEnum::CORONAL:
            sliceIndex = m_browserTabContent->getSliceIndexCoronal(m_underlayVolume);
            maximumSliceIndex = dimJ;
            sliceThickness = y1 - originY;
            sliceOrigin = originY;
            axisLetter = "Y";
            break;
        case VolumeSliceViewPlaneEnum::PARASAGITTAL:
            sliceIndex = m_browserTabContent->getSliceIndexParasagittal(m_underlayVolume);
            maximumSliceIndex = dimI;
            sliceThickness = x1 - originX;
            sliceOrigin = originX;
            axisLetter = "X";
            break;
    }
    
    /*
     * Foreground color for slice coordinate text
     */
    const CaretPreferences* prefs = SessionManager::get()->getCaretPreferences();
    uint8_t foregroundRGBA[4];
    prefs->getBackgroundAndForegroundColors()->getColorForegroundVolumeView(foregroundRGBA);
    foregroundRGBA[3] = 255;
    uint8_t backgroundRGBA[4];
    prefs->getBackgroundAndForegroundColors()->getColorBackgroundVolumeView(backgroundRGBA);
    backgroundRGBA[3] = 255;
    const bool showCoordinates = m_browserTabContent->isVolumeMontageAxesCoordinatesDisplayed();
    
    /*
     * Determine a slice offset to selected slices is in
     * the center of the montage
     */
    const int32_t numSlicesViewed = (numCols * numRows);
    const int32_t sliceOffset = ((numSlicesViewed / 2)
                             * sliceStep);
    
    sliceIndex += sliceOffset;
    
    /*
     * Find first valid slice for montage
     */
    while (sliceIndex >= 0) {
        if (sliceIndex < maximumSliceIndex) {
            break;
        }
        sliceIndex -= sliceStep;
    }
    
    if (sliceIndex >= 0) {
        for (int32_t i = 0; i < numRows; i++) {
            for (int32_t j = 0; j < numCols; j++) {
                if ((sliceIndex >= 0)
                    && (sliceIndex < maximumSliceIndex)) {
                    const int32_t vpX = (j * (vpSizeX + horizontalMargin));
                    const int32_t vpY = ((numRows - i - 1) * (vpSizeY + verticalMargin));
                    int32_t vp[4] = {
                        viewport[0] + vpX,
                        viewport[1] + vpY,
                        vpSizeX,
                        vpSizeY
                    };
                    
                    if ((vp[2] <= 0)
                        || (vp[3] <= 0)) {
                        continue;
                    }
                    
                    const float sliceCoord = (sliceOrigin
                                              + sliceThickness * sliceIndex);
                    switch (sliceViewPlane) {
                        case VolumeSliceViewPlaneEnum::ALL:
                            break;
                        case VolumeSliceViewPlaneEnum::AXIAL:
                            sliceCoordinates[2] = sliceCoord;
                            break;
                        case VolumeSliceViewPlaneEnum::CORONAL:
                            sliceCoordinates[1] = sliceCoord;
                            break;
                        case VolumeSliceViewPlaneEnum::PARASAGITTAL:
                            sliceCoordinates[0] = sliceCoord;
                            break;
                    }
                    
                    drawVolumeSliceViewProjection(sliceDrawingType,
                                                  sliceProjectionType,
                                                  sliceViewPlane,
                                                  sliceCoordinates,
                                                  vp);
                    
                    if (showCoordinates) {
                        const AString coordText = (axisLetter
                                                   + "="
                                                   + AString::number(sliceCoord, 'f', montageCoordPrecision));
                        
                        AnnotationPercentSizeText annotationText(AnnotationAttributesDefaultTypeEnum::NORMAL);
                        annotationText.setHorizontalAlignment(AnnotationTextAlignHorizontalEnum::RIGHT);
                        annotationText.setVerticalAlignment(AnnotationTextAlignVerticalEnum::BOTTOM);
                        annotationText.setFontPercentViewportSize(10.0f);
                        annotationText.setLineColor(CaretColorEnum::NONE);
                        annotationText.setTextColor(CaretColorEnum::CUSTOM);
                        annotationText.setBackgroundColor(CaretColorEnum::CUSTOM);
                        annotationText.setCustomTextColor(foregroundRGBA);
                        annotationText.setCustomBackgroundColor(backgroundRGBA);
                        annotationText.setText(coordText);
                        m_fixedPipelineDrawing->drawTextAtViewportCoords((vpSizeX - 5),
                                                                         5.0,
                                                                         annotationText);
                    }
                }
                sliceIndex -= sliceStep;
            }
        }
    }
    
    /*
     * Draw the axes labels for the montage view
     */
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

    if (m_browserTabContent->isVolumeAxesCrosshairLabelsDisplayed()) {
        drawAxesCrosshairsOrthoAndOblique(sliceProjectionType,
                                          sliceViewPlane,
                                          sliceCoordinates,
                                          false,
                                          true);
    }
}

/**
 * Draw a slice for either projection mode (oblique, orthogonal)
 *
 * @param sliceDrawingType
 *    Type of slice drawing (montage, single)
 * @param sliceProjectionType
 *    Type of projection for the slice drawing (oblique, orthogonal)
 * @param sliceViewPlane
 *    The plane for slice drawing.
 * @param sliceCoordinates
 *    Coordinates of the selected slice.
 * @param viewport
 *    The viewport (region of graphics area) for drawing slices.
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::drawVolumeSliceViewProjection(const VolumeSliceDrawingTypeEnum::Enum sliceDrawingType,
                                                           const VolumeSliceProjectionTypeEnum::Enum sliceProjectionType,
                                                           const VolumeSliceViewPlaneEnum::Enum sliceViewPlane,
                                                           const float sliceCoordinates[3],
                                                           const int32_t viewport[4])
{
    bool twoDimSliceViewFlag = false;
    if (m_modelVolume != NULL) {
        twoDimSliceViewFlag = true;
    }
    else if (m_modelWholeBrain != NULL) {
        /* nothing */
    }
    else {
        CaretAssertMessage(0, "Invalid model type.");
    }
    
    if (twoDimSliceViewFlag) {
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glViewport(viewport[0],
                   viewport[1],
                   viewport[2],
                   viewport[3]);

        /*
         * Set the orthographic projection to fit the slice axis
         */
        setOrthographicProjection(sliceViewPlane,
                                  viewport);
    }
    
    /*
     * Create the plane equation for the slice
     */
    Plane slicePlane;
    createSlicePlaneEquation(sliceProjectionType,
                             sliceViewPlane,
                             sliceCoordinates,
                             slicePlane);
    CaretAssert(slicePlane.isValidPlane());
    if (slicePlane.isValidPlane() == false) {
        return;
    }
    
    
    if (twoDimSliceViewFlag) {
        /*
         * Set the viewing transformation (camera position)
         */
        setVolumeSliceViewingAndModelingTransformations(sliceProjectionType,
                                                        sliceViewPlane,
                                                        slicePlane,
                                                        sliceCoordinates);
    }
    
    SelectionItemVoxel* voxelID = m_brain->getSelectionManager()->getVoxelIdentification();
    SelectionItemVoxelEditing* voxelEditingID = m_brain->getSelectionManager()->getVoxelEditingIdentification();
    
    m_fixedPipelineDrawing->applyClippingPlanes(BrainOpenGLFixedPipeline::CLIPPING_DATA_TYPE_VOLUME,
                                                StructureEnum::ALL);
    
    /*
     * Check for a 'selection' type mode
     */
    bool drawVolumeSlicesFlag = true;
    m_identificationModeFlag = false;
    switch (m_fixedPipelineDrawing->mode) {
        case BrainOpenGLFixedPipeline::MODE_DRAWING:
            break;
        case BrainOpenGLFixedPipeline::MODE_IDENTIFICATION:
            if (voxelID->isEnabledForSelection()
                || voxelEditingID->isEnabledForSelection()) {
                m_identificationModeFlag = true;
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            }
            else {
                /*
                 * Don't return.  Allow other items (such as annotations) to be drawn.
                 */
                drawVolumeSlicesFlag = false;
            }
            break;
        case BrainOpenGLFixedPipeline::MODE_PROJECTION:
            return;
            break;
    }
    
    resetIdentification();
    
    GLboolean cullFaceOn = glIsEnabled(GL_CULL_FACE);
    
    if (drawVolumeSlicesFlag) {
        /*
         * Disable culling so that both sides of the triangles/quads are drawn.
         */
        glDisable(GL_CULL_FACE);
        
        switch (sliceProjectionType) {
            case VolumeSliceProjectionTypeEnum::VOLUME_SLICE_PROJECTION_ORTHOGONAL:
                CaretAssert(0);
                break;
            case VolumeSliceProjectionTypeEnum::VOLUME_SLICE_PROJECTION_OBLIQUE:
            {
                /*
                 * Create the oblique slice transformation matrix
                 */
                Matrix4x4 obliqueTransformationMatrix;
                createObliqueTransformationMatrix(sliceCoordinates,
                                                  obliqueTransformationMatrix);
                
                if (DeveloperFlagsEnum::isFlag(DeveloperFlagsEnum::DEVELOPER_FLAG_OBLIQUE_SLICE_OUTLINE)) {
                    drawObliqueSliceWithOutlines(sliceViewPlane,
                                                 obliqueTransformationMatrix);
                }
                else {
                    drawObliqueSlice(sliceViewPlane,
                                     obliqueTransformationMatrix,
                                     slicePlane);
                }
            }
                break;
        }

        /*
         * Process selection
         */
        if (m_identificationModeFlag) {
            processIdentification();
        }
    }
    
    if ( ! m_identificationModeFlag) {
        if (slicePlane.isValidPlane()) {
            drawLayers(sliceDrawingType,
                       sliceProjectionType,
                       sliceViewPlane,
                       slicePlane,
                       sliceCoordinates);
        }
    }
    
    /*
     * Draw model space annotaitons on the volume slice
     */
    float sliceThickness = 1.0;
    if ( ! m_volumeDrawInfo.empty()) {
        if (m_volumeDrawInfo[0].volumeFile != NULL) {
            float spaceX = 0.0, spaceY = 0.0, spaceZ = 0.0;
            m_volumeDrawInfo[0].volumeFile->getVoxelSpacing(spaceX, spaceY, spaceZ);
            
            switch (sliceViewPlane) {
                case VolumeSliceViewPlaneEnum::ALL:
                    CaretAssert(0);
                    break;
                case VolumeSliceViewPlaneEnum::AXIAL:
                    sliceThickness = spaceZ;
                    break;
                case VolumeSliceViewPlaneEnum::CORONAL:
                    sliceThickness = spaceY;
                    break;
                case VolumeSliceViewPlaneEnum::PARASAGITTAL:
                    sliceThickness = spaceX;
                    break;
            }
        }
    }
    BrainOpenGLAnnotationDrawingFixedPipeline::Inputs inputs(this->m_brain,
                                                             m_fixedPipelineDrawing->mode,
                                                             BrainOpenGLFixedPipeline::s_gluLookAtCenterFromEyeOffsetDistance,
                                                             m_fixedPipelineDrawing->m_windowIndex,
                                                             m_fixedPipelineDrawing->windowTabIndex,
                                                             BrainOpenGLAnnotationDrawingFixedPipeline::Inputs::WINDOW_DRAWING_NO);
    m_fixedPipelineDrawing->m_annotationDrawing->drawModelSpaceAnnotationsOnVolumeSlice(&inputs,
                                                                                        slicePlane,
                                                                                        sliceThickness);
    
    m_fixedPipelineDrawing->disableClippingPlanes();
    
    
    if (cullFaceOn) {
        glEnable(GL_CULL_FACE);
    }
}

/**
 * Draw an oblique slice.
 *
 * @param sliceViewPlane
 *    The plane for slice drawing.
 * @param transformationMatrix
 *    The for oblique viewing.
 * @param plane
 *    Plane equation for the selected slice.
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::drawObliqueSlice(const VolumeSliceViewPlaneEnum::Enum sliceViewPlane,
                                              Matrix4x4& transformationMatrix,
                                              const Plane& plane)
{
    /*
     * When performing voxel identification for editing voxels,
     * we need to draw EVERY voxel since the user may click
     * regions where the voxels are "off".
     */
    float voxelEditingValue = 1.0;
    VolumeFile* voxelEditingVolumeFile = NULL;
    bool volumeEditingDrawAllVoxelsFlag = false;
    if (m_identificationModeFlag) {
        SelectionItemVoxelEditing* voxelEditID = m_brain->getSelectionManager()->getVoxelEditingIdentification();
        if (voxelEditID->isEnabledForSelection()) {
            voxelEditingVolumeFile = voxelEditID->getVolumeFileForEditing();
            if (voxelEditingVolumeFile != NULL) {
                volumeEditingDrawAllVoxelsFlag = true;
                if (voxelEditingVolumeFile->isMappedWithLabelTable()) {
                    if (voxelEditingVolumeFile->getNumberOfMaps() > 0) {
                        voxelEditingValue = voxelEditingVolumeFile->getMapLabelTable(0)->getUnassignedLabelKey();
                    }
                }
            }
        }
    }
    
    const bool obliqueSliceModeThreeDimFlag = false;
    
    float m[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, m);
    Matrix4x4 tm;
    tm.setMatrixFromOpenGL(m);
    
    const int32_t numVolumes = static_cast<int32_t>(m_volumeDrawInfo.size());
    
    /*
     * Get the maximum bounds of the voxels from all slices
     * and the smallest voxel spacing
     */
    float voxelBounds[6];
    float voxelSpacing[3];
    if (false == getVoxelCoordinateBoundsAndSpacing(voxelBounds,
                                                    voxelSpacing)) {
        return;
    }
    float voxelSize = std::min(voxelSpacing[0],
                               std::min(voxelSpacing[1],
                                        voxelSpacing[2]));
    
    /*
     * Use a larger voxel size for the 3D view in volume slice viewing
     * since it draws all three slices and this takes time
     */
    if (obliqueSliceModeThreeDimFlag) {
        voxelSize *= 3.0;
    }
    
    /*
     * Look at point is in center of volume
     */
    float translation[3];
    m_browserTabContent->getTranslation(translation);
    float viewOffsetX = 0.0;
    float viewOffsetY = 0.0;
    switch (sliceViewPlane) {
        case VolumeSliceViewPlaneEnum::ALL:
            CaretAssert(0);
            break;
        case VolumeSliceViewPlaneEnum::AXIAL:
            viewOffsetX = (m_lookAtCenter[0] + translation[0]);
            viewOffsetY = (m_lookAtCenter[1] + translation[1]);
            break;
        case VolumeSliceViewPlaneEnum::CORONAL:
            viewOffsetX = (m_lookAtCenter[0] + translation[0]);
            viewOffsetY = (m_lookAtCenter[2] + translation[2]);
            break;
        case VolumeSliceViewPlaneEnum::PARASAGITTAL:
            viewOffsetX = (m_lookAtCenter[1] + translation[1]);
            viewOffsetY = (m_lookAtCenter[2] + translation[2]);
            break;
    }
    
    float minScreenX = m_orthographicBounds[0] - viewOffsetX;
    float maxScreenX = m_orthographicBounds[1] - viewOffsetX;
    float minScreenY = m_orthographicBounds[2] - viewOffsetY;
    float maxScreenY = m_orthographicBounds[3] - viewOffsetY;
    
    
    /*
     * Get origin voxel IJK
     */
    const float zeroXYZ[3] = { 0.0, 0.0, 0.0 };
    int64_t originIJK[3];
    m_volumeDrawInfo[0].volumeFile->enclosingVoxel(zeroXYZ[0], zeroXYZ[1], zeroXYZ[2],
                                                   originIJK[0], originIJK[1], originIJK[2]);
    
    
    /*
     * Get XYZ center of origin Voxel
     */
    float originVoxelXYZ[3];
    m_volumeDrawInfo[0].volumeFile->indexToSpace(originIJK, originVoxelXYZ);
    float actualOrigin[3];
    m_volumeDrawInfo[0].volumeFile->indexToSpace(originIJK, actualOrigin);
    
    float screenOffsetX = 0.0;
    float screenOffsetY = 0.0;
    float originOffsetX = 0.0;
    float originOffsetY = 0.0;
    switch (sliceViewPlane) {
        case VolumeSliceViewPlaneEnum::ALL:
            CaretAssert(0);
            break;
        case VolumeSliceViewPlaneEnum::AXIAL:
            screenOffsetX = m_lookAtCenter[0];
            screenOffsetY = m_lookAtCenter[1];
            originOffsetX = actualOrigin[0];
            originOffsetY = actualOrigin[1];
            break;
        case VolumeSliceViewPlaneEnum::CORONAL:
            screenOffsetX = m_lookAtCenter[0];
            screenOffsetY = m_lookAtCenter[2];
            originOffsetX = actualOrigin[0];
            originOffsetY = actualOrigin[2];
            break;
        case VolumeSliceViewPlaneEnum::PARASAGITTAL:
            screenOffsetX = m_lookAtCenter[1];
            screenOffsetY = m_lookAtCenter[2];
            originOffsetX = actualOrigin[1];
            originOffsetY = actualOrigin[2];
            break;
    }
    
    const int32_t alignVoxelsFlag = 1;
    if (alignVoxelsFlag == 1) {
        /*
         * Adjust for when selected slices are not at the origin
         */
        const float xOffset = MathFunctions::remainder(screenOffsetX, voxelSize);
        const float yOffset = MathFunctions::remainder(screenOffsetY, voxelSize);
        originOffsetX -= xOffset;
        originOffsetY -= yOffset;
        
        const int64_t numVoxelsToLeft = static_cast<int64_t>(MathFunctions::round(minScreenX + originOffsetX) / voxelSize);
        const int64_t numVoxelsToRight = static_cast<int64_t>(MathFunctions::round(maxScreenX + originOffsetX) / voxelSize);
        const int64_t numVoxelsToBottom = static_cast<int64_t>(MathFunctions::round(minScreenY + originOffsetY) / voxelSize);
        const int64_t numVoxelsToTop = static_cast<int64_t>(MathFunctions::round(maxScreenY + originOffsetY)/ voxelSize);
        
        const float halfVoxel = voxelSize / 2.0;
        
        const float firstVoxelCenterX = (numVoxelsToLeft * voxelSize) + originOffsetX;
        const float lastVoxelCenterX = (numVoxelsToRight * voxelSize) + originOffsetX;
        
        const float firstVoxelCenterY = (numVoxelsToBottom * voxelSize) + originOffsetY;
        const float lastVoxelCenterY = (numVoxelsToTop * voxelSize) + originOffsetY;
        
        float newMinScreenX = firstVoxelCenterX - halfVoxel;
        float newMaxScreenX = lastVoxelCenterX + halfVoxel;
        float newMinScreenY = firstVoxelCenterY - halfVoxel;
        float newMaxScreenY = lastVoxelCenterY + halfVoxel;
        
        if (debugFlag) {
            const AString msg2 = ("Origin Voxel Coordinate: ("
                                  + AString::fromNumbers(actualOrigin, 3, ",")
                                  + "\n   Oblique Screen X: ("
                                  + AString::number(minScreenX)
                                  + ","
                                  + AString::number(maxScreenX)
                                  + ") Y: ("
                                  + AString::number(minScreenY)
                                  + ","
                                  + AString::number(maxScreenY)
                                  + ")\nNew X: ("
                                  + AString::number(newMinScreenX)
                                  + ","
                                  + AString::number(newMaxScreenX)
                                  + ") Y: ("
                                  + AString::number(newMinScreenY)
                                  + ","
                                  + AString::number(newMaxScreenY)
                                  + ") Diff: ("
                                  + AString::number((newMaxScreenX - newMinScreenX) / voxelSize)
                                  + ","
                                  + AString::number((newMaxScreenY - newMinScreenY) / voxelSize)
                                  + ")");
            std::cout << qPrintable(msg2) << std::endl;
        }
        
        minScreenX = newMinScreenX;
        maxScreenX = newMaxScreenX;
        minScreenY = newMinScreenY;
        maxScreenY = newMaxScreenY;
    }
    
    
    if (alignVoxelsFlag == 2) {
        const float quarterVoxelSize = voxelSize / 4.0;
        float newMinScreenX = (static_cast<int64_t>(minScreenX / voxelSize) * voxelSize) + quarterVoxelSize;
        float newMaxScreenX = (static_cast<int64_t>(maxScreenX / voxelSize) * voxelSize) - quarterVoxelSize;
        float newMinScreenY = (static_cast<int64_t>(minScreenY / voxelSize) * voxelSize) + quarterVoxelSize;
        float newMaxScreenY = (static_cast<int64_t>(maxScreenY / voxelSize) * voxelSize) - quarterVoxelSize;
        
        minScreenX = newMinScreenX;
        maxScreenX = newMaxScreenX;
        minScreenY = newMinScreenY;
        maxScreenY = newMaxScreenY;
    }
    
    /*
     * Set the corners of the screen for the respective view
     */
    float bottomLeft[3];
    float bottomRight[3];
    float topRight[3];
    float topLeft[3];
    switch (sliceViewPlane) {
        case VolumeSliceViewPlaneEnum::ALL:
            CaretAssert(0);
            break;
        case VolumeSliceViewPlaneEnum::AXIAL:
            bottomLeft[0] = minScreenX;
            bottomLeft[1] = minScreenY;
            bottomLeft[2] = 0.0;
            bottomRight[0] = maxScreenX;
            bottomRight[1] = minScreenY;
            bottomRight[2] = 0.0;
            topRight[0] = maxScreenX;
            topRight[1] = maxScreenY;
            topRight[2] = 0.0;
            topLeft[0] = minScreenX;
            topLeft[1] = maxScreenY;
            topLeft[2] = 0.0;
            break;
        case VolumeSliceViewPlaneEnum::CORONAL:
            bottomLeft[0] = minScreenX;
            bottomLeft[1] = 0.0;
            bottomLeft[2] = minScreenY;
            bottomRight[0] = maxScreenX;
            bottomRight[1] = 0.0;
            bottomRight[2] = minScreenY;
            topRight[0] = maxScreenX;
            topRight[1] = 0.0;
            topRight[2] = maxScreenY;
            topLeft[0] = minScreenX;
            topLeft[1] = 0.0;
            topLeft[2] = maxScreenY;
            break;
        case VolumeSliceViewPlaneEnum::PARASAGITTAL:
            bottomLeft[0] = 0.0;
            bottomLeft[1] = minScreenX;
            bottomLeft[2] = minScreenY;
            bottomRight[0] = 0.0;
            bottomRight[1] = maxScreenX;
            bottomRight[2] = minScreenY;
            topRight[0] = 0.0;
            topRight[1] = maxScreenX;
            topRight[2] = maxScreenY;
            topLeft[0] = 0.0;
            topLeft[1] = minScreenX;
            topLeft[2] = maxScreenY;
            break;
    }
    
    
    /*
     * Transform the corners of the screen into model coordinates
     */
    transformationMatrix.multiplyPoint3(bottomLeft);
    transformationMatrix.multiplyPoint3(bottomRight);
    transformationMatrix.multiplyPoint3(topRight);
    transformationMatrix.multiplyPoint3(topLeft);
    
    if (debugFlag) {
        const double bottomDist = MathFunctions::distance3D(bottomLeft, bottomRight);
        const double topDist = MathFunctions::distance3D(topLeft, topRight);
        const double bottomVoxels = bottomDist / voxelSize;
        const double topVoxels = topDist / voxelSize;
        const AString msg = ("Bottom Dist: "
                             + AString::number(bottomDist)
                             + " voxel size: "
                             + AString::number(bottomVoxels)
                             + " Top Dist: "
                             + AString::number(bottomDist)
                             + " voxel size: "
                             + AString::number(topVoxels));
        std::cout << qPrintable(msg) << std::endl;
    }
    
    if (debugFlag) {
        m_fixedPipelineDrawing->setLineWidth(3.0);
        glColor3f(1.0, 0.0, 0.0);
        glBegin(GL_LINE_LOOP);
        glVertex3fv(bottomLeft);
        glVertex3fv(bottomRight);
        glVertex3fv(topRight);
        glVertex3fv(topLeft);
        glEnd();
    }
    
    /*
     * Unit vector and distance in model coords along left side of screen
     */
    double bottomLeftToTopLeftUnitVector[3] = {
        topLeft[0] - bottomLeft[0],
        topLeft[1] - bottomLeft[1],
        topLeft[2] - bottomLeft[2],
    };
    MathFunctions::normalizeVector(bottomLeftToTopLeftUnitVector);
    const double bottomLeftToTopLeftDistance = MathFunctions::distance3D(bottomLeft,
                                                                         topLeft);
    
    /*
     * Unit vector and distance in model coords along right side of screen
     */
    double bottomRightToTopRightUnitVector[3] = {
        topRight[0] - bottomRight[0],
        topRight[1] - bottomRight[1],
        topRight[2] - bottomRight[2]
    };
    MathFunctions::normalizeVector(bottomRightToTopRightUnitVector);
    const double bottomRightToTopRightDistance = MathFunctions::distance3D(bottomRight,
                                                                           topRight);
    /*
     * For fastest coloring, need to color data values as a group
     */
    std::vector<VolumeSlice> volumeSlices;
    for (int32_t i = 0; i < numVolumes; i++) {
        volumeSlices.push_back(VolumeSlice(m_volumeDrawInfo[i].volumeFile,
                                           m_volumeDrawInfo[i].mapIndex,
                                           m_volumeDrawInfo[i].opacity));
        
    }
    bool showFirstVoxelCoordFlag = debugFlag;
    
    /*
     * Track voxels that will be drawn
     */
    std::vector<VoxelToDraw*> voxelsToDraw;
    
    if ((bottomLeftToTopLeftDistance > 0)
        && (bottomRightToTopRightDistance > 0)) {
        
        const double bottomLeftToTopLeftStep = voxelSize;
        const double numLeftSteps = (bottomLeftToTopLeftDistance / bottomLeftToTopLeftStep);
        
        const double bottomRightToTopRightStep = (bottomRightToTopRightDistance
                                                  / numLeftSteps);
        
        const double dtVertical = bottomLeftToTopLeftStep / bottomLeftToTopLeftDistance;
        
        /*
         * Voxels are drawn in rows, left to right, across the screen,
         * starting at the bottom.
         */
        double leftEdgeBottomCoord[3];
        double leftEdgeTopCoord[3];
        double rightEdgeBottomCoord[3];
        double rightEdgeTopCoord[3];
        for (double tVertical = 0.0, dLeft = 0.0, dRight = 0.0;
             tVertical < 1.0;
             tVertical += dtVertical, dLeft += bottomLeftToTopLeftStep, dRight += bottomRightToTopRightStep) {
            /*
             * Coordinate on left edge at BOTTOM of current row
             */
            leftEdgeBottomCoord[0] = bottomLeft[0] + (dLeft * bottomLeftToTopLeftUnitVector[0]);
            leftEdgeBottomCoord[1] = bottomLeft[1] + (dLeft * bottomLeftToTopLeftUnitVector[1]);
            leftEdgeBottomCoord[2] = bottomLeft[2] + (dLeft * bottomLeftToTopLeftUnitVector[2]);
            
            /*
             * Coordinate on right edge at BOTTOM of current row
             */
            rightEdgeBottomCoord[0] = bottomRight[0] + (dRight * bottomRightToTopRightUnitVector[0]);
            rightEdgeBottomCoord[1] = bottomRight[1] + (dRight * bottomRightToTopRightUnitVector[1]);
            rightEdgeBottomCoord[2] = bottomRight[2] + (dRight * bottomRightToTopRightUnitVector[2]);
            
            /*
             * Coordinate on left edge at TOP of current row
             */
            leftEdgeTopCoord[0] = bottomLeft[0] + ((dLeft + bottomLeftToTopLeftStep) * bottomLeftToTopLeftUnitVector[0]);
            leftEdgeTopCoord[1] = bottomLeft[1] + ((dLeft + bottomLeftToTopLeftStep) * bottomLeftToTopLeftUnitVector[1]);
            leftEdgeTopCoord[2] = bottomLeft[2] + ((dLeft + bottomLeftToTopLeftStep) * bottomLeftToTopLeftUnitVector[2]);
            
            /*
             * Coordinate on right edge at TOP of current row
             */
            rightEdgeTopCoord[0] = bottomRight[0] + ((dRight + bottomRightToTopRightStep) * bottomRightToTopRightUnitVector[0]);
            rightEdgeTopCoord[1] = bottomRight[1] + ((dRight + bottomRightToTopRightStep) * bottomRightToTopRightUnitVector[1]);
            rightEdgeTopCoord[2] = bottomRight[2] + ((dRight + bottomRightToTopRightStep) * bottomRightToTopRightUnitVector[2]);
            
            /*
             * Determine change in XYZ per voxel along the bottom of the current row
             */
            const double bottomVoxelEdgeDistance = MathFunctions::distance3D(leftEdgeBottomCoord,
                                                                             rightEdgeBottomCoord);
            double bottomEdgeUnitVector[3];
            MathFunctions::createUnitVector(leftEdgeBottomCoord, rightEdgeBottomCoord, bottomEdgeUnitVector);
            const double numVoxelsInRowFloat = bottomVoxelEdgeDistance / voxelSize;
            const int64_t numVoxelsInRow = MathFunctions::round(numVoxelsInRowFloat);
            const double bottomEdgeVoxelSize = bottomVoxelEdgeDistance / numVoxelsInRow;
            const double bottomVoxelEdgeDX = bottomEdgeVoxelSize * bottomEdgeUnitVector[0];
            const double bottomVoxelEdgeDY = bottomEdgeVoxelSize * bottomEdgeUnitVector[1];
            const double bottomVoxelEdgeDZ = bottomEdgeVoxelSize * bottomEdgeUnitVector[2];
            
            /*
             * Determine change in XYZ per voxel along top of the current row
             */
            const double topVoxelEdgeDistance = MathFunctions::distance3D(leftEdgeTopCoord,
                                                                          rightEdgeTopCoord);
            double topEdgeUnitVector[3];
            MathFunctions::createUnitVector(leftEdgeTopCoord, rightEdgeTopCoord, topEdgeUnitVector);
            const double topEdgeVoxelSize = topVoxelEdgeDistance / numVoxelsInRow;
            const double topVoxelEdgeDX = topEdgeVoxelSize * topEdgeUnitVector[0];
            const double topVoxelEdgeDY = topEdgeVoxelSize * topEdgeUnitVector[1];
            const double topVoxelEdgeDZ = topEdgeVoxelSize * topEdgeUnitVector[2];
            
            /*
             * Initialize bottom and top left coordinate of first voxel in row
             */
            double bottomLeftVoxelCoord[3] = {
                leftEdgeBottomCoord[0],
                leftEdgeBottomCoord[1],
                leftEdgeBottomCoord[2]
            };
            double topLeftVoxelCoord[3] = {
                leftEdgeTopCoord[0],
                leftEdgeTopCoord[1],
                leftEdgeTopCoord[2]
            };
            
            const bool useInterpolatedVoxel = true;
            
            /*
             * Draw the voxels in the row
             */
            for (int64_t i = 0; i < numVoxelsInRow; i++) {
                /*
                 * Top right corner of voxel
                 */
                const double topRightVoxelCoord[3] = {
                    topLeftVoxelCoord[0] + topVoxelEdgeDX,
                    topLeftVoxelCoord[1] + topVoxelEdgeDY,
                    topLeftVoxelCoord[2] + topVoxelEdgeDZ
                };
                
                const float voxelCenter[3] = {
                    (float)((bottomLeftVoxelCoord[0] + topRightVoxelCoord[0]) * 0.5),
                    (float)((bottomLeftVoxelCoord[1] + topRightVoxelCoord[1]) * 0.5),
                    (float)((bottomLeftVoxelCoord[2] + topRightVoxelCoord[2]) * 0.5)
                };
                
                
                bool printOriginVoxelInfo = false;
                if (debugFlag) {
                    switch (sliceViewPlane) {
                        case VolumeSliceViewPlaneEnum::ALL:
                            break;
                        case VolumeSliceViewPlaneEnum::AXIAL:
                            if (showFirstVoxelCoordFlag) {
                                const float dist = voxelCenter[0] - actualOrigin[0];
                                const AString msg = ("First Voxel Center: "
                                                     + AString::fromNumbers(voxelCenter, 3, ",")
                                                     + " Dist from origin voxel in X: "
                                                     + AString::number(dist)
                                                     + " Number of voxels between: "
                                                     + AString::number(dist / voxelSize));
                                std::cout << qPrintable(msg) << std::endl;
                                showFirstVoxelCoordFlag = false;
                            }
                            if ((bottomLeftVoxelCoord[0] < actualOrigin[0])
                                && (topRightVoxelCoord[0] > actualOrigin[0])) {
                                if ((bottomLeftVoxelCoord[1] < actualOrigin[1])
                                    && (topRightVoxelCoord[1] > actualOrigin[1])) {
                                    printOriginVoxelInfo = true;
                                }
                            }
                            break;
                        case VolumeSliceViewPlaneEnum::CORONAL:
                            if (showFirstVoxelCoordFlag) {
                                const float dist = voxelCenter[0] - actualOrigin[0];
                                const AString msg = ("First Voxel Center: "
                                                     + AString::fromNumbers(voxelCenter, 3, ",")
                                                     + " Dist from origin voxel in X: "
                                                     + AString::number(dist)
                                                     + " Number of voxels between: "
                                                     + AString::number(dist / voxelSize));
                                std::cout << qPrintable(msg) << std::endl;
                                showFirstVoxelCoordFlag = false;
                            }
                            if ((bottomLeftVoxelCoord[0] < actualOrigin[0])
                                && (topRightVoxelCoord[0] > actualOrigin[0])) {
                                if ((bottomLeftVoxelCoord[2] < actualOrigin[2])
                                    && (topRightVoxelCoord[2] > actualOrigin[2])) {
                                    printOriginVoxelInfo = true;
                                }
                            }
                            break;
                        case VolumeSliceViewPlaneEnum::PARASAGITTAL:
                            if (showFirstVoxelCoordFlag) {
                                const float dist = voxelCenter[1] - actualOrigin[1];
                                const AString msg = ("First Voxel Center: "
                                                     + AString::fromNumbers(voxelCenter, 3, ",")
                                                     + " Dist from origin voxel in Y: "
                                                     + AString::number(dist)
                                                     + " Number of voxels between: "
                                                     + AString::number(dist / voxelSize));
                                std::cout << qPrintable(msg) << std::endl;
                                showFirstVoxelCoordFlag = false;
                            }
                            if ((bottomLeftVoxelCoord[1] < actualOrigin[1])
                                && (topRightVoxelCoord[1] > actualOrigin[1])) {
                                if ((bottomLeftVoxelCoord[2] < actualOrigin[2])
                                    && (topRightVoxelCoord[2] > actualOrigin[2])) {
                                    printOriginVoxelInfo = true;
                                }
                            }
                            break;
                    }
                }
                if (printOriginVoxelInfo) {
                    const AString msg = ("Origin voxel center when drawn is "
                                         + AString::fromNumbers(voxelCenter, 3, ",")
                                         + " but should be "
                                         + AString::fromNumbers(actualOrigin, 3, ",")
                                         + " Voxel Corners: ("
                                         + AString::fromNumbers(bottomLeftVoxelCoord, 3, ",")
                                         + ") ("
                                         + AString::fromNumbers(topRightVoxelCoord, 3, ",")
                                         + ")");
                    std::cout << qPrintable(msg) << std::endl;
                }
                
                /*
                 * Loop through the volumes selected as overlays.
                 */
                VoxelToDraw* voxelDrawingInfo = NULL;
                
                for (int32_t iVol = 0; iVol < numVolumes; iVol++) {
                    const BrainOpenGLFixedPipeline::VolumeDrawInfo& vdi = m_volumeDrawInfo[iVol];
                    const VolumeMappableInterface* volInter = vdi.volumeFile;
                    const VolumeFile* volumeFile = volumeSlices[iVol].m_volumeFile;
                    
                    float values[4] = { 0.0, 0.0, 0.0, 0.0 };
                    bool valueValidFlag = false;
                    
                    bool isPaletteMappedVolumeFile = false;
                    bool isRgbVolumeFile           = false;
                    bool isRgbaVolumeFile          = false;
                    if (volumeFile != NULL) {
                        if (volumeFile->isMappedWithPalette()) {
                            isPaletteMappedVolumeFile = true;
                        }
                        else if (volumeFile->isMappedWithRGBA()) {
                            if (volumeFile->getNumberOfComponents() == 4) {
                                isRgbaVolumeFile = true;
                            }
                            else if (volumeFile->getNumberOfComponents() == 3) {
                                isRgbVolumeFile = true;
                            }
                        }
                    }
                    const CiftiMappableDataFile* ciftiMappableFile = volumeSlices[iVol].m_ciftiMappableDataFile;
                    
                    if (useInterpolatedVoxel
                        && isPaletteMappedVolumeFile) {
                        values[0] = volumeFile->interpolateValue(voxelCenter,
                                                             VolumeFile::CUBIC,
                                                             &valueValidFlag,
                                                             vdi.mapIndex);
                        
                        if (valueValidFlag
                            && (values[0] != 0.0f)) {
                            /*
                             * Apply masking to oblique voxels (WB-750).
                             * In some instances, CUBIC interpolation may result in a voxel
                             * receiving a very small value (0.000000000000000210882405) and 
                             * this will cause the slice drawing to look very unusual.  Masking
                             * is used to prevent this from occurring.
                             *
                             */
                            bool maskValidFlag = false;
                            float maskValue = 0.0f;
                            switch (m_obliqueSliceMaskingType) {
                                case VolumeSliceInterpolationEdgeEffectsMaskingEnum::OFF:
                                    maskValidFlag = false;
                                    break;
                                case VolumeSliceInterpolationEdgeEffectsMaskingEnum::LOOSE:
                                    maskValue = volumeFile->interpolateValue(voxelCenter,
                                                                             VolumeFile::TRILINEAR,
                                                                             &maskValidFlag,
                                                                             vdi.mapIndex);
                                    break;
                                case VolumeSliceInterpolationEdgeEffectsMaskingEnum::TIGHT:
                                    maskValue = volumeFile->interpolateValue(voxelCenter,
                                                                             VolumeFile::ENCLOSING_VOXEL,
                                                                             &maskValidFlag,
                                                                             vdi.mapIndex);
                                    break;
                            }
                            
                            if (maskValidFlag
                                && (maskValue == 0.0f)) {
                                values[0] = 0.0f;
                                valueValidFlag = false;
                            }
                        }
                    }
                    else if (ciftiMappableFile != NULL) {
                        const int64_t voxelOffset = ciftiMappableFile->getMapDataOffsetForVoxelAtCoordinate(voxelCenter,
                                                                                                            vdi.mapIndex);
                        if (voxelOffset >= 0) {
                            CaretAssertVectorIndex(m_ciftiMappableFileData, iVol);
                            const std::vector<float>& data = m_ciftiMappableFileData[iVol];
                            CaretAssertVectorIndex(data, voxelOffset);
                            values[0] = data[voxelOffset];
                            valueValidFlag = true;
                        }
                    }
                    else if (isRgbVolumeFile
                             || isRgbaVolumeFile) {
                        values[0] = volInter->getVoxelValue(voxelCenter,
                                                            &valueValidFlag,
                                                            vdi.mapIndex,
                                                            0);
                        values[1] = volInter->getVoxelValue(voxelCenter,
                                                            &valueValidFlag,
                                                            vdi.mapIndex,
                                                            1);
                        values[2] = volInter->getVoxelValue(voxelCenter,
                                                            &valueValidFlag,
                                                            vdi.mapIndex,
                                                            2);
                        if (isRgbaVolumeFile) {
                            values[3] = volInter->getVoxelValue(voxelCenter,
                                                                &valueValidFlag,
                                                                vdi.mapIndex,
                                                                3);
                        }
                        else {
                            values[3] = 1.0;
                        }
                    }
                    else {
                        values[0] = volInter->getVoxelValue(voxelCenter,
                                                            &valueValidFlag,
                                                            vdi.mapIndex);
                    }
                    
                    /*
                     * Need to draw all voxels when editing
                     */
                    if (volumeEditingDrawAllVoxelsFlag) {
                        if (! valueValidFlag) {
                            if (volumeFile != NULL) {
                                if (volumeFile == voxelEditingVolumeFile) {
                                    values[0] = voxelEditingValue;
                                    valueValidFlag = true;
                                }
                            }
                        }
                    }
                    
                    if (valueValidFlag) {
                        if (voxelDrawingInfo == NULL) {
                            /*
                             * Bottom right corner of voxel
                             */
                            const double bottomRightVoxelCoord[3] = {
                                bottomLeftVoxelCoord[0] + bottomVoxelEdgeDX,
                                bottomLeftVoxelCoord[1] + bottomVoxelEdgeDY,
                                bottomLeftVoxelCoord[2] + bottomVoxelEdgeDZ
                            };
                            
                            /*
                             * Top right corner of voxel
                             */
                            const double topRightVoxelCoord[3] = {
                                topLeftVoxelCoord[0] + topVoxelEdgeDX,
                                topLeftVoxelCoord[1] + topVoxelEdgeDY,
                                topLeftVoxelCoord[2] + topVoxelEdgeDZ
                            };
                            
                            voxelDrawingInfo = new VoxelToDraw(voxelCenter,
                                                               bottomLeftVoxelCoord,
                                                               bottomRightVoxelCoord,
                                                               topRightVoxelCoord,
                                                               topLeftVoxelCoord);
                            voxelsToDraw.push_back(voxelDrawingInfo);
                        }
                        
                        const int64_t offset = ((isRgbVolumeFile|| isRgbaVolumeFile)
                                                ? volumeSlices[iVol].addValuesRGBA(values)
                                                : volumeSlices[iVol].addValue(values[0]));
                        voxelDrawingInfo->addVolumeValue(iVol, offset);
                    }
                }
                
                /*
                 * Move to the next voxel in the row
                 */
                bottomLeftVoxelCoord[0] += bottomVoxelEdgeDX;
                bottomLeftVoxelCoord[1] += bottomVoxelEdgeDY;
                bottomLeftVoxelCoord[2] += bottomVoxelEdgeDZ;
                topLeftVoxelCoord[0] += topVoxelEdgeDX;
                topLeftVoxelCoord[1] += topVoxelEdgeDY;
                topLeftVoxelCoord[2] += topVoxelEdgeDZ;
            }
        }
    }
    
    const int32_t browserTabIndex = m_browserTabContent->getTabNumber();
    const DisplayPropertiesLabels* displayPropertiesLabels = m_brain->getDisplayPropertiesLabels();
    const DisplayGroupEnum::Enum displayGroup = displayPropertiesLabels->getDisplayGroupForTab(browserTabIndex);
    
    /*
     * Color voxel values
     */
    for (int32_t i = 0; i < numVolumes; i++) {
        const int64_t numValues = static_cast<int64_t>(volumeSlices[i].m_values.size());
        if (numValues > 0) {
            volumeSlices[i].allocateColors();
            
            VolumeMappableInterface* volume = volumeSlices[i].m_volumeMappableInterface;
            CaretMappableDataFile* mappableFile = dynamic_cast<CaretMappableDataFile*>(volume);
            VolumeFile* volumeFile = volumeSlices[i].m_volumeFile;
            const bool isRgbaFlag =  ( ! volumeSlices[i].m_valuesGreen.empty());
        
            CaretAssert(mappableFile);
            const int32_t mapIndex = volumeSlices[i].m_mapIndex;
            const float* values = &volumeSlices[i].m_values[0];
            uint8_t* rgba = &volumeSlices[i].m_rgba[0];
            
            if (volumeEditingDrawAllVoxelsFlag
                && (volumeFile == voxelEditingVolumeFile)) {
                for (int64_t i = 0; i < numValues; i++) {
                    const int64_t i4 = i * 4;
                    rgba[i4] = 255;
                    rgba[i4+1] = 255;
                    rgba[i4+2] = 255;
                    rgba[i4+3] = 255;
                }
            }
            else if (mappableFile->isMappedWithPalette()) {
                const PaletteColorMapping* paletteColorMapping = mappableFile->getMapPaletteColorMapping(mapIndex);
                CaretAssertVectorIndex(m_volumeDrawInfo, i);
                NodeAndVoxelColoring::colorScalarsWithPalette(m_volumeDrawInfo[i].statistics,
                                                              paletteColorMapping,
                                                              values,
                                                              paletteColorMapping,
                                                              values,
                                                              numValues,
                                                              rgba);
            }
            else if (mappableFile->isMappedWithLabelTable()) {
                GiftiLabelTable* labelTable = mappableFile->getMapLabelTable(mapIndex);
                NodeAndVoxelColoring::colorIndicesWithLabelTableForDisplayGroupTab(labelTable,
                                                                                   values,
                                                                                   numValues,
                                                                                   displayGroup,
                                                                                   browserTabIndex,
                                                                                   rgba);
            }
            else if (isRgbaFlag) {
                const uint8_t rgbThreshold[3] = { 5, 5, 5 };
                CaretAssert(volumeSlices[i].m_values.size() == volumeSlices[i].m_valuesGreen.size());
                CaretAssert(volumeSlices[i].m_values.size() == volumeSlices[i].m_valuesBlue.size());
                CaretAssert(volumeSlices[i].m_values.size() == volumeSlices[i].m_valuesAlpha.size());
                NodeAndVoxelColoring::colorScalarsWithRGBA(&volumeSlices[i].m_values[0],
                                                           &volumeSlices[i].m_valuesGreen[0],
                                                           &volumeSlices[i].m_valuesBlue[0],
                                                           &volumeSlices[i].m_valuesAlpha[0],
                                                           numValues,
                                                           rgbThreshold,
                                                           rgba);
            }
            else {
                CaretAssert(0);
            }
        }
    }
    
    const int64_t numVoxelsToDraw = static_cast<int64_t>(voxelsToDraw.size());
    
    /*
     * quadCoords is the coordinates for all four corners of a 'quad'
     * that is used to draw a voxel.  quadRGBA is the colors for each
     * voxel drawn as a 'quad'.
     */
    std::vector<float> quadCoordsVector;
    std::vector<float> quadNormalsVector;
    std::vector<uint8_t> quadRGBAsVector;
    
    /*
     * Reserve space to avoid reallocations
     */
    const int64_t coordinatesPerQuad = 4;
    const int64_t componentsPerCoordinate = 3;
    const int64_t colorComponentsPerCoordinate = 4;
    quadCoordsVector.resize(numVoxelsToDraw
                            * coordinatesPerQuad
                            * componentsPerCoordinate);
    quadNormalsVector.resize(quadCoordsVector.size());
    quadRGBAsVector.resize(numVoxelsToDraw *
                           coordinatesPerQuad *
                           colorComponentsPerCoordinate);
    
    int64_t coordOffset = 0;
    int64_t normalOffset = 0;
    int64_t rgbaOffset = 0;
    
    float*   quadCoords  = &quadCoordsVector[0];
    float*   quadNormals = &quadNormalsVector[0];
    uint8_t* quadRGBAs   = &quadRGBAsVector[0];
    
    for (int64_t iVox = 0; iVox < numVoxelsToDraw; iVox++) {
        CaretAssertVectorIndex(voxelsToDraw, iVox);
        VoxelToDraw* vtd = voxelsToDraw[iVox];
        CaretAssert(vtd);
        
        uint8_t voxelRGBA[4] = { 0, 0, 0, 0 };
        
        const int32_t numSlicesForVoxel = static_cast<int32_t>(vtd->m_sliceIndices.size());
        for (int32_t iSlice = 0; iSlice < numSlicesForVoxel; iSlice++) {
            CaretAssertVectorIndex(vtd->m_sliceIndices, iSlice);
            CaretAssertVectorIndex(vtd->m_sliceOffsets, iSlice);
            const int32_t sliceIndex = vtd->m_sliceIndices[iSlice];
            const int64_t voxelOffset = vtd->m_sliceOffsets[iSlice];
            
            const uint8_t* rgba = volumeSlices[sliceIndex].getRgbaForValueByIndex(voxelOffset);
            if (rgba[3] > 0) {
                if (m_identificationModeFlag) {
                    voxelRGBA[0] = rgba[0];
                    voxelRGBA[1] = rgba[1];
                    voxelRGBA[2] = rgba[2];
                    voxelRGBA[3] = rgba[3];
                    
                    VolumeMappableInterface* volMap = volumeSlices[sliceIndex].m_volumeMappableInterface;
                    int64_t voxelI, voxelJ, voxelK;
                    volMap->enclosingVoxel(vtd->m_center[0], vtd->m_center[1], vtd->m_center[2],
                                           voxelI, voxelJ, voxelK);
                    
                    if (volMap->indexValid(voxelI, voxelJ, voxelK)) {
                        float diffXYZ[3];
                        vtd->getDiffXYZ(diffXYZ);
                        addVoxelToIdentification(sliceIndex,
                                                 volumeSlices[sliceIndex].m_mapIndex,
                                                 voxelI,
                                                 voxelJ,
                                                 voxelK,
                                                 diffXYZ,
                                                 voxelRGBA);
                    }
                }
                else {
                    /*
                     * In oblique drawing, each drawn "voxel" is composited from
                     * an interpolated voxel from each slice.  Therefore, we must
                     * do our own alpha (transparency) processing.
                     */
                    bool noAlphaFlag = true;
                    if (iSlice > 0) {
                        float alpha = volumeSlices[sliceIndex].m_opacity;
                        if (alpha < 1.0f) {
                            if ((voxelRGBA[3] > 0)
                                && (rgba[3] > 0)) {
                                const float oneMinusAlpha = 1.0f - alpha;
                                voxelRGBA[0] = static_cast<uint8_t>((rgba[0] * alpha) + (voxelRGBA[0] * oneMinusAlpha));
                                voxelRGBA[1] = static_cast<uint8_t>((rgba[1] * alpha) + (voxelRGBA[1] * oneMinusAlpha));
                                voxelRGBA[2] = static_cast<uint8_t>((rgba[2] * alpha) + (voxelRGBA[2] * oneMinusAlpha));
                                voxelRGBA[3] = 255;
                                noAlphaFlag = false;
                            }
                        }
                    }
                    
                    if (volumeEditingDrawAllVoxelsFlag
                        && (volumeSlices[sliceIndex].m_volumeFile == voxelEditingVolumeFile)) {
                        noAlphaFlag = true;
                    }
                    
                    if (noAlphaFlag) {
                        voxelRGBA[0] = rgba[0];
                        voxelRGBA[1] = rgba[1];
                        voxelRGBA[2] = rgba[2];
                        voxelRGBA[3] = rgba[3];
                    }
                }
            }
        }
        
        if (voxelRGBA[3] > 0) {
            float sliceNormalVector[3];
            plane.getNormalVector(sliceNormalVector);
            
            CaretAssertVectorIndex(quadRGBAsVector, rgbaOffset + 3);
            quadRGBAs[rgbaOffset]   = voxelRGBA[0];
            quadRGBAs[rgbaOffset+1] = voxelRGBA[1];
            quadRGBAs[rgbaOffset+2] = voxelRGBA[2];
            quadRGBAs[rgbaOffset+3] = voxelRGBA[3];
            rgbaOffset += 4;
            
            CaretAssertVectorIndex(quadNormalsVector, normalOffset + 2);
            quadNormals[normalOffset]   = sliceNormalVector[0];
            quadNormals[normalOffset+1] = sliceNormalVector[1];
            quadNormals[normalOffset+2] = sliceNormalVector[2];
            normalOffset += 3;
            
            CaretAssertVectorIndex(quadRGBAsVector, rgbaOffset + 3);
            quadRGBAs[rgbaOffset]   = voxelRGBA[0];
            quadRGBAs[rgbaOffset+1] = voxelRGBA[1];
            quadRGBAs[rgbaOffset+2] = voxelRGBA[2];
            quadRGBAs[rgbaOffset+3] = voxelRGBA[3];
            rgbaOffset += 4;
            
            CaretAssertVectorIndex(quadNormalsVector, normalOffset + 2);
            quadNormals[normalOffset]   = sliceNormalVector[0];
            quadNormals[normalOffset+1] = sliceNormalVector[1];
            quadNormals[normalOffset+2] = sliceNormalVector[2];
            normalOffset += 3;
            
            CaretAssertVectorIndex(quadRGBAsVector, rgbaOffset + 3);
            quadRGBAs[rgbaOffset]   = voxelRGBA[0];
            quadRGBAs[rgbaOffset+1] = voxelRGBA[1];
            quadRGBAs[rgbaOffset+2] = voxelRGBA[2];
            quadRGBAs[rgbaOffset+3] = voxelRGBA[3];
            rgbaOffset += 4;
            
            CaretAssertVectorIndex(quadNormalsVector, normalOffset + 2);
            quadNormals[normalOffset]   = sliceNormalVector[0];
            quadNormals[normalOffset+1] = sliceNormalVector[1];
            quadNormals[normalOffset+2] = sliceNormalVector[2];
            normalOffset += 3;
            
            CaretAssertVectorIndex(quadRGBAsVector, rgbaOffset + 3);
            quadRGBAs[rgbaOffset]   = voxelRGBA[0];
            quadRGBAs[rgbaOffset+1] = voxelRGBA[1];
            quadRGBAs[rgbaOffset+2] = voxelRGBA[2];
            quadRGBAs[rgbaOffset+3] = voxelRGBA[3];
            rgbaOffset += 4;
            
            CaretAssertVectorIndex(quadNormalsVector, normalOffset + 2);
            quadNormals[normalOffset]   = sliceNormalVector[0];
            quadNormals[normalOffset+1] = sliceNormalVector[1];
            quadNormals[normalOffset+2] = sliceNormalVector[2];
            normalOffset += 3;
            
            CaretAssertVectorIndex(quadCoordsVector, coordOffset + 11);
            for (int32_t iq = 0; iq < 12; iq++) {
                quadCoords[coordOffset + iq] = vtd->m_coordinates[iq];
            }
            coordOffset += 12;
        }
    }
    
    quadCoordsVector.resize(coordOffset);
    quadNormalsVector.resize(normalOffset);
    quadRGBAsVector.resize(rgbaOffset);
    
    for (std::vector<VoxelToDraw*>::iterator iter = voxelsToDraw.begin();
         iter != voxelsToDraw.end();
         iter++) {
        VoxelToDraw* vtd = *iter;
        delete vtd;
    }
    voxelsToDraw.clear();
    
    if ( ! quadCoordsVector.empty()) {
        glPushMatrix();
        BrainOpenGLPrimitiveDrawing::drawQuads(quadCoordsVector,
                                               quadNormalsVector,
                                               quadRGBAsVector);
        glPopMatrix();
    }
}

/**
 * Create the equation for the slice plane
 *
 * @param sliceProjectionType
 *    Type of projection for the slice drawing (oblique, orthogonal)
 * @param sliceViewPlane
 *    View plane that is displayed.
 * @param montageSliceIndex
 *    Selected montage slice index
 * @param planeOut
 *    OUTPUT plane of slice after transforms.
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::createSlicePlaneEquation(const VolumeSliceProjectionTypeEnum::Enum sliceProjectionType,
                                                      const VolumeSliceViewPlaneEnum::Enum sliceViewPlane,
                                                      const float sliceCoordinates[3],
                                                      Plane& planeOut)
{
    /*
     * Default the slice normal vector to an orthogonal view
     */
    float sliceNormalVector[3] = { 0.0, 0.0, 0.0 };
    switch (sliceViewPlane) {
        case VolumeSliceViewPlaneEnum::ALL:
        case VolumeSliceViewPlaneEnum::AXIAL:
            sliceNormalVector[2] = 1.0;
            break;
        case VolumeSliceViewPlaneEnum::CORONAL:
            sliceNormalVector[1] = -1.0;
            break;
        case VolumeSliceViewPlaneEnum::PARASAGITTAL:
            sliceNormalVector[0] = -1.0;
            break;
    }
    
    switch (sliceProjectionType) {
            break;
        case VolumeSliceProjectionTypeEnum::VOLUME_SLICE_PROJECTION_OBLIQUE:
        {
            /*
             * Transform the slice normal vector by the oblique rotation
             * matrix so that the normal vector points out of the slice
             */
            const Matrix4x4 obliqueRotationMatrix = m_browserTabContent->getObliqueVolumeRotationMatrix();
            obliqueRotationMatrix.multiplyPoint3(sliceNormalVector);
            MathFunctions::normalizeVector(sliceNormalVector);
        }
            break;
        case VolumeSliceProjectionTypeEnum::VOLUME_SLICE_PROJECTION_ORTHOGONAL:
            CaretAssert(0);
            break;
    }
    
    Plane plane(sliceNormalVector,
                sliceCoordinates);
    planeOut = plane;
    
    m_lookAtCenter[0] = sliceCoordinates[0];
    m_lookAtCenter[1] = sliceCoordinates[1];
    m_lookAtCenter[2] = sliceCoordinates[2];
}

/**
 * Set the volume slice viewing transformation.  This sets the position and
 * orientation of the camera.
 *
 * @param sliceProjectionType
 *    Type of projection for the slice drawing (oblique, orthogonal)
 * @param sliceViewPlane
 *    View plane that is displayed.
 * @param plane
 *    Plane equation of selected slice.
 * @param sliceCoordinates
 *    Coordinates of the selected slices.
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::setVolumeSliceViewingAndModelingTransformations(const VolumeSliceProjectionTypeEnum::Enum sliceProjectionType,
                                                                             const VolumeSliceViewPlaneEnum::Enum sliceViewPlane,
                                                                             const Plane& plane,
                                                                             const float sliceCoordinates[3])
{
    /*
     * Initialize the modelview matrix to the identity matrix
     * This places the camera at the origin, pointing down the
     * negative-Z axis with the up vector set to (0,1,0 =>
     * positive-Y is up).
     */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    const float* userTranslation = m_browserTabContent->getTranslation();
    
    /*
     * Move the camera with the user's translation
     */
    float viewTranslationX = 0.0;
    float viewTranslationY = 0.0;
    float viewTranslationZ = 0.0;
    
    switch (sliceViewPlane) {
        case VolumeSliceViewPlaneEnum::ALL:
        case VolumeSliceViewPlaneEnum::AXIAL:
            viewTranslationX = sliceCoordinates[0] + userTranslation[0];
            viewTranslationY = sliceCoordinates[1] + userTranslation[1];
            break;
        case VolumeSliceViewPlaneEnum::CORONAL:
            viewTranslationX = sliceCoordinates[0] + userTranslation[0];
            viewTranslationY = sliceCoordinates[2] + userTranslation[2];
            break;
        case VolumeSliceViewPlaneEnum::PARASAGITTAL:
            viewTranslationX = -(sliceCoordinates[1] + userTranslation[1]);
            viewTranslationY =   sliceCoordinates[2] + userTranslation[2];
            break;
    }
    
    glTranslatef(viewTranslationX,
                 viewTranslationY,
                 viewTranslationZ);
    
    
    
    
    glGetDoublev(GL_MODELVIEW_MATRIX,
                 m_viewingMatrix);
    
    /*
     * Since an orthographic projection is used, the camera only needs
     * to be a little bit from the center along the plane's normal vector.
     */
    double planeNormal[3];
    plane.getNormalVector(planeNormal);
    double cameraXYZ[3] = {
        m_lookAtCenter[0] + planeNormal[0] * BrainOpenGLFixedPipeline::s_gluLookAtCenterFromEyeOffsetDistance,
        m_lookAtCenter[1] + planeNormal[1] * BrainOpenGLFixedPipeline::s_gluLookAtCenterFromEyeOffsetDistance,
        m_lookAtCenter[2] + planeNormal[2] * BrainOpenGLFixedPipeline::s_gluLookAtCenterFromEyeOffsetDistance,
    };
    
    /*
     * Set the up vector which indices which way is up (screen Y)
     */
    float up[3] = { 0.0, 0.0, 0.0 };
    switch (sliceViewPlane) {
        case VolumeSliceViewPlaneEnum::ALL:
        case VolumeSliceViewPlaneEnum::AXIAL:
            up[1] = 1.0;
            break;
        case VolumeSliceViewPlaneEnum::CORONAL:
            up[2] = 1.0;
            break;
        case VolumeSliceViewPlaneEnum::PARASAGITTAL:
            up[2] = 1.0;
            break;
    }
    
    /*
     * For oblique viewing, the up vector needs to be rotated by the
     * oblique rotation matrix.
     */
    switch (sliceProjectionType) {
        case VolumeSliceProjectionTypeEnum::VOLUME_SLICE_PROJECTION_OBLIQUE:
            m_browserTabContent->getObliqueVolumeRotationMatrix().multiplyPoint3(up);
            break;
        case VolumeSliceProjectionTypeEnum::VOLUME_SLICE_PROJECTION_ORTHOGONAL:
            CaretAssert(0);
            break;
    }
    
    /*
     * Now set the camera to look at the selected coordinate (center)
     * with the camera offset a little bit from the center.
     * This allows the slice's voxels to be drawn in the actual coordinates.
     */
    gluLookAt(cameraXYZ[0], cameraXYZ[1], cameraXYZ[2],
              m_lookAtCenter[0], m_lookAtCenter[1], m_lookAtCenter[2],
              up[0], up[1], up[2]);
}

/**
 * Draw the layers type data.
 *
 * @param sliceDrawingType
 *    Type of slice drawing (montage, single)
 * @param sliceProjectionType
 *    Type of projection for the slice drawing (oblique, orthogonal)
 * @param sliceViewPlane
 *    View plane that is displayed.
 * @param slicePlane
 *    Plane of the slice.
 * @param sliceCoordinates
 *    Coordinates of the selected slices.
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::drawLayers(const VolumeSliceDrawingTypeEnum::Enum sliceDrawingType,
                                        const VolumeSliceProjectionTypeEnum::Enum sliceProjectionType,
                                        const VolumeSliceViewPlaneEnum::Enum sliceViewPlane,
                                        const Plane& slicePlane,
                                        const float sliceCoordinates[3])
{
    bool drawCrosshairsFlag = true;
    bool drawFibersFlag     = true;
    bool drawFociFlag       = true;
    bool drawOutlineFlag    = true;
    
    if (m_modelWholeBrain != NULL) {
        drawCrosshairsFlag = false;
        drawFibersFlag = false;
        drawFociFlag = false;
    }
    
    if ( ! m_identificationModeFlag) {
        if (slicePlane.isValidPlane()) {
            /*
             * Disable culling so that both sides of the triangles/quads are drawn.
             */
            GLboolean cullFaceOn = glIsEnabled(GL_CULL_FACE);
            glDisable(GL_CULL_FACE);
            
            glPushMatrix();
            
            GLboolean depthBufferEnabled = false;
            glGetBooleanv(GL_DEPTH_TEST,
                          &depthBufferEnabled);
            
            /*
             * Use some polygon offset that will adjust the depth values of the
             * layers so that the layers depth values place the layers in front of
             * the volume slice.
             */
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(0.0, 1.0);
            
            if (drawOutlineFlag) {
                BrainOpenGLVolumeSliceDrawing::drawSurfaceOutline(m_modelType,
                                                                  slicePlane,
                                                                  m_browserTabContent->getVolumeSurfaceOutlineSet(),
                                                                  m_fixedPipelineDrawing,
                                                                  true);
            }
            
            if (drawFibersFlag) {
                glDisable(GL_DEPTH_TEST);
                m_fixedPipelineDrawing->drawFiberOrientations(&slicePlane,
                                                              StructureEnum::ALL);
                m_fixedPipelineDrawing->drawFiberTrajectories(&slicePlane,
                                                              StructureEnum::ALL);
                if (depthBufferEnabled) {
                    glEnable(GL_DEPTH_TEST);
                }
                else {
                    glDisable(GL_DEPTH_TEST);
                }
            }
            if (drawFociFlag) {
                glDisable(GL_DEPTH_TEST);
                drawVolumeSliceFoci(slicePlane);
                if (depthBufferEnabled) {
                    glEnable(GL_DEPTH_TEST);
                }
                else {
                    glDisable(GL_DEPTH_TEST);
                }
            }
            
            glDisable(GL_POLYGON_OFFSET_FILL);
            
            if (drawCrosshairsFlag) {
                glPushMatrix();
                drawAxesCrosshairs(sliceProjectionType,
                                   sliceDrawingType,
                                   sliceViewPlane,
                                   sliceCoordinates);
                glPopMatrix();
                if (depthBufferEnabled) {
                    glEnable(GL_DEPTH_TEST);
                }
                else {
                    glDisable(GL_DEPTH_TEST);
                }
            }
            
            glPopMatrix();
            
            if (cullFaceOn) {
                glEnable(GL_CULL_FACE);
            }
        }
    }
}

/**
 * Draw foci on volume slice.
 *
 * @param plane
 *   Plane of the volume slice on which surface outlines are drawn.
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::drawVolumeSliceFoci(const Plane& plane)
{
    SelectionItemFocusVolume* idFocus = m_brain->getSelectionManager()->getVolumeFocusIdentification();
    
    /*
     * Check for a 'selection' type mode
     */
    bool isSelect = false;
    switch (m_fixedPipelineDrawing->mode) {
        case BrainOpenGLFixedPipeline::MODE_DRAWING:
            break;
        case BrainOpenGLFixedPipeline::MODE_IDENTIFICATION:
            if (idFocus->isEnabledForSelection()) {
                isSelect = true;
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            }
            else {
                return;
            }
            break;
        case BrainOpenGLFixedPipeline::MODE_PROJECTION:
            return;
            break;
    }
    
    VolumeMappableInterface* underlayVolume = m_volumeDrawInfo[0].volumeFile;
    float minVoxelSpacing;
    float maxVoxelSpacing;
    if ( ! getMinMaxVoxelSpacing(underlayVolume, minVoxelSpacing, maxVoxelSpacing)) {
        return;
    }
    
    const float sliceThickness = maxVoxelSpacing;
    const float halfSliceThickness = sliceThickness * 0.5;
    
    
    const DisplayPropertiesFoci* fociDisplayProperties = m_brain->getDisplayPropertiesFoci();
    const DisplayGroupEnum::Enum displayGroup = fociDisplayProperties->getDisplayGroupForTab(m_fixedPipelineDrawing->windowTabIndex);
    
    if (fociDisplayProperties->isDisplayed(displayGroup,
                                           m_fixedPipelineDrawing->windowTabIndex) == false) {
        return;
    }
    const float focusDiameter = fociDisplayProperties->getFociSize(displayGroup,
                                                                   m_fixedPipelineDrawing->windowTabIndex);
    const FeatureColoringTypeEnum::Enum fociColoringType = fociDisplayProperties->getColoringType(displayGroup,
                                                                                                  m_fixedPipelineDrawing->windowTabIndex);
    
    const CaretColorEnum::Enum caretColor = fociDisplayProperties->getStandardColorType(displayGroup,
                                                                                        m_fixedPipelineDrawing->windowTabIndex);
    float caretColorRGBA[4];
    CaretColorEnum::toRGBAFloat(caretColor, caretColorRGBA);
    
    bool drawAsSpheres = false;
    switch (fociDisplayProperties->getDrawingType(displayGroup,
                                                  m_fixedPipelineDrawing->windowTabIndex)) {
        case FociDrawingTypeEnum::DRAW_AS_SPHERES:
            drawAsSpheres = true;
            break;
        case FociDrawingTypeEnum::DRAW_AS_SQUARES:
            break;
    }
    
    /*
     * Process each foci file
     */
    const int32_t numberOfFociFiles = m_brain->getNumberOfFociFiles();
    for (int32_t iFile = 0; iFile < numberOfFociFiles; iFile++) {
        FociFile* fociFile = m_brain->getFociFile(iFile);
        
        const GroupAndNameHierarchyModel* classAndNameSelection = fociFile->getGroupAndNameHierarchyModel();
        if (classAndNameSelection->isSelected(displayGroup,
                                              m_fixedPipelineDrawing->windowTabIndex) == false) {
            continue;
        }
        
        const GiftiLabelTable* classColorTable = fociFile->getClassColorTable();
        const GiftiLabelTable* nameColorTable = fociFile->getNameColorTable();
        
        const int32_t numFoci = fociFile->getNumberOfFoci();
        
        for (int32_t j = 0; j < numFoci; j++) {
            Focus* focus = fociFile->getFocus(j);
            
            const GroupAndNameHierarchyItem* groupNameItem = focus->getGroupNameSelectionItem();
            if (groupNameItem != NULL) {
                if (groupNameItem->isSelected(displayGroup,
                                              m_fixedPipelineDrawing->windowTabIndex) == false) {
                    continue;
                }
            }
            
            float rgba[4] = { 0.0, 0.0, 0.0, 1.0 };
            switch (fociColoringType) {
                case FeatureColoringTypeEnum::FEATURE_COLORING_TYPE_CLASS:
                    if (focus->isClassRgbaValid() == false) {
                        const GiftiLabel* colorLabel = classColorTable->getLabelBestMatching(focus->getClassName());
                        if (colorLabel != NULL) {
                            colorLabel->getColor(rgba);
                            focus->setClassRgba(rgba);
                        }
                        else {
                            focus->setClassRgba(rgba);
                        }
                    }
                    focus->getClassRgba(rgba);
                    break;
                case FeatureColoringTypeEnum::FEATURE_COLORING_TYPE_STANDARD_COLOR:
                    rgba[0] = caretColorRGBA[0];
                    rgba[1] = caretColorRGBA[1];
                    rgba[2] = caretColorRGBA[2];
                    rgba[3] = caretColorRGBA[3];
                    break;
                case FeatureColoringTypeEnum::FEATURE_COLORING_TYPE_NAME:
                    if (focus->isNameRgbaValid() == false) {
                        const GiftiLabel* colorLabel = nameColorTable->getLabelBestMatching(focus->getName());
                        if (colorLabel != NULL) {
                            colorLabel->getColor(rgba);
                            focus->setNameRgba(rgba);
                        }
                        else {
                            focus->setNameRgba(rgba);
                        }
                    }
                    focus->getNameRgba(rgba);
                    break;
            }
            
            const int32_t numProjections = focus->getNumberOfProjections();
            for (int32_t k = 0; k < numProjections; k++) {
                const SurfaceProjectedItem* spi = focus->getProjection(k);
                if (spi->isVolumeXYZValid()) {
                    float xyz[3];
                    spi->getVolumeXYZ(xyz);
                    
                    bool drawIt = false;
                    if (plane.absoluteDistanceToPlane(xyz) < halfSliceThickness) {
                        drawIt = true;
                    }
                    
                    if (drawIt) {
                        glPushMatrix();
                        glTranslatef(xyz[0], xyz[1], xyz[2]);
                        if (isSelect) {
                            uint8_t idRGBA[4];
                            m_fixedPipelineDrawing->colorIdentification->addItem(idRGBA,
                                                                                 SelectionItemDataTypeEnum::FOCUS_VOLUME,
                                                                                 iFile, // file index
                                                                                 j, // focus index
                                                                                 k);// projection index
                            idRGBA[3] = 255;
                            if (drawAsSpheres) {
                                m_fixedPipelineDrawing->drawSphereWithDiameter(idRGBA,
                                                                               focusDiameter);
                            }
                            else {
                                glColor4ubv(idRGBA);
                                drawSquare(focusDiameter);
                            }
                        }
                        else {
                            if (drawAsSpheres) {
                                m_fixedPipelineDrawing->drawSphereWithDiameter(rgba,
                                                                               focusDiameter);
                            }
                            else {
                                glColor3fv(rgba);
                                drawSquare(focusDiameter);
                            }
                        }
                        glPopMatrix();
                    }
                }
            }
        }
    }
    
    if (isSelect) {
        int32_t fociFileIndex = -1;
        int32_t focusIndex = -1;
        int32_t focusProjectionIndex = -1;
        float depth = -1.0;
        m_fixedPipelineDrawing->getIndexFromColorSelection(SelectionItemDataTypeEnum::FOCUS_VOLUME,
                                                           m_fixedPipelineDrawing->mouseX,
                                                           m_fixedPipelineDrawing->mouseY,
                                                           fociFileIndex,
                                                           focusIndex,
                                                           focusProjectionIndex,
                                                           depth);
        if (fociFileIndex >= 0) {
            if (idFocus->isOtherScreenDepthCloserToViewer(depth)) {
                Focus* focus = m_brain->getFociFile(fociFileIndex)->getFocus(focusIndex);
                idFocus->setBrain(m_brain);
                idFocus->setFocus(focus);
                idFocus->setFociFile(m_brain->getFociFile(fociFileIndex));
                idFocus->setFocusIndex(focusIndex);
                idFocus->setFocusProjectionIndex(focusProjectionIndex);
                idFocus->setVolumeFile(underlayVolume);
                idFocus->setScreenDepth(depth);
                float xyz[3];
                const SurfaceProjectedItem* spi = focus->getProjection(focusProjectionIndex);
                spi->getVolumeXYZ(xyz);
                m_fixedPipelineDrawing->setSelectedItemScreenXYZ(idFocus, xyz);
                CaretLogFine("Selected Volume Focus Identification Symbol: " + QString::number(focusIndex));
            }
        }
    }
}

/**
 * Draw the axes crosshairs.
 *
 * @param sliceProjectionType
 *    Type of projection for the slice drawing (oblique, orthogonal)
 * @param sliceDrawingType
 *    Type of slice drawing (montage, single)
 * @param sliceViewPlane
 *    View plane that is displayed.
 * @param sliceCoordinates
 *    Coordinates of the selected slices.
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::drawAxesCrosshairs(const VolumeSliceProjectionTypeEnum::Enum sliceProjectionType,
                                                const VolumeSliceDrawingTypeEnum::Enum sliceDrawingType,
                                                const VolumeSliceViewPlaneEnum::Enum sliceViewPlane,
                                                const float sliceCoordinates[3])
{
    const bool drawCrosshairsFlag = m_browserTabContent->isVolumeAxesCrosshairsDisplayed();
    bool drawCrosshairLabelsFlag = m_browserTabContent->isVolumeAxesCrosshairLabelsDisplayed();
    
    switch (sliceDrawingType) {
        case VolumeSliceDrawingTypeEnum::VOLUME_SLICE_DRAW_MONTAGE:
            drawCrosshairLabelsFlag = false;
            break;
        case VolumeSliceDrawingTypeEnum::VOLUME_SLICE_DRAW_SINGLE:
            break;
    }
    switch (sliceProjectionType) {
        case VolumeSliceProjectionTypeEnum::VOLUME_SLICE_PROJECTION_ORTHOGONAL:
            CaretAssert(0);
            break;
        case VolumeSliceProjectionTypeEnum::VOLUME_SLICE_PROJECTION_OBLIQUE:
        {
            glPushMatrix();
            glLoadIdentity();
            drawAxesCrosshairsOrthoAndOblique(sliceProjectionType,
                                              sliceViewPlane,
                                              sliceCoordinates,
                                              drawCrosshairsFlag,
                                              drawCrosshairLabelsFlag);
            glPopMatrix();
        }
            break;
    }
}

/**
 * Draw the axes crosshairs for an orthogonal slice.
 *
 * @param sliceProjectionType
 *    Type of projection for the slice drawing (oblique, orthogonal)
 * @param sliceViewPlane
 *    The slice plane view.
 * @param sliceCoordinates
 *    Coordinates of the selected slices.
 * @param drawCrosshairsFlag
 *    If true, draw the crosshairs.
 * @param drawCrosshairLabelsFlag
 *    If true, draw the crosshair labels.
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::drawAxesCrosshairsOrthoAndOblique(const VolumeSliceProjectionTypeEnum::Enum sliceProjectionType,
                                                                 const VolumeSliceViewPlaneEnum::Enum sliceViewPlane,
                                                               const float sliceCoordinates[3],
                                                                 const bool drawCrosshairsFlag,
                                                                 const bool drawCrosshairLabelsFlag)
{
    bool obliqueModeFlag = false;
    switch (sliceProjectionType) {
        case VolumeSliceProjectionTypeEnum::VOLUME_SLICE_PROJECTION_OBLIQUE:
            obliqueModeFlag = true;
            break;
        case VolumeSliceProjectionTypeEnum::VOLUME_SLICE_PROJECTION_ORTHOGONAL:
            CaretAssert(0);
            break;
    }
    
    GLboolean depthEnabled = GL_FALSE;
    glGetBooleanv(GL_DEPTH_TEST,
                  &depthEnabled);
    glDisable(GL_DEPTH_TEST);
    
    const float bigValue = 10000.0;
    
    float horizontalAxisStartXYZ[3] = {
        sliceCoordinates[0],
        sliceCoordinates[1],
        sliceCoordinates[2]
    };
    float horizontalAxisEndXYZ[3] = {
        sliceCoordinates[0],
        sliceCoordinates[1],
        sliceCoordinates[2]
    };
    
    float verticalAxisStartXYZ[3] = {
        sliceCoordinates[0],
        sliceCoordinates[1],
        sliceCoordinates[2]
    };
    float verticalAxisEndXYZ[3] = {
        sliceCoordinates[0],
        sliceCoordinates[1],
        sliceCoordinates[2]
    };
    
    float trans[3];
    m_browserTabContent->getTranslation(trans);

    float horizTrans[3] = { trans[0], trans[1], trans[2] };
    float vertTrans[3] = { trans[0], trans[1], trans[2] };
    
    if (obliqueModeFlag) {
        switch (sliceViewPlane) {
            case VolumeSliceViewPlaneEnum::ALL:
                break;
            case VolumeSliceViewPlaneEnum::AXIAL:
                break;
            case VolumeSliceViewPlaneEnum::CORONAL:
                horizontalAxisStartXYZ[0] = sliceCoordinates[0];
                horizontalAxisStartXYZ[1] = sliceCoordinates[2];
                horizontalAxisStartXYZ[2] = sliceCoordinates[1];
                horizontalAxisEndXYZ[0]   = sliceCoordinates[0];
                horizontalAxisEndXYZ[1]   = sliceCoordinates[2];
                horizontalAxisEndXYZ[2]   = sliceCoordinates[1];
                
                horizTrans[0] = trans[0];
                horizTrans[1] = trans[2];
                horizTrans[2] = trans[1];
                
                verticalAxisStartXYZ[0] = sliceCoordinates[0];
                verticalAxisStartXYZ[1] = sliceCoordinates[1];
                verticalAxisStartXYZ[2] = sliceCoordinates[2];
                verticalAxisEndXYZ[0]   = sliceCoordinates[0];
                verticalAxisEndXYZ[1]   = sliceCoordinates[1];
                verticalAxisEndXYZ[2]   = sliceCoordinates[2];

                vertTrans[0]   = trans[0];
                vertTrans[1]   = trans[1];
                vertTrans[2]   = trans[2];
                break;
            case VolumeSliceViewPlaneEnum::PARASAGITTAL:
                horizontalAxisStartXYZ[0] = sliceCoordinates[1];
                horizontalAxisStartXYZ[1] = sliceCoordinates[2];
                horizontalAxisStartXYZ[2] = sliceCoordinates[0];
                horizontalAxisEndXYZ[0]   = sliceCoordinates[1];
                horizontalAxisEndXYZ[1]   = sliceCoordinates[2];
                horizontalAxisEndXYZ[2]   = sliceCoordinates[0];
                
                horizTrans[0] = trans[1];
                horizTrans[1] = trans[2];
                horizTrans[2] = trans[0];
                
                verticalAxisStartXYZ[0] = -sliceCoordinates[1];
                verticalAxisStartXYZ[1] = sliceCoordinates[0];
                verticalAxisStartXYZ[2] = sliceCoordinates[2];
                verticalAxisEndXYZ[0]   = -sliceCoordinates[1];
                verticalAxisEndXYZ[1]   = sliceCoordinates[0];
                verticalAxisEndXYZ[2]   = sliceCoordinates[2];
                
                vertTrans[0]   = -trans[1];
                vertTrans[1]   = trans[0];
                vertTrans[2]   = trans[2];
                break;
        }
    }
    
    float axialRGBA[4];
    getAxesColor(VolumeSliceViewPlaneEnum::AXIAL,
                 axialRGBA);
    
    float coronalRGBA[4];
    getAxesColor(VolumeSliceViewPlaneEnum::CORONAL,
                 coronalRGBA);
    
    float paraRGBA[4];
    getAxesColor(VolumeSliceViewPlaneEnum::PARASAGITTAL,
                 paraRGBA);
    
    AString horizontalLeftText  = "";
    AString horizontalRightText = "";
    AString verticalBottomText  = "";
    AString verticalTopText     = "";
    
    float* horizontalAxisRGBA   = axialRGBA;
    float* verticalAxisRGBA     = axialRGBA;
    
    switch (sliceViewPlane) {
        case VolumeSliceViewPlaneEnum::ALL:
            break;
        case VolumeSliceViewPlaneEnum::AXIAL:
            horizontalLeftText   = "L";
            horizontalRightText  = "R";
            horizontalAxisRGBA = coronalRGBA;
            horizontalAxisStartXYZ[0] -= bigValue;
            horizontalAxisEndXYZ[0]   += bigValue;
            
            verticalBottomText = "P";
            verticalTopText    = "A";
            verticalAxisRGBA = paraRGBA;
            verticalAxisStartXYZ[1] -= bigValue;
            verticalAxisEndXYZ[1]   += bigValue;
            break;
        case VolumeSliceViewPlaneEnum::CORONAL:
            horizontalLeftText   = "L";
            horizontalRightText  = "R";
            horizontalAxisRGBA = axialRGBA;
            if (obliqueModeFlag) {
                horizontalAxisStartXYZ[0] -= bigValue;
                horizontalAxisEndXYZ[0]   += bigValue;
            }
            else {
                horizontalAxisStartXYZ[0] -= bigValue;
                horizontalAxisEndXYZ[0]   += bigValue;
            }
            
            verticalBottomText = "I";
            verticalTopText    = "S";
            verticalAxisRGBA = paraRGBA;
            if (obliqueModeFlag) {
                verticalAxisStartXYZ[1] -= bigValue;
                verticalAxisEndXYZ[1]   += bigValue;
            }
            else {
                verticalAxisStartXYZ[2] -= bigValue;
                verticalAxisEndXYZ[2]   += bigValue;
            }
            break;
        case VolumeSliceViewPlaneEnum::PARASAGITTAL:
            horizontalLeftText   = "A";
            horizontalRightText  = "P";
            horizontalAxisRGBA = axialRGBA;
            if (obliqueModeFlag) {
                horizontalAxisStartXYZ[0] -= bigValue;
                horizontalAxisEndXYZ[0]   += bigValue;
            }
            else {
                horizontalAxisStartXYZ[1] -= bigValue;
                horizontalAxisEndXYZ[1]   += bigValue;
            }
            
            verticalBottomText = "I";
            verticalTopText    = "S";
            verticalAxisRGBA = coronalRGBA;
            if (obliqueModeFlag) {
                verticalAxisStartXYZ[1] -= bigValue;
                verticalAxisEndXYZ[1]   += bigValue;
            }
            else {
                verticalAxisStartXYZ[2] -= bigValue;
                verticalAxisEndXYZ[2]   += bigValue;
            }
            break;
    }
    
    /*
     * Offset text labels be a percentage of viewort width/height
     */
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT,
                  viewport);
    const int textOffsetX = viewport[2] * 0.01f;
    const int textOffsetY = viewport[3] * 0.01f;
    const int textLeftWindowXY[2] = {
        textOffsetX,
        (viewport[3] / 2)
    };
    const int textRightWindowXY[2] = {
        viewport[2] - textOffsetX,
        (viewport[3] / 2)
    };
    const int textBottomWindowXY[2] = {
        viewport[2] / 2,
        textOffsetY
    };
    const int textTopWindowXY[2] = {
        (viewport[2] / 2),
        viewport[3] - textOffsetY
    };
    
    /*
     * Crosshairs
     */
    if (drawCrosshairsFlag) {
        glPushMatrix();
        glTranslatef(horizTrans[0], horizTrans[1], horizTrans[2]);
        std::unique_ptr<GraphicsPrimitiveV3fC4f> horizHairPrimitive(GraphicsPrimitive::newPrimitiveV3fC4f(GraphicsPrimitive::PrimitiveType::POLYGONAL_LINES));
        horizHairPrimitive->addVertex(horizontalAxisStartXYZ, horizontalAxisRGBA);
        horizHairPrimitive->addVertex(horizontalAxisEndXYZ, horizontalAxisRGBA);
        horizHairPrimitive->setLineWidth(GraphicsPrimitive::LineWidthType::PIXELS, 2.0f);
        GraphicsEngineDataOpenGL::draw(horizHairPrimitive.get());
        glPopMatrix();
        
        glPushMatrix();
        glTranslatef(vertTrans[0], vertTrans[1], vertTrans[2]);
        std::unique_ptr<GraphicsPrimitiveV3fC4f> vertHairPrimitive(GraphicsPrimitive::newPrimitiveV3fC4f(GraphicsPrimitive::PrimitiveType::POLYGONAL_LINES));
        vertHairPrimitive->addVertex(verticalAxisStartXYZ, verticalAxisRGBA);
        vertHairPrimitive->addVertex(verticalAxisEndXYZ, verticalAxisRGBA);
        vertHairPrimitive->setLineWidth(GraphicsPrimitive::LineWidthType::PIXELS, 2.0f);
        GraphicsEngineDataOpenGL::draw(vertHairPrimitive.get());
        glPopMatrix();
    }
    
    if (drawCrosshairLabelsFlag) {
        const AnnotationTextFontPointSizeEnum::Enum fontSize = AnnotationTextFontPointSizeEnum::SIZE18;
        
        const int textCenter[2] = {
            textLeftWindowXY[0],
            textLeftWindowXY[1]
        };
        const int halfFontSize = AnnotationTextFontPointSizeEnum::toSizeNumeric(fontSize) / 2;
        
        uint8_t backgroundRGBA[4] = {
            m_fixedPipelineDrawing->m_backgroundColorByte[0],
            m_fixedPipelineDrawing->m_backgroundColorByte[1],
            m_fixedPipelineDrawing->m_backgroundColorByte[2],
            m_fixedPipelineDrawing->m_backgroundColorByte[3]
        };
        
        GLint savedViewport[4];
        glGetIntegerv(GL_VIEWPORT, savedViewport);
        
        int vpLeftX   = savedViewport[0] + textCenter[0] - halfFontSize;
        int vpRightX  = savedViewport[0] + textCenter[0] + halfFontSize;
        int vpBottomY = savedViewport[1] + textCenter[1] - halfFontSize;
        int vpTopY    = savedViewport[1] + textCenter[1] + halfFontSize;
        MathFunctions::limitRange(vpLeftX,
                                  savedViewport[0],
                                  savedViewport[0] + savedViewport[2]);
        MathFunctions::limitRange(vpRightX,
                                  savedViewport[0],
                                  savedViewport[0] + savedViewport[2]);
        MathFunctions::limitRange(vpBottomY,
                                  savedViewport[1],
                                  savedViewport[1] + savedViewport[3]);
        MathFunctions::limitRange(vpTopY,
                                  savedViewport[1],
                                  savedViewport[1] + savedViewport[3]);
        
        const int vpSizeX = vpRightX - vpLeftX;
        const int vpSizeY = vpTopY - vpBottomY;
        glViewport(vpLeftX, vpBottomY, vpSizeX, vpSizeY);
        
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
        
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        
        std::vector<uint8_t> rgba;
        std::vector<float> coords, normals;
        
        coords.push_back(-1.0);
        coords.push_back(-1.0);
        coords.push_back( 0.0);
        normals.push_back(0.0);
        normals.push_back(0.0);
        normals.push_back(1.0);
        rgba.push_back(backgroundRGBA[0]);
        rgba.push_back(backgroundRGBA[1]);
        rgba.push_back(backgroundRGBA[2]);
        rgba.push_back(backgroundRGBA[3]);
        
        coords.push_back( 1.0);
        coords.push_back(-1.0);
        coords.push_back( 0.0);
        normals.push_back(0.0);
        normals.push_back(0.0);
        normals.push_back(1.0);
        rgba.push_back(backgroundRGBA[0]);
        rgba.push_back(backgroundRGBA[1]);
        rgba.push_back(backgroundRGBA[2]);
        rgba.push_back(backgroundRGBA[3]);
        
        coords.push_back( 1.0);
        coords.push_back( 1.0);
        coords.push_back( 0.0);
        normals.push_back(0.0);
        normals.push_back(0.0);
        normals.push_back(1.0);
        rgba.push_back(backgroundRGBA[0]);
        rgba.push_back(backgroundRGBA[1]);
        rgba.push_back(backgroundRGBA[2]);
        rgba.push_back(backgroundRGBA[3]);
        
        coords.push_back(-1.0);
        coords.push_back( 1.0);
        coords.push_back( 0.0);
        normals.push_back(0.0);
        normals.push_back(0.0);
        normals.push_back(1.0);
        rgba.push_back(backgroundRGBA[0]);
        rgba.push_back(backgroundRGBA[1]);
        rgba.push_back(backgroundRGBA[2]);
        rgba.push_back(backgroundRGBA[3]);
        
        
        BrainOpenGLPrimitiveDrawing::drawQuads(coords,
                                               normals,
                                               rgba);
        
        glPopMatrix();
        
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        
        glViewport(savedViewport[0],
                   savedViewport[1],
                   savedViewport[2],
                   savedViewport[3]);
        
        AnnotationPercentSizeText annotationText(AnnotationAttributesDefaultTypeEnum::NORMAL);
        annotationText.setBoldStyleEnabled(true);
        annotationText.setFontPercentViewportSize(5.0f);
        annotationText.setTextColor(CaretColorEnum::CUSTOM);
        annotationText.setBackgroundColor(CaretColorEnum::CUSTOM);
        annotationText.setCustomTextColor(horizontalAxisRGBA);
        annotationText.setCustomBackgroundColor(backgroundRGBA);

        annotationText.setHorizontalAlignment(AnnotationTextAlignHorizontalEnum::LEFT);
        annotationText.setVerticalAlignment(AnnotationTextAlignVerticalEnum::MIDDLE);
        annotationText.setText(horizontalLeftText);
        m_fixedPipelineDrawing->drawTextAtViewportCoords(textLeftWindowXY[0],
                                                         textLeftWindowXY[1],
                                                         annotationText);
        
        annotationText.setText(horizontalRightText);
        annotationText.setHorizontalAlignment(AnnotationTextAlignHorizontalEnum::RIGHT);
        annotationText.setVerticalAlignment(AnnotationTextAlignVerticalEnum::MIDDLE);
        m_fixedPipelineDrawing->drawTextAtViewportCoords(textRightWindowXY[0],
                                                         textRightWindowXY[1],
                                                         annotationText);
        
        annotationText.setHorizontalAlignment(AnnotationTextAlignHorizontalEnum::CENTER);
        annotationText.setVerticalAlignment(AnnotationTextAlignVerticalEnum::BOTTOM);
        annotationText.setCustomTextColor(verticalAxisRGBA);
        annotationText.setText(verticalBottomText);
        m_fixedPipelineDrawing->drawTextAtViewportCoords(textBottomWindowXY[0],
                                                         textBottomWindowXY[1],
                                                         annotationText);

        annotationText.setText(verticalTopText);
        annotationText.setHorizontalAlignment(AnnotationTextAlignHorizontalEnum::CENTER);
        annotationText.setVerticalAlignment(AnnotationTextAlignVerticalEnum::TOP);
        annotationText.getCoordinate()->setXYZ(textTopWindowXY[0], textTopWindowXY[1], 0.0);
        m_fixedPipelineDrawing->drawTextAtViewportCoords(textTopWindowXY[0],
                                                         textTopWindowXY[1],
                                                         annotationText);
    }
    
    if (depthEnabled) {
        glEnable(GL_DEPTH_TEST);
    }
}

/**
 * Get the RGBA coloring for a slice view plane.
 *
 * @param sliceViewPlane
 *    The slice view plane.
 * @param rgbaOut
 *    Output colors ranging 0.0 to 1.0
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::getAxesColor(const VolumeSliceViewPlaneEnum::Enum sliceViewPlane,
                                            float rgbaOut[4]) const
{
    switch (sliceViewPlane) {
        case VolumeSliceViewPlaneEnum::ALL:
            CaretAssert(0);
            break;
        case VolumeSliceViewPlaneEnum::AXIAL:
            rgbaOut[0] = 0.0;
            rgbaOut[1] = 0.0;
            rgbaOut[2] = 1.0;
            rgbaOut[3] = 1.0;
            break;
        case VolumeSliceViewPlaneEnum::CORONAL:
            rgbaOut[0] = 0.0;
            rgbaOut[1] = 1.0;
            rgbaOut[2] = 0.0;
            rgbaOut[3] = 1.0;
            break;
        case VolumeSliceViewPlaneEnum::PARASAGITTAL:
            rgbaOut[0] = 1.0;
            rgbaOut[1] = 0.0;
            rgbaOut[2] = 0.0;
            rgbaOut[3] = 1.0;
            break;
    }
}

/**
 * Draw a one millimeter square facing the user.
 * NOTE: This method will alter the current
 * modelviewing matrices so caller may need
 * to enclose the call to this method within
 * glPushMatrix() and glPopMatrix().
 *
 * @param size
 *     Size of square.
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::drawSquare(const float size)
{
    const float length = size * 0.5;
    
    /*
     * Draw both front and back side since in some instances,
     * such as surface montage, we are viweing from the far
     * side (from back of monitor)
     */
    glBegin(GL_QUADS);
    glNormal3f(0.0, 0.0, 1.0);
    glVertex3f(-length, -length, 0.0);
    glVertex3f( length, -length, 0.0);
    glVertex3f( length,  length, 0.0);
    glVertex3f(-length,  length, 0.0);
    glNormal3f(0.0, 0.0, -1.0);
    glVertex3f(-length, -length, 0.0);
    glVertex3f(-length,  length, 0.0);
    glVertex3f( length,  length, 0.0);
    glVertex3f( length, -length, 0.0);
    glEnd();
}

/**
 * Get the minimum and maximum distance between adjacent voxels in all
 * slices planes.  Output spacing value are always non-negative even if
 * a right-to-left orientation.
 *
 * @param volume
 *    Volume for which min/max spacing is requested.
 * @param minSpacingOut
 *    Output minimum spacing.
 * @param maxSpacingOut
 *    Output maximum spacing.
 * @return
 *    True if min and max spacing are greater than zero.
 */
bool
BrainOpenGLVolumeObliqueSliceDrawing::getMinMaxVoxelSpacing(const VolumeMappableInterface* volume,
                                                     float& minSpacingOut,
                                                     float& maxSpacingOut) const
{
    CaretAssert(volume);
    
    float originX, originY, originZ;
    float x1, y1, z1;
    volume->indexToSpace(0, 0, 0, originX, originY, originZ);
    volume->indexToSpace(1, 1, 1, x1, y1, z1);
    const float dx = std::fabs(x1 - originX);
    const float dy = std::fabs(y1 - originY);
    const float dz = std::fabs(z1 - originZ);
    
    minSpacingOut = std::min(std::min(dx, dy), dz);
    maxSpacingOut = std::max(std::max(dx, dy), dz);
    
    if ((minSpacingOut > 0.0)
        && (maxSpacingOut > 0.0)) {
        return true;
    }
    return false;
}

/**
 * Draw orientation axes
 *
 * @param viewport
 *    The viewport region for the orientation axes.
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::drawOrientationAxes(const int viewport[4])
{
    const bool drawCylindersFlag = m_browserTabContent->isVolumeAxesCrosshairsDisplayed();
    const bool drawLabelsFlag = m_browserTabContent->isVolumeAxesCrosshairLabelsDisplayed();
    
    /*
     * Set the viewport
     */
    glViewport(viewport[0],
               viewport[1],
               viewport[2],
               viewport[3]);
    const double viewportWidth  = viewport[2];
    const double viewportHeight = viewport[3];
    
    /*
     * Determine bounds for orthographic projection
     */
    const double maxCoord = 100.0;
    const double minCoord = -maxCoord;
    double left   = 0.0;
    double right  = 0.0;
    double top    = 0.0;
    double bottom = 0.0;
    const double nearDepth = -1000.0;
    const double farDepth  =  1000.0;
    if (viewportHeight > viewportWidth) {
        left  = minCoord;
        right = maxCoord;
        const double aspectRatio = (viewportHeight
                                    / viewportWidth);
        top   = maxCoord * aspectRatio;
        bottom = minCoord * aspectRatio;
    }
    else {
        const double aspectRatio = (viewportWidth
                                    / viewportHeight);
        top   = maxCoord;
        bottom = minCoord;
        left  = minCoord * aspectRatio;
        right = maxCoord * aspectRatio;
    }
    
    /*
     * Set the orthographic projection
     */
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(left, right,
            bottom, top,
            nearDepth, farDepth);
    
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    {
        /*
         * Set the viewing transformation, places 'eye' so that it looks
         * at the 'model' which is, in this case, the axes
         */
        double eyeX = 0.0;
        double eyeY = 0.0;
        double eyeZ = BrainOpenGLFixedPipeline::s_gluLookAtCenterFromEyeOffsetDistance; //100.0;
        const double centerX = 0;
        const double centerY = 0;
        const double centerZ = 0;
        const double upX = 0;
        const double upY = 1;
        const double upZ = 0;
        gluLookAt(eyeX, eyeY, eyeZ,
                  centerX, centerY, centerZ,
                  upX, upY, upZ);
        
        /*
         * Set the modeling transformation
         */
        const Matrix4x4 obliqueRotationMatrix = m_browserTabContent->getObliqueVolumeRotationMatrix();
        double rotationMatrix[16];
        obliqueRotationMatrix.getMatrixForOpenGL(rotationMatrix);
        glMultMatrixd(rotationMatrix);
        
        /*
         * Disable depth buffer.  Otherwise, when volume slices are drawn
         * black regions of the slices may set depth buffer and the occlude
         * the axes from display.
         */
        GLboolean depthBufferEnabled = false;
        glGetBooleanv(GL_DEPTH_TEST,
                      &depthBufferEnabled);
        glDisable(GL_DEPTH_TEST);
        const float red[4] = {
            1.0, 0.0, 0.0, 1.0
        };
        const float green[4] = {
            0.0, 1.0, 0.0, 1.0
        };
        const float blue[4] = {
            0.0, 0.0, 1.0, 1.0
        };
        
        const double axisMaxCoord = maxCoord * 0.8;
        const double axisMinCoord = -axisMaxCoord;
        const double textMaxCoord = maxCoord * 0.9;
        const double textMinCoord = -textMaxCoord;
        
        
        const float axialPlaneMin[3] = { 0.0, 0.0, (float)axisMinCoord };
        const float axialPlaneMax[3] = { 0.0, 0.0, (float)axisMaxCoord };
        const double axialTextMin[3]  = { 0.0, 0.0, (float)textMinCoord };
        const double axialTextMax[3]  = { 0.0, 0.0, (float)textMaxCoord };
        
        const float coronalPlaneMin[3] = { (float)axisMinCoord, 0.0, 0.0 };
        const float coronalPlaneMax[3] = { (float)axisMaxCoord, 0.0, 0.0 };
        const double coronalTextMin[3]  = { (float)textMinCoord, 0.0, 0.0 };
        const double coronalTextMax[3]  = { (float)textMaxCoord, 0.0, 0.0 };
        
        const float paraPlaneMin[3] = { 0.0, (float)axisMinCoord, 0.0 };
        const float paraPlaneMax[3] = { 0.0, (float)axisMaxCoord, 0.0 };
        const double paraTextMin[3]  = { 0.0, (float)textMinCoord, 0.0 };
        const double paraTextMax[3]  = { 0.0, (float)textMaxCoord, 0.0 };
        
        /*
         * Set radius as percentage of viewport height
         */
        float axesCrosshairRadius = 1.0;
        if (viewportHeight > 0) {
            const float percentageRadius = 0.005f;
            axesCrosshairRadius = percentageRadius * viewportHeight;
        }
        
        if (drawCylindersFlag) {
            m_fixedPipelineDrawing->drawCylinder(blue,
                                                 axialPlaneMin,
                                                 axialPlaneMax,
                                                 axesCrosshairRadius * 0.5f);
        }
        
        AnnotationPercentSizeText annotationText(AnnotationAttributesDefaultTypeEnum::NORMAL);
        annotationText.setHorizontalAlignment(AnnotationTextAlignHorizontalEnum::CENTER);
        annotationText.setVerticalAlignment(AnnotationTextAlignVerticalEnum::MIDDLE);
        annotationText.setFontPercentViewportSize(5.0f);
        annotationText.setCoordinateSpace(AnnotationCoordinateSpaceEnum::STEREOTAXIC);
        annotationText.setTextColor(CaretColorEnum::CUSTOM);
        
        if (drawLabelsFlag) {
            annotationText.setCustomTextColor(blue);
            annotationText.setText("I");
            m_fixedPipelineDrawing->drawTextAtModelCoords(axialTextMin,
                                                          annotationText);
            annotationText.setText("S");
            m_fixedPipelineDrawing->drawTextAtModelCoords(axialTextMax,
                                                          annotationText);
        }
        
        
        if (drawCylindersFlag) {
            m_fixedPipelineDrawing->drawCylinder(green,
                                                 coronalPlaneMin,
                                                 coronalPlaneMax,
                                                 axesCrosshairRadius * 0.5f);
        }
        
        if (drawLabelsFlag) {
            annotationText.setCustomTextColor(green);
            annotationText.setText("L");
            m_fixedPipelineDrawing->drawTextAtModelCoords(coronalTextMin,
                                                          annotationText);
            annotationText.setText("R");
            m_fixedPipelineDrawing->drawTextAtModelCoords(coronalTextMax,
                                                          annotationText);
        }
        
        
        if (drawCylindersFlag) {
            m_fixedPipelineDrawing->drawCylinder(red,
                                                 paraPlaneMin,
                                                 paraPlaneMax,
                                                 axesCrosshairRadius * 0.5f);
        }
        
        if (drawLabelsFlag) {
            annotationText.setCustomTextColor(red);
            annotationText.setText("P");
            m_fixedPipelineDrawing->drawTextAtModelCoords(paraTextMin,
                                                          annotationText);

            annotationText.setText("A");
            m_fixedPipelineDrawing->drawTextAtModelCoords(paraTextMax,
                                                          annotationText);
        }
    }
    glPopMatrix();
    
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
}

/**
 * Set the orthographic projection.
 *
 * @param sliceViewPlane
 *    View plane that is displayed.
 * @param viewport
 *    The viewport.
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::setOrthographicProjection(const VolumeSliceViewPlaneEnum::Enum sliceViewPlane,
                                                         const int viewport[4])
{
    
    /*
     * Determine model size in screen Y when viewed
     */
    BoundingBox boundingBox;
    m_volumeDrawInfo[0].volumeFile->getVoxelSpaceBoundingBox(boundingBox);
    
    /*
     * Set top and bottom to the min/max coordinate
     * that runs vertically on the screen
     */
    double modelTop = 200.0;
    double modelBottom = -200.0;
    switch (sliceViewPlane) {
        case VolumeSliceViewPlaneEnum::ALL:
            CaretAssertMessage(0, "Should never get here");
            break;
        case VolumeSliceViewPlaneEnum::AXIAL:
            modelTop = boundingBox.getMaxY();
            modelBottom = boundingBox.getMinY();
            break;
        case VolumeSliceViewPlaneEnum::CORONAL:
            modelTop = boundingBox.getMaxZ();
            modelBottom = boundingBox.getMinZ();
            break;
        case VolumeSliceViewPlaneEnum::PARASAGITTAL:
            modelTop = boundingBox.getMaxZ();
            modelBottom = boundingBox.getMinZ();
            break;
    }
    
    /*
     * Scale ratio makes region slightly larger than model
     */
    const double zoom = m_browserTabContent->getScaling();
    double scaleRatio = (1.0 / 0.98);
    if (zoom > 0.0) {
        scaleRatio /= zoom;
    }
    modelTop *= scaleRatio;
    modelBottom *= scaleRatio;
    
    /*
     * Determine aspect ratio of viewport
     */
    const double viewportWidth = viewport[2];
    const double viewportHeight = viewport[3];
    const double aspectRatio = (viewportWidth
                                / viewportHeight);
    
    /*
     * Set bounds of orthographic projection
     */
    const double halfModelY = ((modelTop - modelBottom) / 2.0);
    const double orthoBottom = modelBottom;
    const double orthoTop    = modelTop;
    const double orthoRight  =  halfModelY * aspectRatio;
    const double orthoLeft   = -halfModelY * aspectRatio;
    const double nearDepth = -1000.0;
    const double farDepth  =  1000.0;
    m_orthographicBounds[0] = orthoLeft;
    m_orthographicBounds[1] = orthoRight;
    m_orthographicBounds[2] = orthoBottom;
    m_orthographicBounds[3] = orthoTop;
    m_orthographicBounds[4] = nearDepth;
    m_orthographicBounds[5] = farDepth;
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(m_orthographicBounds[0],
            m_orthographicBounds[1],
            m_orthographicBounds[2],
            m_orthographicBounds[3],
            m_orthographicBounds[4],
            m_orthographicBounds[5]);
    glMatrixMode(GL_MODELVIEW);
}

/**
 * Reset for volume identification.
 *
 * Clear identification indices and if identification is enabled,
 * reserve a reasonable amount of space for the indices.
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::resetIdentification()
{
    m_identificationIndices.clear();
    
    if (m_identificationModeFlag) {
        int64_t estimatedNumberOfItems = 0;
        
        std::vector<int64_t> volumeDims;
        m_volumeDrawInfo[0].volumeFile->getDimensions(volumeDims);
        if (volumeDims.size() >= 3) {
            const int64_t maxDim = std::max(volumeDims[0],
                                            std::max(volumeDims[1], volumeDims[2]));
            estimatedNumberOfItems = maxDim * maxDim * IDENTIFICATION_INDICES_PER_VOXEL;
        }
        
        m_identificationIndices.reserve(estimatedNumberOfItems);
    }
}

/**
 * Add a voxel to the identification indices.
 *
 * @param volumeIndex
 *    Index of the volume.
 * @param mapIndex
 *    Index of the volume map.
 * @param voxelI
 *    Voxel Index I
 * @param voxelJ
 *    Voxel Index J
 * @param voxelK
 *    Voxel Index K
 * @param voxelDiffXYZ
 *    Change in XYZ from voxel bottom left to top right
 * @param rgbaForColorIdentificationOut
 *    Encoded identification in RGBA color OUTPUT
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::addVoxelToIdentification(const int32_t volumeIndex,
                                                        const int32_t mapIndex,
                                                        const int32_t voxelI,
                                                        const int32_t voxelJ,
                                                        const int32_t voxelK,
                                                        const float voxelDiffXYZ[3],
                                                        uint8_t rgbaForColorIdentificationOut[4])
{
    const int32_t idIndex = m_identificationIndices.size() / IDENTIFICATION_INDICES_PER_VOXEL;
    
    m_fixedPipelineDrawing->colorIdentification->addItem(rgbaForColorIdentificationOut,
                                                         SelectionItemDataTypeEnum::VOXEL,
                                                         idIndex);
    rgbaForColorIdentificationOut[3] = 255;
    
    /*
     * ID stack requires integers to 
     * use an integer pointer to the float values
     */
    CaretAssert(sizeof(float) == sizeof(int32_t));
    const int32_t* intPointerDiffXYZ = (int32_t*)voxelDiffXYZ;
    
    /*
     * If these items change, need to update reset and
     * processing of identification.
     */
    m_identificationIndices.push_back(volumeIndex);
    m_identificationIndices.push_back(mapIndex);
    m_identificationIndices.push_back(voxelI);
    m_identificationIndices.push_back(voxelJ);
    m_identificationIndices.push_back(voxelK);
    m_identificationIndices.push_back(intPointerDiffXYZ[0]);
    m_identificationIndices.push_back(intPointerDiffXYZ[1]);
    m_identificationIndices.push_back(intPointerDiffXYZ[2]);
}

/**
 * Process voxel identification.
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::processIdentification()
{
    int32_t identifiedItemIndex;
    float depth = -1.0;
    m_fixedPipelineDrawing->getIndexFromColorSelection(SelectionItemDataTypeEnum::VOXEL,
                                                       m_fixedPipelineDrawing->mouseX,
                                                       m_fixedPipelineDrawing->mouseY,
                                                       identifiedItemIndex,
                                                       depth);
    if (identifiedItemIndex >= 0) {
        const int32_t idIndex = identifiedItemIndex * IDENTIFICATION_INDICES_PER_VOXEL;
        const int32_t volDrawInfoIndex = m_identificationIndices[idIndex];
        CaretAssertVectorIndex(m_volumeDrawInfo, volDrawInfoIndex);
        VolumeMappableInterface* vf = m_volumeDrawInfo[volDrawInfoIndex].volumeFile;
        const int64_t voxelIndices[3] = {
            m_identificationIndices[idIndex + 2],
            m_identificationIndices[idIndex + 3],
            m_identificationIndices[idIndex + 4]
        };
        
        const float* floatDiffXYZ = (float*)&m_identificationIndices[idIndex + 5];
        
        SelectionItemVoxel* voxelID = m_brain->getSelectionManager()->getVoxelIdentification();
        if (voxelID->isEnabledForSelection()) {
            if (voxelID->isOtherScreenDepthCloserToViewer(depth)) {
                voxelID->setVoxelIdentification(m_brain,
                                                vf,
                                                voxelIndices,
                                                depth);
                
                float voxelCoordinates[3];
                vf->indexToSpace(voxelIndices[0], voxelIndices[1], voxelIndices[2],
                                 voxelCoordinates[0], voxelCoordinates[1], voxelCoordinates[2]);
                
                m_fixedPipelineDrawing->setSelectedItemScreenXYZ(voxelID,
                                                                 voxelCoordinates);
                CaretLogFinest("Selected Voxel (3D): " + AString::fromNumbers(voxelIndices, 3, ","));
            }
        }
        
        SelectionItemVoxelEditing* voxelEditID = m_brain->getSelectionManager()->getVoxelEditingIdentification();
        if (voxelEditID->isEnabledForSelection()) {
            if (voxelEditID->getVolumeFileForEditing() == vf) {
                if (voxelEditID->isOtherScreenDepthCloserToViewer(depth)) {
                    voxelEditID->setVoxelIdentification(m_brain,
                                                        vf,
                                                        voxelIndices,
                                                        depth);
                    voxelEditID->setVoxelDiffXYZ(floatDiffXYZ);
                    
                    float voxelCoordinates[3];
                    vf->indexToSpace(voxelIndices[0], voxelIndices[1], voxelIndices[2],
                                     voxelCoordinates[0], voxelCoordinates[1], voxelCoordinates[2]);
                    
                    m_fixedPipelineDrawing->setSelectedItemScreenXYZ(voxelEditID,
                                                                     voxelCoordinates);
                    CaretLogFinest("Selected Voxel Editing (3D): Indices ("
                                   + AString::fromNumbers(voxelIndices, 3, ",")
                                   + ") Diff XYZ ("
                                   + AString::fromNumbers(floatDiffXYZ, 3, ",")
                                   + ")");
                }
            }
        }
    }
}

/**
 * Get the maximum bounds that enclose the volumes and the minimum
 * voxel spacing from the volumes.
 *
 * @param boundsOut
 *    Bounds of the volumes.
 * @param spacingOut
 *    Minimum voxel spacing from the volumes.  Always positive values (even if
 *    volumes is oriented right to left).
 *
 */
bool
BrainOpenGLVolumeObliqueSliceDrawing::getVoxelCoordinateBoundsAndSpacing(float boundsOut[6],
                                                                float spacingOut[3])
{
    const int32_t numberOfVolumesToDraw = static_cast<int32_t>(m_volumeDrawInfo.size());
    if (numberOfVolumesToDraw <= 0) {
        return false;
    }
    
    /*
     * Find maximum extent of all voxels and smallest voxel
     * size in each dimension.
     */
    float minVoxelX = std::numeric_limits<float>::max();
    float maxVoxelX = -std::numeric_limits<float>::max();
    float minVoxelY = std::numeric_limits<float>::max();
    float maxVoxelY = -std::numeric_limits<float>::max();
    float minVoxelZ = std::numeric_limits<float>::max();
    float maxVoxelZ = -std::numeric_limits<float>::max();
    float voxelStepX = std::numeric_limits<float>::max();
    float voxelStepY = std::numeric_limits<float>::max();
    float voxelStepZ = std::numeric_limits<float>::max();
    for (int32_t i = 0; i < numberOfVolumesToDraw; i++) {
        const VolumeMappableInterface* volumeFile = m_volumeDrawInfo[i].volumeFile;
        int64_t dimI, dimJ, dimK, numMaps, numComponents;
        volumeFile->getDimensions(dimI, dimJ, dimK, numMaps, numComponents);
        
        float originX, originY, originZ;
        float x1, y1, z1;
        float lastX, lastY, lastZ;
        volumeFile->indexToSpace(0, 0, 0, originX, originY, originZ);
        volumeFile->indexToSpace(1, 1, 1, x1, y1, z1);
        volumeFile->indexToSpace(dimI - 1, dimJ - 1, dimK - 1, lastX, lastY, lastZ);
        const float dx = x1 - originX;
        const float dy = y1 - originY;
        const float dz = z1 - originZ;
        voxelStepX = std::min(voxelStepX, std::fabs(dx));
        voxelStepY = std::min(voxelStepY, std::fabs(dy));
        voxelStepZ = std::min(voxelStepZ, std::fabs(dz));
        
        minVoxelX = std::min(minVoxelX, std::min(originX, lastX));
        maxVoxelX = std::max(maxVoxelX, std::max(originX, lastX));
        minVoxelY = std::min(minVoxelY, std::min(originY, lastY));
        maxVoxelY = std::max(maxVoxelY, std::max(originY, lastY));
        minVoxelZ = std::min(minVoxelZ, std::min(originZ, lastZ));
        maxVoxelZ = std::max(maxVoxelZ, std::max(originZ, lastZ));
    }
    
    boundsOut[0] = minVoxelX;
    boundsOut[1] = maxVoxelX;
    boundsOut[2] = minVoxelY;
    boundsOut[3] = maxVoxelY;
    boundsOut[4] = minVoxelZ;
    boundsOut[5] = maxVoxelZ;
    
    spacingOut[0] = voxelStepX;
    spacingOut[1] = voxelStepY;
    spacingOut[2] = voxelStepZ;
    
    /*
     * Two dimensions: single slice
     * Three dimensions: multiple slices
     */
    int32_t validDimCount = 0;
    if (maxVoxelX > minVoxelX) validDimCount++;
    if (maxVoxelY > minVoxelY) validDimCount++;
    if (maxVoxelZ > minVoxelZ) validDimCount++;
    
    bool valid = false;
    if ((validDimCount >= 2)
        && (voxelStepX > 0.0)
        && (voxelStepY > 0.0)
        && (voxelStepZ > 0.0)) {
        valid = true;
    }
    
    return valid;
}

/**
 * Create the oblique transformation matrix.
 *
 * @param sliceCoordinates
 *    Slice that is being drawn.
 * @param obliqueTransformationMatrixOut
 *    OUTPUT transformation matrix for oblique viewing.
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::createObliqueTransformationMatrix(const float sliceCoordinates[3],
                                                                 Matrix4x4& obliqueTransformationMatrixOut)
{
    /*
     * Initialize the oblique transformation matrix
     */
    obliqueTransformationMatrixOut.identity();
    
    /*
     * Get the oblique rotation matrix
     */
    Matrix4x4 obliqueRotationMatrix = m_browserTabContent->getObliqueVolumeRotationMatrix();
    
    /*
     * Create the transformation matrix
     */
    obliqueTransformationMatrixOut.postmultiply(obliqueRotationMatrix);
    
    /*
     * Translate to selected coordinate
     */
    obliqueTransformationMatrixOut.translate(sliceCoordinates[0],
                                             sliceCoordinates[1],
                                             sliceCoordinates[2]);
}

/* ======================================================================= */

/**
 * Constructor
 *
 * @param volumeMappableInterface
 *   Volume that contains the data values.
 * @param mapIndex
 *   Index of selected map.
 */
BrainOpenGLVolumeObliqueSliceDrawing::VolumeSlice::VolumeSlice(VolumeMappableInterface* volumeMappableInterface,
                                                               const int32_t mapIndex,
                                                               const float opacity)
{
    m_volumeMappableInterface = volumeMappableInterface;
    m_volumeFile = dynamic_cast<VolumeFile*>(m_volumeMappableInterface);
    m_ciftiMappableDataFile = dynamic_cast<CiftiMappableDataFile*>(m_volumeMappableInterface);
    
    CaretAssert(m_volumeMappableInterface);
    m_mapIndex = mapIndex;
    CaretAssert(m_mapIndex >= 0);
    
    m_opacity = opacity;
    CaretAssert((m_opacity >= 0.0f) && (m_opacity <= 1.0f));
    
    const int64_t sliceDim = 300;
    const int64_t numVoxels = sliceDim * sliceDim;
    m_values.reserve(numVoxels);
    m_valuesGreen.reserve(numVoxels);
    m_valuesBlue.reserve(numVoxels);
    m_valuesAlpha.reserve(numVoxels);
}

/**
 * Add a value and return its index.
 *
 * @param value
 *     Value that is added.
 * @return
 *     The index for the value.
 */
int64_t
BrainOpenGLVolumeObliqueSliceDrawing::VolumeSlice::addValue(const float value)
{
    const int64_t indx = static_cast<int64_t>(m_values.size());
    m_values.push_back(value);
    return indx;
}

/**
 * Add values for RGBA and returns its index.
 *
 * @param value
 *     Value that is added.
 * @return
 *     The index for the value.
 */
int64_t
BrainOpenGLVolumeObliqueSliceDrawing::VolumeSlice::addValuesRGBA(const float values[4])
{
    const int64_t indx = static_cast<int64_t>(m_values.size());
    m_values.push_back(values[0]);
    m_valuesGreen.push_back(values[1]);
    m_valuesBlue.push_back(values[2]);
    m_valuesAlpha.push_back(values[3]);
    return indx;
}

/**
 * Return RGBA colors for value using the value's index
 * returned by addValue().
 *
 * @param indx
 *    Index of the value.
 * @return
 *    RGBA coloring for value.
 */
uint8_t*
BrainOpenGLVolumeObliqueSliceDrawing::VolumeSlice::getRgbaForValueByIndex(const int64_t indx)
{
    CaretAssertVectorIndex(m_rgba, indx * 4);
    return &m_rgba[indx*4];
}

/**
 * Allocate colors for the voxel values
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::VolumeSlice::allocateColors()
{
    m_rgba.resize(m_values.size() * 4);
}

/* ======================================================================= */
/**
 * Create a voxel for drawing.
 *
 * @param center
 *    Center of voxel.
 * @param leftBottom
 *    Left bottom coordinate of voxel.
 * @param rightBottom
 *    Right bottom coordinate of voxel.
 * @param rightTop
 *    Right top coordinate of voxel.
 * @param leftTop
 *    Left top coordinate of voxel.
 */
BrainOpenGLVolumeObliqueSliceDrawing::VoxelToDraw::VoxelToDraw(const float center[3],
                                                        const double leftBottom[3],
                                                        const double rightBottom[3],
                                                        const double rightTop[3],
                                                        const double leftTop[3])
{
    m_center[0] = center[0];
    m_center[1] = center[1];
    m_center[2] = center[2];
    
    m_coordinates[0]  = leftBottom[0];
    m_coordinates[1]  = leftBottom[1];
    m_coordinates[2]  = leftBottom[2];
    m_coordinates[3]  = rightBottom[0];
    m_coordinates[4]  = rightBottom[1];
    m_coordinates[5]  = rightBottom[2];
    m_coordinates[6]  = rightTop[0];
    m_coordinates[7]  = rightTop[1];
    m_coordinates[8]  = rightTop[2];
    m_coordinates[9]  = leftTop[0];
    m_coordinates[10] = leftTop[1];
    m_coordinates[11] = leftTop[2];
    
    const int64_t numSlices = 5;
    m_sliceIndices.reserve(numSlices);
    m_sliceOffsets.reserve(numSlices);
}

/**
 * Get the change in XYZ for the voxel ([top right] minus [bottom left])
 *
 * @param dxyzOut
 *     Change in XYZ from bottom left to top right
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::VoxelToDraw::getDiffXYZ(float dxyzOut[3]) const
{
    dxyzOut[0] = m_coordinates[6] - m_coordinates[0];
    dxyzOut[1] = m_coordinates[7] - m_coordinates[1];
    dxyzOut[2] = m_coordinates[8] - m_coordinates[2];
}

/**
 * Add a value from a volume slice.
 *
 * @param sliceIndex
 *    Index of the slice.
 * @param sliceOffset
 *    Offset of value in the slice.
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::VoxelToDraw::addVolumeValue(const int64_t sliceIndex,
                                                           const int64_t sliceOffset)
{
    CaretAssert(sliceIndex >= 0);
    CaretAssert(sliceOffset >= 0);
    m_sliceIndices.push_back(sliceIndex);
    m_sliceOffsets.push_back(sliceOffset);
}

/**
 * Draw an oblique slice with support for outlining labels and thresholded palette data.
 *
 * @param sliceViewPlane
 *    The plane for slice drawing.
 * @param transformationMatrix
 *    The for oblique viewing.
 * @param plane
 *    Plane equation for the selected slice.
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::drawObliqueSliceWithOutlines(const VolumeSliceViewPlaneEnum::Enum sliceViewPlane,
                                                                   Matrix4x4& transformationMatrix)
{
    /*
     * When performing voxel identification for editing voxels,
     * we need to draw EVERY voxel since the user may click
     * regions where the voxels are "off".
     */
    float voxelEditingValue = 1.0;
    VolumeFile* voxelEditingVolumeFile = NULL;
    bool volumeEditingDrawAllVoxelsFlag = false;
    if (m_identificationModeFlag) {
        SelectionItemVoxelEditing* voxelEditID = m_brain->getSelectionManager()->getVoxelEditingIdentification();
        if (voxelEditID->isEnabledForSelection()) {
            voxelEditingVolumeFile = voxelEditID->getVolumeFileForEditing();
            if (voxelEditingVolumeFile != NULL) {
                volumeEditingDrawAllVoxelsFlag = true;
                if (voxelEditingVolumeFile->isMappedWithLabelTable()) {
                    if (voxelEditingVolumeFile->getNumberOfMaps() > 0) {
                        voxelEditingValue = voxelEditingVolumeFile->getMapLabelTable(0)->getUnassignedLabelKey();
                    }
                }
            }
        }
    }
    
    const bool obliqueSliceModeThreeDimFlag = false;
    
    float m[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, m);
    Matrix4x4 tm;
    tm.setMatrixFromOpenGL(m);
    
    const int32_t numVolumes = static_cast<int32_t>(m_volumeDrawInfo.size());
    
    /*
     * Get the maximum bounds of the voxels from all slices
     * and the smallest voxel spacing
     */
    float voxelBounds[6];
    float voxelSpacing[3];
    if (false == getVoxelCoordinateBoundsAndSpacing(voxelBounds,
                                                    voxelSpacing)) {
        return;
    }
    float voxelSize = std::min(voxelSpacing[0],
                               std::min(voxelSpacing[1],
                                        voxelSpacing[2]));
    
    /*
     * Use a larger voxel size for the 3D view in volume slice viewing
     * since it draws all three slices and this takes time
     */
    if (obliqueSliceModeThreeDimFlag) {
        voxelSize *= 3.0;
    }
    
    /*
     * Look at point is in center of volume
     */
    float translation[3];
    m_browserTabContent->getTranslation(translation);
    float viewOffsetX = 0.0;
    float viewOffsetY = 0.0;
    switch (sliceViewPlane) {
        case VolumeSliceViewPlaneEnum::ALL:
            CaretAssert(0);
            break;
        case VolumeSliceViewPlaneEnum::AXIAL:
            viewOffsetX = (m_lookAtCenter[0] + translation[0]);
            viewOffsetY = (m_lookAtCenter[1] + translation[1]);
            break;
        case VolumeSliceViewPlaneEnum::CORONAL:
            viewOffsetX = (m_lookAtCenter[0] + translation[0]);
            viewOffsetY = (m_lookAtCenter[2] + translation[2]);
            break;
        case VolumeSliceViewPlaneEnum::PARASAGITTAL:
            viewOffsetX = (m_lookAtCenter[1] + translation[1]);
            viewOffsetY = (m_lookAtCenter[2] + translation[2]);
            break;
    }
    
    float minScreenX = m_orthographicBounds[0] - viewOffsetX;
    float maxScreenX = m_orthographicBounds[1] - viewOffsetX;
    float minScreenY = m_orthographicBounds[2] - viewOffsetY;
    float maxScreenY = m_orthographicBounds[3] - viewOffsetY;
    
    
    /*
     * Get origin voxel IJK
     */
    const float zeroXYZ[3] = { 0.0, 0.0, 0.0 };
    int64_t originIJK[3];
    m_volumeDrawInfo[0].volumeFile->enclosingVoxel(zeroXYZ[0], zeroXYZ[1], zeroXYZ[2],
                                                   originIJK[0], originIJK[1], originIJK[2]);
    
    
    /*
     * Get XYZ center of origin Voxel
     */
    float originVoxelXYZ[3];
    m_volumeDrawInfo[0].volumeFile->indexToSpace(originIJK, originVoxelXYZ);
    float actualOrigin[3];
    m_volumeDrawInfo[0].volumeFile->indexToSpace(originIJK, actualOrigin);
    
    float screenOffsetX = 0.0;
    float screenOffsetY = 0.0;
    float originOffsetX = 0.0;
    float originOffsetY = 0.0;
    switch (sliceViewPlane) {
        case VolumeSliceViewPlaneEnum::ALL:
            CaretAssert(0);
            break;
        case VolumeSliceViewPlaneEnum::AXIAL:
            screenOffsetX = m_lookAtCenter[0];
            screenOffsetY = m_lookAtCenter[1];
            originOffsetX = actualOrigin[0];
            originOffsetY = actualOrigin[1];
            break;
        case VolumeSliceViewPlaneEnum::CORONAL:
            screenOffsetX = m_lookAtCenter[0];
            screenOffsetY = m_lookAtCenter[2];
            originOffsetX = actualOrigin[0];
            originOffsetY = actualOrigin[2];
            break;
        case VolumeSliceViewPlaneEnum::PARASAGITTAL:
            screenOffsetX = m_lookAtCenter[1];
            screenOffsetY = m_lookAtCenter[2];
            originOffsetX = actualOrigin[1];
            originOffsetY = actualOrigin[2];
            break;
    }
    
    const int32_t alignVoxelsFlag = 1;
    if (alignVoxelsFlag == 1) {
        /*
         * Adjust for when selected slices are not at the origin
         */
        const float xOffset = MathFunctions::remainder(screenOffsetX, voxelSize);
        const float yOffset = MathFunctions::remainder(screenOffsetY, voxelSize);
        originOffsetX -= xOffset;
        originOffsetY -= yOffset;
        
        const int64_t numVoxelsToLeft = static_cast<int64_t>(MathFunctions::round(minScreenX + originOffsetX) / voxelSize);
        const int64_t numVoxelsToRight = static_cast<int64_t>(MathFunctions::round(maxScreenX + originOffsetX) / voxelSize);
        const int64_t numVoxelsToBottom = static_cast<int64_t>(MathFunctions::round(minScreenY + originOffsetY) / voxelSize);
        const int64_t numVoxelsToTop = static_cast<int64_t>(MathFunctions::round(maxScreenY + originOffsetY)/ voxelSize);
        
        const float halfVoxel = voxelSize / 2.0;
        
        const float firstVoxelCenterX = (numVoxelsToLeft * voxelSize) + originOffsetX;
        const float lastVoxelCenterX = (numVoxelsToRight * voxelSize) + originOffsetX;
        
        const float firstVoxelCenterY = (numVoxelsToBottom * voxelSize) + originOffsetY;
        const float lastVoxelCenterY = (numVoxelsToTop * voxelSize) + originOffsetY;
        
        float newMinScreenX = firstVoxelCenterX - halfVoxel;
        float newMaxScreenX = lastVoxelCenterX + halfVoxel;
        float newMinScreenY = firstVoxelCenterY - halfVoxel;
        float newMaxScreenY = lastVoxelCenterY + halfVoxel;
        
        if (debugFlag) {
            const AString msg2 = ("Origin Voxel Coordinate: ("
                                  + AString::fromNumbers(actualOrigin, 3, ",")
                                  + "\n   Oblique Screen X: ("
                                  + AString::number(minScreenX)
                                  + ","
                                  + AString::number(maxScreenX)
                                  + ") Y: ("
                                  + AString::number(minScreenY)
                                  + ","
                                  + AString::number(maxScreenY)
                                  + ")\nNew X: ("
                                  + AString::number(newMinScreenX)
                                  + ","
                                  + AString::number(newMaxScreenX)
                                  + ") Y: ("
                                  + AString::number(newMinScreenY)
                                  + ","
                                  + AString::number(newMaxScreenY)
                                  + ") Diff: ("
                                  + AString::number((newMaxScreenX - newMinScreenX) / voxelSize)
                                  + ","
                                  + AString::number((newMaxScreenY - newMinScreenY) / voxelSize)
                                  + ")");
            std::cout << qPrintable(msg2) << std::endl;
        }
        
        minScreenX = newMinScreenX;
        maxScreenX = newMaxScreenX;
        minScreenY = newMinScreenY;
        maxScreenY = newMaxScreenY;
    }
    
    /*
     * Set the corners of the screen for the respective view
     */
    float bottomLeft[3];
    float bottomRight[3];
    float topRight[3];
    float topLeft[3];
    switch (sliceViewPlane) {
        case VolumeSliceViewPlaneEnum::ALL:
            CaretAssert(0);
            break;
        case VolumeSliceViewPlaneEnum::AXIAL:
            bottomLeft[0] = minScreenX;
            bottomLeft[1] = minScreenY;
            bottomLeft[2] = 0.0;
            bottomRight[0] = maxScreenX;
            bottomRight[1] = minScreenY;
            bottomRight[2] = 0.0;
            topRight[0] = maxScreenX;
            topRight[1] = maxScreenY;
            topRight[2] = 0.0;
            topLeft[0] = minScreenX;
            topLeft[1] = maxScreenY;
            topLeft[2] = 0.0;
            break;
        case VolumeSliceViewPlaneEnum::CORONAL:
            bottomLeft[0] = minScreenX;
            bottomLeft[1] = 0.0;
            bottomLeft[2] = minScreenY;
            bottomRight[0] = maxScreenX;
            bottomRight[1] = 0.0;
            bottomRight[2] = minScreenY;
            topRight[0] = maxScreenX;
            topRight[1] = 0.0;
            topRight[2] = maxScreenY;
            topLeft[0] = minScreenX;
            topLeft[1] = 0.0;
            topLeft[2] = maxScreenY;
            break;
        case VolumeSliceViewPlaneEnum::PARASAGITTAL:
            bottomLeft[0] = 0.0;
            bottomLeft[1] = minScreenX;
            bottomLeft[2] = minScreenY;
            bottomRight[0] = 0.0;
            bottomRight[1] = maxScreenX;
            bottomRight[2] = minScreenY;
            topRight[0] = 0.0;
            topRight[1] = maxScreenX;
            topRight[2] = maxScreenY;
            topLeft[0] = 0.0;
            topLeft[1] = minScreenX;
            topLeft[2] = maxScreenY;
            break;
    }
    
    
    /*
     * Transform the corners of the screen into model coordinates
     */
    transformationMatrix.multiplyPoint3(bottomLeft);
    transformationMatrix.multiplyPoint3(bottomRight);
    transformationMatrix.multiplyPoint3(topRight);
    transformationMatrix.multiplyPoint3(topLeft);
    
    if (debugFlag) {
        const double bottomDist = MathFunctions::distance3D(bottomLeft, bottomRight);
        const double topDist = MathFunctions::distance3D(topLeft, topRight);
        const double bottomVoxels = bottomDist / voxelSize;
        const double topVoxels = topDist / voxelSize;
        const AString msg = ("Bottom Dist: "
                             + AString::number(bottomDist)
                             + " voxel size: "
                             + AString::number(bottomVoxels)
                             + " Top Dist: "
                             + AString::number(bottomDist)
                             + " voxel size: "
                             + AString::number(topVoxels));
        std::cout << qPrintable(msg) << std::endl;
    }
    
    if (debugFlag) {
        m_fixedPipelineDrawing->setLineWidth(3.0);
        glColor3f(1.0, 0.0, 0.0);
        glBegin(GL_LINE_LOOP);
        glVertex3fv(bottomLeft);
        glVertex3fv(bottomRight);
        glVertex3fv(topRight);
        glVertex3fv(topLeft);
        glEnd();
    }
    
    /*
     * Unit vector and distance in model coords along left side of screen
     */
    double bottomLeftToTopLeftUnitVector[3] = {
        topLeft[0] - bottomLeft[0],
        topLeft[1] - bottomLeft[1],
        topLeft[2] - bottomLeft[2],
    };
    MathFunctions::normalizeVector(bottomLeftToTopLeftUnitVector);
    const double bottomLeftToTopLeftDistance = MathFunctions::distance3D(bottomLeft,
                                                                         topLeft);
    
    /*
     * Unit vector and distance in model coords along right side of screen
     */
    double bottomRightToTopRightUnitVector[3] = {
        topRight[0] - bottomRight[0],
        topRight[1] - bottomRight[1],
        topRight[2] - bottomRight[2]
    };
    MathFunctions::normalizeVector(bottomRightToTopRightUnitVector);
    const double bottomRightToTopRightDistance = MathFunctions::distance3D(bottomRight,
                                                                           topRight);
    /*
     * For fastest coloring, need to color data values as a group
     */
    std::vector<VolumeSlice> volumeSlices;
    for (int32_t i = 0; i < numVolumes; i++) {
        if (m_volumeDrawInfo[i].opacity > 0.0) {
            volumeSlices.push_back(VolumeSlice(m_volumeDrawInfo[i].volumeFile,
                                               m_volumeDrawInfo[i].mapIndex,
                                               m_volumeDrawInfo[i].opacity));
        }
        
    }

    if ((bottomLeftToTopLeftDistance > 0)
        && (bottomRightToTopRightDistance > 0)) {
        float leftToRightStepXYZ[3] = {
            bottomRight[0] - bottomLeft[0],
            bottomRight[1] - bottomLeft[1],
            bottomRight[2] - bottomLeft[2]
        };
        const float bottomLeftToBottomRightDistance = MathFunctions::normalizeVector(leftToRightStepXYZ);
        leftToRightStepXYZ[0] *= voxelSize;
        leftToRightStepXYZ[1] *= voxelSize;
        leftToRightStepXYZ[2] *= voxelSize;
        
        const int32_t numberOfRows = std::round(bottomLeftToTopLeftDistance / voxelSize);
        const int32_t numberOfColumns = std::round(bottomLeftToBottomRightDistance / voxelSize);
        
        
        float bottomToTopStepXYZ[3] = {
            topLeft[0] - bottomLeft[0],
            topLeft[1] - bottomLeft[1],
            topLeft[2] - bottomLeft[2]
        };
        MathFunctions::normalizeVector(bottomToTopStepXYZ);
        bottomToTopStepXYZ[0] *= voxelSize;
        bottomToTopStepXYZ[1] *= voxelSize;
        bottomToTopStepXYZ[2] *= voxelSize;
        
        const int32_t browserTabIndex = m_browserTabContent->getTabNumber();
        const DisplayPropertiesLabels* displayPropertiesLabels = m_brain->getDisplayPropertiesLabels();
        const DisplayGroupEnum::Enum displayGroup = displayPropertiesLabels->getDisplayGroupForTab(browserTabIndex);
        
        std::vector<ObliqueSlice*> slices;
        for (int32_t iVol = 0; iVol < numVolumes; iVol++) {
            const BrainOpenGLFixedPipeline::VolumeDrawInfo& vdi = m_volumeDrawInfo[iVol];
            VolumeMappableInterface* volInter = vdi.volumeFile;
            
            bool volumeEditDrawAllVoxelsFlag = false;
            if (voxelEditingVolumeFile != NULL) {
                if (voxelEditingVolumeFile == m_volumeDrawInfo[iVol].volumeFile) {
                    volumeEditDrawAllVoxelsFlag = true;
                }
            }
            ObliqueSlice* slice = new ObliqueSlice(m_fixedPipelineDrawing,
                                                   volInter,
                                                   m_volumeDrawInfo[iVol].opacity,
                                                   m_volumeDrawInfo[iVol].mapIndex,
                                                   numberOfRows,
                                                   numberOfColumns,
                                                   browserTabIndex,
                                                   displayPropertiesLabels,
                                                   displayGroup,
                                                   bottomLeft,
                                                   leftToRightStepXYZ,
                                                   bottomToTopStepXYZ,
                                                   m_obliqueSliceMaskingType,
                                                   voxelEditingValue,
                                                   volumeEditDrawAllVoxelsFlag,
                                                   m_identificationModeFlag);
            slices.push_back(slice);
        }
        
        const int32_t numSlices = static_cast<int32_t>(slices.size());
        if (numSlices > 0) {
            bool drawAllSlicesFlag = true;
            const bool compositeFlag = true;
            if (compositeFlag
                && ( ! m_identificationModeFlag)) {
                CaretAssertVectorIndex(slices, 0);
                ObliqueSlice* underlaySlice = slices[0];
                if (numSlices > 1) {
                    std::vector<ObliqueSlice*> overlaySlices;
                    for (int32_t i = 1; i < numSlices; i++) {
                        CaretAssertVectorIndex(slices, i);
                        overlaySlices.push_back(slices[i]);
                    }
                    if (underlaySlice->compositeSlicesRGBA(overlaySlices)) {
                        underlaySlice->draw(m_fixedPipelineDrawing);
                        drawAllSlicesFlag = false;
                    }
                }
            }
            
            if (drawAllSlicesFlag) {
                for (auto s : slices) {
                    s->draw(m_fixedPipelineDrawing);
                }
            }
        }
        
        for (auto s : slices) {
            delete s;
        }
        slices.clear();
    }
}

/**
 * Constructor for drawing an oblique slice.
 *
 * @param fixedPipelineDrawing
 *    The fixed pipeline drawing.
 * @param volumeInterface
 *    Interface for volume-type file being drawn.
 * @param opacity
 *    Opacity for drawing the slice.
 * @param mapIndex
 *    Index of map in file being drawn.
 * @param numberOfRows
 *    Number of rows used when drawing slice.
 * @param numberOfColumns
 *    Number of columns used when drawing slice.
 * @param browserTabIndex
 *    Index of browser tab being drawn.
 * @param displayPropertiesLabels
 *    Display properties for labels.
 * @param displayGroup
 *    Selected display group for labels.
 * @param originXYZ
 *    XYZ coordinate of first voxel in slice.
 * @param leftToRightStepXYZ
 *    XYZ step for one voxel in screen left to right.
 * @param bottomToTopStepXYZ
 *    XYZ step for one voxel in screen bottom to top.
 * @param maskingType
 *    Masking type to eliminate interpolation artifacts
 * @param voxelEditingValue
 *    Value used when editing voxels.
 * @param volumeEditingDrawAllVoxelsFlag
 *    Draw all voxels when editing a volume.
 * @param identificationModeFlag
 *    True if identification mode is active.
 */
BrainOpenGLVolumeObliqueSliceDrawing::ObliqueSlice::ObliqueSlice(BrainOpenGLFixedPipeline* fixedPipelineDrawing,
                                                                 VolumeMappableInterface* volumeInterface,
                                                                 const float opacity,
                                                                 const int32_t mapIndex,
                                                                 const int32_t numberOfRows,
                                                                 const int32_t numberOfColumns,
                                                                 const int32_t browserTabIndex,
                                                                 const DisplayPropertiesLabels* displayPropertiesLabels,
                                                                 const DisplayGroupEnum::Enum displayGroup,
                                                                 const float originXYZ[3],
                                                                 const float leftToRightStepXYZ[3],
                                                                 const float bottomToTopStepXYZ[3],
                                                                 const VolumeSliceInterpolationEdgeEffectsMaskingEnum::Enum maskingType,
                                                                 const float voxelEditingValue,
                                                                 const bool volumeEditingDrawAllVoxelsFlag,
                                                                 const bool identificationModeFlag)
:
m_volumeInterface(volumeInterface),
m_opacity(opacity),
m_mapFile(dynamic_cast<const CaretMappableDataFile*>(volumeInterface)),
m_mapIndex(mapIndex),
m_numberOfRows(numberOfRows),
m_numberOfColumns(numberOfColumns),
m_browserTabIndex(browserTabIndex),
m_displayPropertiesLabels(displayPropertiesLabels),
m_displayGroup(displayGroup),
m_identificationX(fixedPipelineDrawing->mouseX),
m_identificationY(fixedPipelineDrawing->mouseY),
m_identificationModeFlag(identificationModeFlag)
{
    CaretAssert(volumeInterface);
    CaretAssert(m_mapFile);
    
    m_opacityByte = 255;
    if (m_opacity >= 1.0) {
        m_opacityByte = 255;
    }
    if (m_opacity <= 0.0) {
        m_opacityByte = 0;
    }
    else {
        m_opacityByte = static_cast<uint8_t>(m_opacity * 255.0f);
    }
    
    for (int32_t i = 0; i < 3; i++) {
        m_selectionIJK[i] = -1;
        m_leftToRightStepXYZ[i] = leftToRightStepXYZ[i];
        m_bottomToTopStepXYZ[i] = bottomToTopStepXYZ[i];
        m_originXYZ[i] = originXYZ[i];
    }

    const VolumeFile* volumeFileConst =  dynamic_cast<const VolumeFile*>(volumeInterface);
    m_volumeFile = const_cast<VolumeFile*>(volumeFileConst);
    const CiftiMappableDataFile* ciftiMappableFileConst = dynamic_cast<const CiftiMappableDataFile*>(volumeInterface);
    m_ciftiMappableFile = const_cast<CiftiMappableDataFile*>(ciftiMappableFileConst);
    
    m_dataValueType = DataValueType::INVALID;
    if (m_mapFile->isMappedWithPalette()) {
        if (m_volumeFile != NULL) {
            m_dataValueType = DataValueType::VOLUME_PALETTE;
        }
        else if (m_ciftiMappableFile != NULL) {
            m_dataValueType = DataValueType::CIFTI_PALETTE;
        }
        else {
            CaretAssert(0);
        }
    }
    else if (m_mapFile->isMappedWithLabelTable()) {
        if (m_volumeFile != NULL) {
            m_dataValueType = DataValueType::VOLUME_LABEL;
        }
        else if (m_ciftiMappableFile != NULL) {
            m_dataValueType = DataValueType::CIFTI_LABEL;
        }
        else {
            CaretAssert(0);
        }
    }
    else if (m_mapFile->isMappedWithRGBA()) {
        if (m_volumeFile != NULL) {
            if (m_volumeFile->getNumberOfComponents() == 4) {
                m_dataValueType = DataValueType::VOLUME_RGBA;
            }
            else if (m_volumeFile->getNumberOfComponents() == 3) {
                m_dataValueType = DataValueType::VOLUME_RGB;
            }
        }
        else {
            CaretAssert(0);
        }
    }
    CaretAssert(m_dataValueType != DataValueType::INVALID);
    
    m_sliceNumberOfVoxels     = m_numberOfRows * m_numberOfColumns;
    m_voxelNumberOfComponents = 1;
    switch (m_dataValueType) {
        case DataValueType::INVALID:
            CaretAssert(0);
            break;
        case DataValueType::CIFTI_LABEL:
        case DataValueType::CIFTI_PALETTE:
        case DataValueType::VOLUME_LABEL:
        case DataValueType::VOLUME_PALETTE:
            break;
        case DataValueType::VOLUME_RGB:
        case DataValueType::VOLUME_RGBA:
            m_voxelNumberOfComponents = 4;
            break;
    }
    m_data.reserve(m_sliceNumberOfVoxels
                   * m_voxelNumberOfComponents);
    
    m_thresholdData.reserve(m_data.size());
    
    m_rgba.reserve(m_sliceNumberOfVoxels * 4);
    
    if (m_identificationModeFlag) {
        m_identificationIJK.reserve(m_sliceNumberOfVoxels * 3);
        m_identificationHelper.reset(new IdentificationWithColor());
    }
    
    setThresholdFileAndMap();
    
    loadData(maskingType,
             voxelEditingValue,
             volumeEditingDrawAllVoxelsFlag);
    
    assignRgba(volumeEditingDrawAllVoxelsFlag);
    
    if ( ! m_identificationModeFlag) {
        addOutlines();
    }
    
    CaretAssert((m_sliceNumberOfVoxels*4) == static_cast<int32_t>(m_rgba.size()));
}

/**
 * Set thresholding file and map, if applicable.
 */
bool
BrainOpenGLVolumeObliqueSliceDrawing::ObliqueSlice::setThresholdFileAndMap()
{
    switch (m_dataValueType) {
        case DataValueType::INVALID:
            CaretAssert(0);
            break;
        case DataValueType::CIFTI_LABEL:
            break;
        case DataValueType::CIFTI_PALETTE:
        {
            const PaletteColorMapping* pcm = m_ciftiMappableFile->getMapPaletteColorMapping(m_mapIndex);
            CaretAssert(pcm);
            switch (pcm->getThresholdType()) {
                case PaletteThresholdTypeEnum::THRESHOLD_TYPE_OFF:
                    break;
                case PaletteThresholdTypeEnum::THRESHOLD_TYPE_NORMAL:
                    m_thresholdCiftiMappableFile = m_ciftiMappableFile;
                    m_thresholdMapIndex   = m_mapIndex;
                    break;
                case PaletteThresholdTypeEnum::THRESHOLD_TYPE_FILE:
                {
                    CaretMappableDataFileAndMapSelectionModel* selector = m_ciftiMappableFile->getMapThresholdFileSelectionModel(m_mapIndex);
                    m_thresholdCiftiMappableFile = dynamic_cast<CiftiMappableDataFile*>(selector->getSelectedFile());
                    if (m_thresholdCiftiMappableFile != NULL) {
                        m_thresholdMapIndex = selector->getSelectedMapIndex();
                    }
                }
                    break;
                case PaletteThresholdTypeEnum::THRESHOLD_TYPE_MAPPED:
                case PaletteThresholdTypeEnum::THRESHOLD_TYPE_MAPPED_AVERAGE_AREA:
                    CaretAssert(0);
                    break;
            }
            if (m_thresholdCiftiMappableFile != NULL) {
                if ((m_thresholdMapIndex >= 0)
                    && (m_thresholdMapIndex < m_thresholdCiftiMappableFile->getNumberOfMaps())) {
                    m_thresholdPaletteColorMapping = m_thresholdCiftiMappableFile->getMapPaletteColorMapping(m_thresholdMapIndex);
                }
                else {
                    m_thresholdCiftiMappableFile = NULL;
                    m_thresholdMapIndex = -1;
                }
            }
        }
            break;
        case DataValueType::VOLUME_LABEL:
            break;
        case DataValueType::VOLUME_PALETTE:
        {
            const PaletteColorMapping* pcm = m_volumeFile->getMapPaletteColorMapping(m_mapIndex);
            CaretAssert(pcm);
            switch (pcm->getThresholdType()) {
                case PaletteThresholdTypeEnum::THRESHOLD_TYPE_OFF:
                    break;
                case PaletteThresholdTypeEnum::THRESHOLD_TYPE_NORMAL:
                    m_thresholdVolumeFile = m_volumeFile;
                    m_thresholdMapIndex   = m_mapIndex;
                    break;
                case PaletteThresholdTypeEnum::THRESHOLD_TYPE_FILE:
                {
                    CaretMappableDataFileAndMapSelectionModel* selector = m_volumeFile->getMapThresholdFileSelectionModel(m_mapIndex);
                    m_thresholdVolumeFile = dynamic_cast<VolumeFile*>(selector->getSelectedFile());
                    if (m_thresholdVolumeFile != NULL) {
                        m_thresholdMapIndex = selector->getSelectedMapIndex();
                    }
                }
                    break;
                case PaletteThresholdTypeEnum::THRESHOLD_TYPE_MAPPED:
                case PaletteThresholdTypeEnum::THRESHOLD_TYPE_MAPPED_AVERAGE_AREA:
                    CaretAssert(0);
                    break;
            }
            if (m_thresholdVolumeFile != NULL) {
                if ((m_thresholdMapIndex >= 0)
                    && (m_thresholdMapIndex < m_thresholdVolumeFile->getNumberOfMaps())) {
                    m_thresholdPaletteColorMapping = m_thresholdVolumeFile->getMapPaletteColorMapping(m_thresholdMapIndex);
                }
                else {
                    m_thresholdVolumeFile = NULL;
                    m_thresholdMapIndex = -1;
                }
            }
        }
            break;
        case DataValueType::VOLUME_RGB:
            break;
        case DataValueType::VOLUME_RGBA:
            break;
    }
    
    if (m_thresholdMapIndex < 0) {
        m_thresholdCiftiMappableFile = NULL;
        m_thresholdVolumeFile = NULL;
        m_thresholdPaletteColorMapping = NULL;
    }
    
    return (m_thresholdMapIndex >= 0);
}

/**
 * Assign RGBA coloring for the voxels.
 *
 * @param volumeEditingDrawAllVoxelsFlag
 *    True if editing and all voxels should be drawn for selection.
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::ObliqueSlice::assignRgba(const bool volumeEditingDrawAllVoxelsFlag)
{
    if (m_data.empty()) {
        return;
    }
    
    switch (m_dataValueType) {
        case DataValueType::INVALID:
            CaretAssert(0);
            break;
        case DataValueType::CIFTI_LABEL:
        case DataValueType::CIFTI_PALETTE:
        case DataValueType::VOLUME_LABEL:
        case DataValueType::VOLUME_PALETTE:
            /*
             * One data value per voxel.
             */
            m_rgba.resize(m_data.size() * 4);
            break;
        case DataValueType::VOLUME_RGB:
        case DataValueType::VOLUME_RGBA:
            /*
             * For RGBA, there are four data values per voxel
             */
            m_rgba.resize(m_data.size());
            break;
    }

    if (volumeEditingDrawAllVoxelsFlag) {
        const int32_t numVoxels = static_cast<int32_t>(m_rgba.size() / 4);
        for (int32_t i = 0; i < numVoxels; i++) {
            const int32_t i4 = i * 4;
            m_rgba[i4]   = 255;
            m_rgba[i4+1] = 255;
            m_rgba[i4+2] = 255;
            m_rgba[i4+3] = 255;
        }
        
        return;
    }
    
    switch (m_dataValueType) {
        case DataValueType::INVALID:
            break;
        case DataValueType::CIFTI_LABEL:
        {
            CaretAssert(m_ciftiMappableFile);
            const GiftiLabelTable* labelTable = m_ciftiMappableFile->getMapLabelTable(m_mapIndex);
            CaretAssert(labelTable);
            NodeAndVoxelColoring::colorIndicesWithLabelTableForDisplayGroupTab(labelTable,
                                                                               &m_data[0],
                                                                               m_data.size(),
                                                                               m_displayGroup,
                                                                               m_browserTabIndex,
                                                                               &m_rgba[0]);
        }
            break;
        case DataValueType::CIFTI_PALETTE:
        {
            CaretAssert(m_ciftiMappableFile);
            PaletteColorMapping* pcm = m_ciftiMappableFile->getMapPaletteColorMapping(m_mapIndex);
            CaretAssert(pcm);
            const FastStatistics* stats = m_ciftiMappableFile->getMapFastStatistics(m_mapIndex);
            CaretAssert(stats);
            CaretAssert(m_data.size() == m_thresholdData.size());
            
            PaletteColorMapping* threshPcm = ((m_thresholdPaletteColorMapping != NULL)
                                              ? m_thresholdPaletteColorMapping
                                              : pcm);
            NodeAndVoxelColoring::colorScalarsWithPalette(stats,
                                                          pcm,
                                                          &m_data[0],
                                                          threshPcm,
                                                          &m_thresholdData[0],
                                                          m_data.size(),
                                                          &m_rgba[0]);
        }
            break;
        case DataValueType::VOLUME_LABEL:
        {
            CaretAssert(m_volumeFile);
            const GiftiLabelTable* labelTable = m_volumeFile->getMapLabelTable(m_mapIndex);
            CaretAssert(labelTable);
            NodeAndVoxelColoring::colorIndicesWithLabelTableForDisplayGroupTab(labelTable,
                                                                               &m_data[0],
                                                                               m_data.size(),
                                                                               m_displayGroup,
                                                                               m_browserTabIndex,
                                                                               &m_rgba[0]);
        }
            break;
        case DataValueType::VOLUME_PALETTE:
        {
            CaretAssert(m_volumeFile);
            PaletteColorMapping* pcm = m_volumeFile->getMapPaletteColorMapping(m_mapIndex);
            CaretAssert(pcm);
            const FastStatistics* stats = m_volumeFile->getMapFastStatistics(m_mapIndex);
            CaretAssert(stats);
            CaretAssert(m_data.size() == m_thresholdData.size());
            
            PaletteColorMapping* threshPcm = ((m_thresholdPaletteColorMapping != NULL)
                                              ? m_thresholdPaletteColorMapping
                                              : pcm);
            NodeAndVoxelColoring::colorScalarsWithPalette(stats,
                                                          pcm,
                                                          &m_data[0],
                                                          threshPcm,
                                                          &m_thresholdData[0],
                                                          m_data.size(),
                                                          &m_rgba[0]);
        }
            break;
        case DataValueType::VOLUME_RGB:
        case DataValueType::VOLUME_RGBA:
        {
            /*
             * Test RGB data to see if it ranges from [0, 1] or [0, 255].
             * Assume if any component is greater than 1, then the data is [0, 255]
             */
            bool range255Flag = std::any_of(m_data.begin(),
                                            m_data.end(),
                                            [](float f) { return f > 1.0f; });
            
            CaretAssert(m_data.size() == m_rgba.size());
            const int32_t numVoxels = static_cast<int32_t>(m_data.size() / 4);
            if (range255Flag) {
                for (int32_t i = 0; i < numVoxels; i++) {
                    const int32_t i4 = i * 4;
                    m_rgba[i4] = static_cast<uint8_t>(m_data[i4]);
                    m_rgba[i4+1] = static_cast<uint8_t>(m_data[i4+1]);
                    m_rgba[i4+2] = static_cast<uint8_t>(m_data[i4+2]);
                    if (m_dataValueType == DataValueType::VOLUME_RGB) {
                        m_rgba[i4+3] = ((m_data[i4+3] > 0)
                                        ? 255
                                        : 0);
                    }
                }
            }
            else {
                for (int32_t i = 0; i < numVoxels; i++) {
                    const int32_t i4 = i * 4;
                    m_rgba[i4]   = static_cast<uint8_t>(m_data[i4] * 255.0);
                    m_rgba[i4+1] = static_cast<uint8_t>(m_data[i4+1] * 255.0);
                    m_rgba[i4+2] = static_cast<uint8_t>(m_data[i4+2] * 255.0);
                    m_rgba[i4+3] = static_cast<uint8_t>(m_data[i4+3] * 255.0);
                }
            }
        }
            break;
    }
}

/**
 * Add any outlines for labels or thresholded palette data.
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::ObliqueSlice::addOutlines()
{
    LabelDrawingProperties* labelDrawingProperties = NULL;
    PaletteColorMapping* paletteColorMapping = NULL;
    switch (m_dataValueType) {
        case DataValueType::INVALID:
            break;
        case DataValueType::CIFTI_LABEL:
            CaretAssert(m_ciftiMappableFile);
            labelDrawingProperties = m_ciftiMappableFile->getLabelDrawingProperties();
            break;
        case DataValueType::CIFTI_PALETTE:
            CaretAssert(m_ciftiMappableFile);
            paletteColorMapping = m_ciftiMappableFile->getMapPaletteColorMapping(m_mapIndex);
            break;
        case DataValueType::VOLUME_LABEL:
            CaretAssert(m_volumeFile);
            labelDrawingProperties = m_volumeFile->getLabelDrawingProperties();
            break;
        case DataValueType::VOLUME_PALETTE:
            CaretAssert(m_volumeFile);
            paletteColorMapping = m_volumeFile->getMapPaletteColorMapping(m_mapIndex);
            break;
        case DataValueType::VOLUME_RGB:
            break;
        case DataValueType::VOLUME_RGBA:
            break;
    }
    
    if (labelDrawingProperties != NULL) {
        const LabelDrawingTypeEnum::Enum labelDrawingType = labelDrawingProperties->getDrawingType();
        if (labelDrawingType != LabelDrawingTypeEnum::DRAW_FILLED) {
            const CaretColorEnum::Enum outlineColor = labelDrawingProperties->getOutlineColor();
            NodeAndVoxelColoring::convertSliceColoringToOutlineMode(&m_rgba[0],
                                                                    labelDrawingType,
                                                                    outlineColor,
                                                                    m_numberOfColumns,
                                                                    m_numberOfRows);
        }
    }
    
    if (paletteColorMapping != NULL) {
        const PaletteThresholdOutlineDrawingModeEnum::Enum outlineMode = paletteColorMapping->getThresholdOutlineDrawingMode();
        if (outlineMode != PaletteThresholdOutlineDrawingModeEnum::OFF) {
            const CaretColorEnum::Enum outlineColor = paletteColorMapping->getThresholdOutlineDrawingColor();
            NodeAndVoxelColoring::convertPaletteSliceColoringToOutlineMode(&m_rgba[0],
                                                                           outlineMode,
                                                                           outlineColor,
                                                                           m_numberOfColumns,
                                                                           m_numberOfRows);
        }
    }
}

/**
 * Loads voxels data into the 'slice'.
 * @param maskingType
 *    Masking type to hide artifacts of interpolation
 * @param voxelEditingValue
 *    Value for voxels when editing a volume
 * @param volumeEditingDrawAllVoxelsFlag
 *    True if editing and all voxels should be drawn for selection.
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::ObliqueSlice::loadData(const VolumeSliceInterpolationEdgeEffectsMaskingEnum::Enum maskingType,
                                                             const float voxelEditingValue,
                                                             const bool volumeEditingDrawAllVoxelsFlag)
{
    m_validVoxelCount = 0;
    
    bool needCiftiMapDataFlag = false;
    switch (m_dataValueType) {
        case DataValueType::INVALID:
            break;
        case DataValueType::CIFTI_LABEL:
        case DataValueType::CIFTI_PALETTE:
            needCiftiMapDataFlag = true;
            break;
        case DataValueType::VOLUME_LABEL:
        case DataValueType::VOLUME_PALETTE:
        case DataValueType::VOLUME_RGB:
        case DataValueType::VOLUME_RGBA:
            break;
    }
    
    float voxelCenterOffset[3] = {
        ((m_leftToRightStepXYZ[0] / 2.0f) + (m_bottomToTopStepXYZ[0] / 2.0f)),
        ((m_leftToRightStepXYZ[1] / 2.0f) + (m_bottomToTopStepXYZ[1] / 2.0f)),
        ((m_leftToRightStepXYZ[2] / 2.0f) + (m_bottomToTopStepXYZ[2] / 2.0f)),
    };

    std::vector<float> ciftiFileMapData;
    std::vector<float> thresholdCiftiFileMapData;
    if (needCiftiMapDataFlag) {
        /*
         * Cifti files are slow at getting individual voxels since they
         * provide no access to individual voxels.  The reason is that
         * the data may be on a server (Dense data) and accessing a single
         * voxel would require requesting the entire map.  So, for
         * each Cifti file, get the entire map.
         */
        CaretAssert(m_ciftiMappableFile);
        m_ciftiMappableFile->getMapData(m_mapIndex,
                                        ciftiFileMapData);
        
        if (m_thresholdCiftiMappableFile != NULL) {
            m_thresholdCiftiMappableFile->getMapData(m_thresholdMapIndex,
                                                     thresholdCiftiFileMapData);
        }
    }
    
    for (int32_t iRow = 0; iRow < m_numberOfRows; iRow++) {
        float rowStartXYZ[3] = {
            m_originXYZ[0] + (iRow * m_bottomToTopStepXYZ[0]),
            m_originXYZ[1] + (iRow * m_bottomToTopStepXYZ[1]),
            m_originXYZ[2] + (iRow * m_bottomToTopStepXYZ[2])
        };
        
        for (int32_t iCol = 0; iCol < m_numberOfColumns; iCol++) {
            
            const float voxelBottomLeft[3] = {
                rowStartXYZ[0] + (iCol * m_leftToRightStepXYZ[0]),
                rowStartXYZ[1] + (iCol * m_leftToRightStepXYZ[1]),
                rowStartXYZ[2] + (iCol * m_leftToRightStepXYZ[2])
            };
            
            const float voxelCenter[3] = {
                voxelBottomLeft[0] + voxelCenterOffset[0],
                voxelBottomLeft[1] + voxelCenterOffset[1],
                voxelBottomLeft[2] + voxelCenterOffset[2]
            };
            
            float values[4] = { 0.0, 0.0, 0.0, 0.0 };
            bool valueValidFlag = false;
            
            float thresholdValue = 0.0;
            bool thresholdValueValidFlag = false;
            
            switch (m_dataValueType) {
                case DataValueType::INVALID:
                    CaretAssert(0);
                    break;
                case DataValueType::CIFTI_LABEL:
                case DataValueType::CIFTI_PALETTE:
                {
                    CaretAssert(m_ciftiMappableFile);
                    const int64_t voxelOffset = m_ciftiMappableFile->getMapDataOffsetForVoxelAtCoordinate(voxelCenter,
                                                                                                              m_mapIndex);
                    if (voxelOffset >= 0) {
                        CaretAssertVectorIndex(ciftiFileMapData, voxelOffset);
                        values[0] = ciftiFileMapData[voxelOffset];
                        valueValidFlag = true;
                    }
                    
                    if (m_thresholdCiftiMappableFile != NULL) {
                        const int64_t thresholdVoxelOffset = m_thresholdCiftiMappableFile->getMapDataOffsetForVoxelAtCoordinate(voxelCenter,
                                                                                                                                m_thresholdMapIndex);
                        if (thresholdVoxelOffset >= 0) {
                            CaretAssertVectorIndex(thresholdCiftiFileMapData, thresholdVoxelOffset);
                            thresholdValue = thresholdCiftiFileMapData[thresholdVoxelOffset];
                            thresholdValueValidFlag = true;
                        }
                    }
                }
                    break;
                case DataValueType::VOLUME_LABEL:
                {
                    CaretAssert(m_volumeFile);
                    values[0] = m_volumeFile->getVoxelValue(voxelCenter,
                                                          &valueValidFlag,
                                                          m_mapIndex,
                                                          0);
                }
                    break;
                case DataValueType::VOLUME_PALETTE:
                {
                    CaretAssert(m_volumeFile);
                    values[0] = m_volumeFile->interpolateValue(voxelCenter,
                                                             VolumeFile::CUBIC,
                                                             &valueValidFlag,
                                                             m_mapIndex);
                    
                    if (valueValidFlag
                        && (values[0] != 0.0f)) {
                        /*
                         * Apply masking to oblique voxels (WB-750).
                         * In some instances, CUBIC interpolation may result in a voxel
                         * receiving a very small value (0.000000000000000210882405) and
                         * this will cause the slice drawing to look very unusual.  Masking
                         * is used to prevent this from occurring.
                         *
                         */
                        bool maskValidFlag = false;
                        float maskValue = 0.0f;
                        switch (maskingType) {
                            case VolumeSliceInterpolationEdgeEffectsMaskingEnum::OFF:
                                maskValidFlag = false;
                                break;
                            case VolumeSliceInterpolationEdgeEffectsMaskingEnum::LOOSE:
                                maskValue = m_volumeFile->interpolateValue(voxelCenter,
                                                                         VolumeFile::TRILINEAR,
                                                                         &maskValidFlag,
                                                                         m_mapIndex);
                                break;
                            case VolumeSliceInterpolationEdgeEffectsMaskingEnum::TIGHT:
                                maskValue = m_volumeFile->interpolateValue(voxelCenter,
                                                                         VolumeFile::ENCLOSING_VOXEL,
                                                                         &maskValidFlag,
                                                                         m_mapIndex);
                                break;
                        }
                        
                        if (maskValidFlag
                            && (maskValue == 0.0f)) {
                            values[0] = 0.0f;
                            valueValidFlag = false;
                        }
                    }
                    
                    if (m_thresholdVolumeFile != NULL) {
                        thresholdValue = m_thresholdVolumeFile->getVoxelValue(voxelCenter,
                                                                              &thresholdValueValidFlag,
                                                                              m_thresholdMapIndex,
                                                                              0);
                    }
                }
                    break;
                case DataValueType::VOLUME_RGB:
                case DataValueType::VOLUME_RGBA:
                {
                    values[0] = m_volumeFile->getVoxelValue(voxelCenter,
                                                               &valueValidFlag,
                                                               m_mapIndex,
                                                               0);
                    if (valueValidFlag) {
                        values[1] = m_volumeFile->getVoxelValue(voxelCenter,
                                                                   &valueValidFlag,
                                                                   m_mapIndex,
                                                                   1);
                        values[2] = m_volumeFile->getVoxelValue(voxelCenter,
                                                                   &valueValidFlag,
                                                                   m_mapIndex,
                                                                   2);
                        if (m_dataValueType == DataValueType::VOLUME_RGBA) {
                            values[3] = m_volumeFile->getVoxelValue(voxelCenter,
                                                                       &valueValidFlag,
                                                                       m_mapIndex,
                                                                       3);
                        }
                        else {
                            values[3] = 1.0;
                        }
                    }
                }
                    break;
            }
            
            if (m_thresholdMapIndex < 0) {
                thresholdValue = values[0];
            }
            
            /*
             * Need to draw all voxels when editing
             */
            if (volumeEditingDrawAllVoxelsFlag) {
                if (! valueValidFlag) {
                    values[0] = voxelEditingValue;
                    valueValidFlag = true;
                }
            }
            
            switch (m_dataValueType) {
                case DataValueType::INVALID:
                    break;
                case DataValueType::CIFTI_LABEL:
                case DataValueType::CIFTI_PALETTE:
                case DataValueType::VOLUME_LABEL:
                case DataValueType::VOLUME_PALETTE:
                    m_data.push_back(values[0]);
                    m_thresholdData.push_back(thresholdValue);
                    break;
                case DataValueType::VOLUME_RGB:
                case DataValueType::VOLUME_RGBA:
                    m_data.push_back(values[0]);
                    m_data.push_back(values[1]);
                    m_data.push_back(values[2]);
                    m_data.push_back(values[3]);
                    break;
            }
            
            if (m_identificationModeFlag) {
                /*
                 * When identifying, need an IJK triplet
                 * for each voxel
                 */
                int64_t ijk[3] = { -1, -1, -1 };
                if (valueValidFlag) {
                    if (m_volumeFile != NULL) {
                        m_volumeFile->enclosingVoxel(voxelCenter, ijk);
                        if ( ! m_volumeFile->indexValid(ijk)) {
                            ijk[0] = -1;
                            ijk[1] = -1;
                            ijk[2] = -1;
                            valueValidFlag = false;
                        }
                    }
                    else if (m_ciftiMappableFile != NULL) {
                        int64_t i(-1), j(-1), k(-1);
                        m_ciftiMappableFile->enclosingVoxel(voxelCenter[0], voxelCenter[1], voxelCenter[2],
                                                            i, j, k);
                        if (m_ciftiMappableFile->indexValid(i, j, k)) {
                            ijk[0] = i;
                            ijk[1] = j;
                            ijk[2] = k;
                        }
                        else {
                            valueValidFlag = false;
                        }
                    }
                }
                m_identificationIJK.insert(m_identificationIJK.end(),
                                           ijk, ijk + 3);
            }
            
            if (valueValidFlag) {
                m_validVoxelCount++;
            }
        }
    }
    
    CaretAssert((m_sliceNumberOfVoxels * m_voxelNumberOfComponents) == static_cast<int32_t>(m_data.size()));
    if (m_thresholdMapIndex >= 0) {
        CaretAssert(m_data.size() == m_thresholdData.size());
    }
    
    if (m_identificationModeFlag) {
        CaretAssert((m_data.size() * 3) == m_identificationIJK.size());
    }
}

/**
 * Draw the oblique slice with the graphics primitives.
 *
 * @param fixedPipelineDrawing
 *     The fixed pipeline drawing.
 */
void
BrainOpenGLVolumeObliqueSliceDrawing::ObliqueSlice::draw(BrainOpenGLFixedPipeline* fixedPipelineDrawing)
{
    Brain* brain = fixedPipelineDrawing->m_brain;
    SelectionItemVoxel* selectionItemVoxel = brain->getSelectionManager()->getVoxelIdentification();
    SelectionItemVoxelEditing* selectionItemVoxelEditing = brain->getSelectionManager()->getVoxelEditingIdentification();

    const GraphicsPrimitive::PrimitiveType primitiveType = GraphicsPrimitive::PrimitiveType::OPENGL_TRIANGLES;
    std::unique_ptr<GraphicsPrimitiveV3fC4ub> primitive(GraphicsPrimitive::newPrimitiveV3fC4ub(primitiveType));
    
    float rowXYZ[3] = {
        m_originXYZ[0],
        m_originXYZ[1],
        m_originXYZ[2]
    };
    
    std::vector<int64_t> selectionIJK;
    for (int32_t iRow = 0; iRow < m_numberOfRows; iRow++) {
        float voxelXYZ[3] = {
            rowXYZ[0],
            rowXYZ[1],
            rowXYZ[2]
        };
        
        const int32_t rowRgbaOffset = (iRow * m_numberOfColumns * 4);
        for (int32_t iCol = 0; iCol < m_numberOfColumns; iCol++) {
            float nextXYZ[3] = {
                voxelXYZ[0] + m_leftToRightStepXYZ[0],
                voxelXYZ[1] + m_leftToRightStepXYZ[1],
                voxelXYZ[2] + m_leftToRightStepXYZ[2],
            };
            
            const int32_t rgbaIndex = rowRgbaOffset + (iCol * 4);
            CaretAssertVectorIndex(m_rgba, rgbaIndex + 3);
            const uint8_t alpha = m_rgba[rgbaIndex +3];
            if (alpha > 0) {
                const float upperLeftXYZ[3] = {
                    voxelXYZ[0] + m_bottomToTopStepXYZ[0],
                    voxelXYZ[1] + m_bottomToTopStepXYZ[1],
                    voxelXYZ[2] + m_bottomToTopStepXYZ[2]
                };
                const float upperRightXYZ[3] = {
                    nextXYZ[0] + m_bottomToTopStepXYZ[0],
                    nextXYZ[1] + m_bottomToTopStepXYZ[1],
                    nextXYZ[2] + m_bottomToTopStepXYZ[2]
                };
                const uint8_t* rgba = &m_rgba[rgbaIndex];
                
                /*
                 * Each voxel is drawn with two triangles because:
                 * (1) Voxels may be sparse and triangles strips do not support 'restart'
                 * until newer versions of OpenGL.
                 * (2) Support for Quads is removed in newer versions of OpenGL.
                 */
                primitive->addVertex(voxelXYZ,
                                     rgba);
                primitive->addVertex(nextXYZ,
                                     rgba);
                primitive->addVertex(upperLeftXYZ,
                                     rgba);
                primitive->addVertex(upperLeftXYZ,
                                     rgba);
                primitive->addVertex(nextXYZ,
                                     rgba);
                primitive->addVertex(upperRightXYZ,
                                     rgba);
                
                if (m_identificationModeFlag) {
                    /*
                     * Each voxel is drawn with two triangles so need to put ID 
                     * in twice since so that number of selections equals number
                     * of triangles
                     */
                    const int32_t idIndex = ((iRow * m_numberOfColumns * 3) + (iCol * 3));
                    CaretAssertVectorIndex(m_identificationIJK, idIndex + 2);
                    selectionIJK.push_back(m_identificationIJK[idIndex]);
                    selectionIJK.push_back(m_identificationIJK[idIndex+1]);
                    selectionIJK.push_back(m_identificationIJK[idIndex+2]);
                    selectionIJK.push_back(m_identificationIJK[idIndex]);
                    selectionIJK.push_back(m_identificationIJK[idIndex+1]);
                    selectionIJK.push_back(m_identificationIJK[idIndex+2]);
                }
            }
            
            voxelXYZ[0] = nextXYZ[0];
            voxelXYZ[1] = nextXYZ[1];
            voxelXYZ[2] = nextXYZ[2];
        }
        
        rowXYZ[0] += m_bottomToTopStepXYZ[0];
        rowXYZ[1] += m_bottomToTopStepXYZ[1];
        rowXYZ[2] += m_bottomToTopStepXYZ[2];
    }
    
    if (primitive->isValid()) {
        if (m_identificationModeFlag) {
            int32_t selectedIndex = -1;
            float selectedDepth  = -1.0;
            GraphicsEngineDataOpenGL::drawWithSelection(primitive.get(),
                                                        fixedPipelineDrawing->mouseX,
                                                        fixedPipelineDrawing->mouseY,
                                                        selectedIndex,
                                                        selectedDepth);
            
            if (selectedIndex >= 0) {
                const int32_t ijkIndex = selectedIndex * 3;
                CaretAssertVectorIndex(selectionIJK, ijkIndex + 2);
                
                m_selectionIJK[0] = selectionIJK[ijkIndex];
                m_selectionIJK[1] = selectionIJK[ijkIndex+1];
                m_selectionIJK[2] = selectionIJK[ijkIndex+2];
                
                if ((m_selectionIJK[0] >= 0)
                    && (m_selectionIJK[1] >= 0)
                    && (m_selectionIJK[2])) {
                    if (selectionItemVoxel->isEnabledForSelection()) {
                        if (selectionItemVoxel->isOtherScreenDepthCloserToViewer(selectedDepth)) {
                            selectionItemVoxel->setVoxelIdentification(brain,
                                                                       m_volumeInterface,
                                                                       m_selectionIJK,
                                                                       selectedDepth);
                            
                            float voxelCoordinates[3];
                            m_volumeInterface->indexToSpace(m_selectionIJK[0], m_selectionIJK[1], m_selectionIJK[2],
                                                            voxelCoordinates[0], voxelCoordinates[1], voxelCoordinates[2]);
                            
                            fixedPipelineDrawing->setSelectedItemScreenXYZ(selectionItemVoxel,
                                                                           voxelCoordinates);
                            CaretLogFinest("Selected Voxel (3D): " + AString::fromNumbers(m_selectionIJK, 3, ","));
                        }
                    }
                    
                    if (selectionItemVoxelEditing->isEnabledForSelection()) {
                        if (selectionItemVoxelEditing->getVolumeFileForEditing() == m_volumeFile) {
                            if (selectionItemVoxelEditing->isOtherScreenDepthCloserToViewer(selectedDepth)) {
                                selectionItemVoxelEditing->setVoxelIdentification(brain,
                                                                                  m_volumeInterface,
                                                                                  m_selectionIJK,
                                                                                  selectedDepth);
                                float floatDiffXYZ[3] = {
                                    m_leftToRightStepXYZ[0] + m_bottomToTopStepXYZ[0],
                                    m_leftToRightStepXYZ[1] + m_bottomToTopStepXYZ[1],
                                    m_leftToRightStepXYZ[2] + m_bottomToTopStepXYZ[2]
                                };
                                selectionItemVoxelEditing->setVoxelDiffXYZ(floatDiffXYZ);
                                
                                float voxelCoordinates[3];
                                m_volumeInterface->indexToSpace(m_selectionIJK[0], m_selectionIJK[1], m_selectionIJK[2],
                                                                voxelCoordinates[0], voxelCoordinates[1], voxelCoordinates[2]);
                                
                                fixedPipelineDrawing->setSelectedItemScreenXYZ(selectionItemVoxelEditing,
                                                                               voxelCoordinates);
                                CaretLogFinest("Selected Voxel Editing (3D): Indices ("
                                               + AString::fromNumbers(m_selectionIJK, 3, ",")
                                               + ") Diff XYZ ("
                                               + AString::fromNumbers(floatDiffXYZ, 3, ",")
                                               + ")");
                            }
                        }
                    }
                }
            }
        }
        else {
            GraphicsEngineDataOpenGL::draw(primitive.get());
        }
    }
}

/**
 * @return True if the output slice indices are valid, else false.
 *
 * @param ijkOut
 *     Output slice indices.
 */
bool
BrainOpenGLVolumeObliqueSliceDrawing::ObliqueSlice::getSelectionIJK(int32_t ijkOut[3]) const
{
    ijkOut[0] = m_selectionIJK[0];
    ijkOut[1] = m_selectionIJK[1];
    ijkOut[2] = m_selectionIJK[2];
    return (m_selectionIJK[0] >= 0);
}

/**
 * @return True if the given slice spatially matches (voxel row/columns and coordinates).
 *
 * @param slice
 *     Slice for comparison.
 */
bool
BrainOpenGLVolumeObliqueSliceDrawing::ObliqueSlice::spatialMatch(const ObliqueSlice* slice)
{
    CaretAssert(slice);
    
    const float diffMax = 0.001f;
    if ((m_numberOfRows == slice->m_numberOfRows)
        && (m_numberOfColumns == slice->m_numberOfColumns)
        && (MathFunctions::distanceSquared3D(m_originXYZ, slice->m_originXYZ) < diffMax)
        && (MathFunctions::distanceSquared3D(m_leftToRightStepXYZ, slice->m_leftToRightStepXYZ) < diffMax)
        && (MathFunctions::distanceSquared3D(m_bottomToTopStepXYZ, slice->m_bottomToTopStepXYZ) < diffMax)) {
        return true;
    }
    
    return false;
}

/**
 * Composite the given slices onto this slice
 *
 * @param slices
 *     Slices that are composited.
 * @return
 *     True if able to composite all slices, else false.
 */
bool
BrainOpenGLVolumeObliqueSliceDrawing::ObliqueSlice::compositeSlicesRGBA(const std::vector<ObliqueSlice*>& slices)
{
    std::vector<ObliqueSlice*> matchedSlices;
    for (auto s : slices) {
        if (s != this) {
            if (spatialMatch(s)) {
                if (m_rgba.size() == s->m_rgba.size()) {
                    matchedSlices.push_back(s);
                }
                else {
                    return false;
                }
            }
        }
    }
    
    if (matchedSlices.empty()) {
        return false;
    }
    
    const int32_t numSlices = static_cast<int32_t>(matchedSlices.size());
    const int32_t rgbaLength = static_cast<int32_t>(m_rgba.size() / 4);
    
    for (int32_t i = 0; i < rgbaLength; i++) {
        const int32_t i4 = i * 4;
        for (int32_t iSlice = 0; iSlice < numSlices; iSlice++) {
            CaretAssertVectorIndex(matchedSlices, iSlice);
            const uint8_t* rgbaTop = &matchedSlices[iSlice]->m_rgba[i4];
            uint8_t alphaTop = rgbaTop[3];
            if (matchedSlices[iSlice]->m_opacity < 1.0f) {
                float sliceAlphaFloat = ((static_cast<float>(alphaTop) / 255.0f) * matchedSlices[iSlice]->m_opacity);
                if (sliceAlphaFloat >= 1.0f) {
                    alphaTop = 255;
                }
                else if (sliceAlphaFloat <= 0.0f) {
                    alphaTop = 0;
                }
                else {
                    alphaTop = static_cast<uint8_t>(sliceAlphaFloat * 255);
                }
            }
            if (alphaTop > 0) {
                uint8_t* rgbaBottom = &m_rgba[i4];
                if ((alphaTop == 255) ||
                    (rgbaBottom[3] == 0)) {
                    rgbaBottom[0] = rgbaTop[0];
                    rgbaBottom[1] = rgbaTop[1];
                    rgbaBottom[2] = rgbaTop[2];
                    rgbaBottom[3] = 255;
                }
                else if (alphaTop > 0) {
                    const float alpha = static_cast<float>(alphaTop) / 255.0f;
                    const float oneMinusAlpha = 1.0f - alpha;
                    for (int32_t m = 0; m < 3; m++) {
                        const int32_t top    = static_cast<int32_t>(static_cast<float>(rgbaTop[m]) * alpha);
                        const int32_t bottom = static_cast<int32_t>(static_cast<float>(rgbaBottom[m]) * oneMinusAlpha);
                        int32_t blended = (top + bottom);
                        if (blended > 255)    blended = 255;
                        else if (blended < 0) blended = 0;
                        
                        rgbaBottom[m] = static_cast<uint8_t>(blended);
                    }
                    rgbaBottom[3] = 255;
                }
            }
        }
    }
    
    return true;
}




