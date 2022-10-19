
/*LICENSE_START*/
/*
 *  Copyright (C) 2020 Washington University School of Medicine
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

#define __BRAIN_OPEN_G_L_HISTOLOGY_SLICE_DRAWING_DECLARE__
#include "BrainOpenGLHistologySliceDrawing.h"
#undef __BRAIN_OPEN_G_L_HISTOLOGY_SLICE_DRAWING_DECLARE__

#include <cmath>

#include "Brain.h"
#include "BrainOpenGLFixedPipeline.h"
#include "BrainOpenGLIdentificationDrawing.h"
#include "BrainOpenGLViewportContent.h"
#include "BrowserTabContent.h"
#include "CaretAssert.h"
#include "CaretColorEnum.h"
#include "CaretLogger.h"
#include "CziImage.h"
#include "CziImageFile.h"
#include "DisplayPropertiesCziImages.h"
#include "EventManager.h"
#include "EventOpenGLObjectToWindowTransform.h"
#include "GraphicsEngineDataOpenGL.h"
#include "GraphicsObjectToWindowTransform.h"
#include "GraphicsRegionSelectionBox.h"
#include "GraphicsPrimitiveV3f.h"
#include "GraphicsPrimitiveV3fC4f.h"
#include "GraphicsPrimitiveV3fT2f.h"
#include "HistologyCoordinate.h"
#include "HistologySlice.h"
#include "HistologySlicesFile.h"
#include "ImageFile.h"
#include "ModelHistology.h"
#include "HistologyOverlaySet.h"
#include "SelectionItemHistologyCoordinate.h"
#include "SelectionItemAnnotation.h"
#include "SelectionManager.h"

using namespace caret;

const bool debugFlag(false);
    
/**
 * \class caret::BrainOpenGLHistologySliceDrawing
 * \brief Draw histology slice data
 * \ingroup Brain
 */

/**
 * Constructor.
 */
BrainOpenGLHistologySliceDrawing::BrainOpenGLHistologySliceDrawing()
: CaretObject()
{
    
}

/**
 * Destructor.
 */
BrainOpenGLHistologySliceDrawing::~BrainOpenGLHistologySliceDrawing()
{
}

/**
 * Setup the orthographic bounds for the underlay histology file
 * @param orthoLeftOut
 *    Output with orthographic left
 * @param orthoRightOut
 *    Output with orthographic right
 * @param orthoBottomOut
 *    Output with orthographic bottom
 * @param orthoTopOut
 *    Output with orthographic top
 * @return True if othographic bounds are valid, else false.
 */
bool
BrainOpenGLHistologySliceDrawing::getOrthoBounds(double& orthoLeftOut,
                                                 double& orthoRightOut,
                                                 double& orthoBottomOut,
                                                 double& orthoTopOut)
{
    orthoLeftOut   = -1.0;
    orthoRightOut  =  1.0;
    orthoBottomOut = -1.0;
    orthoTopOut    =  1.0;
        
    BoundingBox boundingBox;
    
    bool firstFlag(true);
    const int32_t numberOfFiles(m_mediaFilesAndDataToDraw.size());
    for (int32_t i = 0; i < numberOfFiles; i++) {
        CaretAssertVectorIndex(m_mediaFilesAndDataToDraw, i);
        HistologySlicesFile* histologyFile(m_mediaFilesAndDataToDraw[i].m_selectedFile);
        CaretAssert(histologyFile);
        const MediaFile* mediaFile(m_mediaFilesAndDataToDraw[i].m_mediaFile);
        CaretAssert(mediaFile);
        if (mediaFile->isPlaneXyzSupported()) {
            const BoundingBox bb(mediaFile->getPlaneXyzBoundingBox());
            if (firstFlag) {
                firstFlag   = false;
                boundingBox = bb;
            }
            else {
                boundingBox.unionOperation(bb);
            }
        }
    }
    
    const double viewportWidth(m_viewport[2]);
    const double viewportHeight(m_viewport[3]);
    const double viewportAspectRatio = (viewportHeight
                                        / viewportWidth);

    const double imageWidth(boundingBox.getDifferenceX());
    const double imageHeight(boundingBox.getDifferenceY());
    if ((imageWidth < 1.0)
        || (imageHeight < 1.0)) {
        return false;
    }
    const double imageAspectRatio = (imageHeight
                                     / imageWidth);
    
    const bool originTopLeftFlag(true);
    
    const double marginPercent(0.02);
    const double marginSizePixels = imageHeight * marginPercent;
    if (imageAspectRatio > viewportAspectRatio) {
        orthoBottomOut = boundingBox.getMinY() - marginSizePixels;
        orthoTopOut    = boundingBox.getMaxY() + marginSizePixels;
        if (originTopLeftFlag) {
            std::swap(orthoBottomOut, orthoTopOut);
        }
        const double orthoHeight(std::fabs(orthoTopOut - orthoBottomOut));
        const double orthoWidth(orthoHeight / viewportAspectRatio);
        const double halfOrthoWidth(orthoWidth / 2.0);
        const double centerX(boundingBox.getCenterX());
        orthoLeftOut  = centerX - halfOrthoWidth;
        orthoRightOut = centerX + halfOrthoWidth;
    }
    else {
        orthoLeftOut  = boundingBox.getMinX() - marginSizePixels;
        orthoRightOut = boundingBox.getMaxX() + marginSizePixels;
        const double orthoWidth(orthoRightOut - orthoLeftOut);
        const double orthoHeight(orthoWidth * viewportAspectRatio);
        const double halfOrthoHeight(orthoHeight / 2.0);
        const double centerY(boundingBox.getCenterY());
        orthoBottomOut = centerY - halfOrthoHeight;
        orthoTopOut    = centerY + halfOrthoHeight;
        if (originTopLeftFlag) {
            std::swap(orthoBottomOut, orthoTopOut);
        }
    }

    CaretAssert(orthoRightOut > orthoLeftOut);
    if (originTopLeftFlag) {
        CaretAssert(orthoBottomOut > orthoTopOut);
    }
    else {
        CaretAssert(orthoTopOut > orthoBottomOut);
    }
    
    return true;
}

/**
 * @param fixedPipelineDrawing
 *    The fixed pipeline drawing
 * @param viewportContent
 *    The viewport content
 * @param browserTabContent
 *    Content of the browser tab
 * @param histologyModel
 *    Histology model for drawing
 * @param viewport
 *    Size of the viewport
 */
void
BrainOpenGLHistologySliceDrawing::draw(BrainOpenGLFixedPipeline* fixedPipelineDrawing,
                                       const BrainOpenGLViewportContent* viewportContent,
                                       BrowserTabContent* browserTabContent,
                                       ModelHistology* histologyModel,
                                       const std::array<int32_t, 4>& viewport)
{
    CaretAssert(fixedPipelineDrawing);
    CaretAssert(browserTabContent);
    CaretAssert(histologyModel);
    
    m_fixedPipelineDrawing = fixedPipelineDrawing;
    m_browserTabContent    = browserTabContent;
    m_viewport             = viewport;
    m_mediaFilesAndDataToDraw.clear();
    
    HistologyOverlaySet* overlaySet = histologyModel->getHistologyOverlaySet(m_browserTabContent->getTabNumber());
    CaretAssert(overlaySet);
    const int32_t numberOfOverlays = overlaySet->getNumberOfDisplayedOverlays();
    
    /*
     * Find overlays containing files that support coordinates
     */
    bool firstFlag(true);
    Vector3D underlayStereotaxicXYZ;
    for (int32_t iOverlay = (numberOfOverlays - 1); iOverlay >= 0; iOverlay--) {
        HistologyOverlay* overlay = overlaySet->getOverlay(iOverlay);
        CaretAssert(overlay);
        
        if (overlay->isEnabled()) {
            const HistologyOverlay::SelectionData selectionData(overlay->getSelectionData());
            const HistologySlicesFile* selectedFile(selectionData.m_selectedFile);
            if (selectedFile != NULL) {
                const HistologyCoordinate histologyCoordinate(browserTabContent->getHistologySelectedCoordinate(selectionData.m_selectedFile));
                int32_t selectedSliceIndex(histologyCoordinate.getSliceIndex());
                if (firstFlag) {
                    if (histologyCoordinate.isStereotaxicXYZValid()) {
                        underlayStereotaxicXYZ = histologyCoordinate.getStereotaxicXYZ();
                        firstFlag = false;
                    }
                }
                else {
                    /*
                     * Since this is not the underlay, need to find slice in this file
                     * closest to the underlay's slice
                     */
                    HistologyCoordinate hc(HistologyCoordinate::newInstanceStereotaxicXYZ(const_cast<HistologySlicesFile*>(selectedFile),
                                                                                          underlayStereotaxicXYZ));
                    if (hc.isValid()
                        && hc.isSliceIndexValid()) {
                        selectedSliceIndex = hc.getSliceIndex();
                    }
                    else {
                        CaretLogSevere("Slice index invalid for histology overlay with file: "
                                       + selectedFile->getFileName());
                        continue;
                    }
                }
                std::vector<HistologyOverlay::DrawingData> drawingData(overlay->getDrawingData(selectedSliceIndex));
                m_mediaFilesAndDataToDraw.insert(m_mediaFilesAndDataToDraw.end(),
                                                 drawingData.begin(),
                                                 drawingData.end());
            }
        }
    }
    
    if (m_mediaFilesAndDataToDraw.empty()) {
        return;
    }
    
    double orthoLeft(-1.0);
    double orthoRight(1.0);
    double orthoBottom(-1.0);
    double orthoTop(1.0);
    if ( ! getOrthoBounds(orthoLeft, orthoRight, orthoBottom, orthoTop)) {
        return;
    }
    if (debugFlag) {
        std::cout << "Ortho L=" << orthoLeft << ", R=" << orthoRight
        << ", B=" << orthoBottom << ", T=" << orthoTop << std::endl;
    }

    if ((m_viewport[2] < 1)
        || (m_viewport[3] < 1)) {
        return;
    }
    glViewport(m_viewport[0],
               m_viewport[1],
               m_viewport[2],
               m_viewport[3]);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(orthoLeft, orthoRight,
            orthoBottom, orthoTop,
            -1.0, 1.0);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    /*
     * Note on translation: Origin is at top left so a positive-Y translation
     * moves the image down.  However, the user may edit the translation values
     * and from the user's perspective, positive-Y is up.  Also, other models
     * types have postive-Y up (surface, volume).  So we use the negative
     * of the Y-translation.
     */
    float translation[3];
    m_browserTabContent->getTranslation(translation);
    glTranslatef(translation[0], -translation[1], 0.0);
    
    const float scaling = m_browserTabContent->getScaling();
    glScalef(scaling, scaling, 1.0);

    std::array<float, 4> orthoLRBT {
        static_cast<float>(orthoLeft),
        static_cast<float>(orthoRight),
        static_cast<float>(orthoBottom),
        static_cast<float>(orthoTop)
    };
    GraphicsObjectToWindowTransform* transform = new GraphicsObjectToWindowTransform();
    fixedPipelineDrawing->loadObjectToWindowTransform(transform, orthoLRBT, 0.0, true);
    viewportContent->setHistologyGraphicsObjectToWindowTransform(transform);
    
    const float orthoHeight(std::fabs(orthoBottom - orthoTop));
    drawModelLayers(orthoLRBT,
                    viewportContent,
                    transform,
                    browserTabContent->getTabNumber(),
                    orthoHeight,
                    viewport[3]);
    
    drawSelectionBox();
    
    {
        glPushMatrix();
        glLoadIdentity();
        
        /*
         * Draw small yellow crosshairs across center of viewport
         */
        const float xCenter((orthoLeft + orthoRight) / 2.0);
        const float yCenter((orthoBottom + orthoTop) / 2.0);
        const float length(300);
        glColor3f(1.0, 1.0, 0.0);
        glLineWidth(1.0);
        glBegin(GL_LINES);
        glVertex2f(xCenter - length, yCenter);
        glVertex2f(xCenter + length, yCenter);
        glVertex2f(xCenter, yCenter - length);
        glVertex2f(xCenter, yCenter + length);
        glEnd();
        
        glPopMatrix();
    }
    
}

/**
 * Draw the models layers
 * @param orthoLRBT
 *    Orthographic projection
 * @param viewportContent
 *    The viewport content
 * @param objectToWindowTransform
 *   Transforms point from object to window space
 * @param tabIndex
 *   Index of the tab
 * @param viewportHeight
 *   Height of viewport
 */
void
BrainOpenGLHistologySliceDrawing::drawModelLayers(const std::array<float, 4>& orthoLRBT,
                                                  const BrainOpenGLViewportContent* viewportContent,
                                                  const GraphicsObjectToWindowTransform* transform,
                                                  const int32_t /*tabIndex*/,
                                                  const float /*orthoHeight*/,
                                                  const float /*viewportHeight*/)
{
    SelectionItemHistologyCoordinate* idHistology = m_fixedPipelineDrawing->m_brain->getSelectionManager()->getHistologyPlaneCoordinateIdentification();
    SelectionItemAnnotation* annotationID = m_fixedPipelineDrawing->m_brain->getSelectionManager()->getAnnotationIdentification();

    /*
     * Check for a 'selection' type mode
     */
    bool selectImageFlag(false);
    switch (m_fixedPipelineDrawing->mode) {
        case BrainOpenGLFixedPipeline::MODE_DRAWING:
            break;
        case BrainOpenGLFixedPipeline::MODE_IDENTIFICATION:
            if (idHistology->isEnabledForSelection()) {
                selectImageFlag = true;
            }
            else if (annotationID->isEnabledForSelection()) {
                /* continue drawing so annotations get selected */
            }
            else {
                return;
            }
            break;
        case BrainOpenGLFixedPipeline::MODE_PROJECTION:
            return;
            break;
    }
    
    glPushMatrix();
    
    HistologySlice*      underlayHistologySlice(NULL);
    HistologySlicesFile* underlayHistologySlicesFile(NULL);
    int32_t              underlayHistologySliceNumber(-1);
    
    const int32_t numMediaFiles(static_cast<int32_t>(m_mediaFilesAndDataToDraw.size()));
    for (int32_t i = 0; i < numMediaFiles; i++) {
        CaretAssertVectorIndex(m_mediaFilesAndDataToDraw, i);
        HistologyOverlay::DrawingData& drawingData = m_mediaFilesAndDataToDraw[i];
        MediaFile* mediaFile(drawingData.m_mediaFile);
        CaretAssert(mediaFile);
        CziImageFile* cziImageFile(mediaFile->castToCziImageFile());
        ImageFile* imageFile(mediaFile->castToImageFile());

        if (cziImageFile != NULL) {
            const int32_t frameIndex(0);
            const bool allFramesSelectedFlag(true);
            const int32_t manualPyramidLayerIndex(0);
            cziImageFile->updateImageForDrawingInTab(drawingData.m_tabIndex,
                                                     drawingData.m_overlayIndex,
                                                     frameIndex,
                                                     allFramesSelectedFlag,
                                                     CziImageResolutionChangeModeEnum::AUTO2,
                                                     MediaDisplayCoordinateModeEnum::PLANE,
                                                     manualPyramidLayerIndex,
                                                     transform);
        }
        else if (imageFile != NULL) {
            /* nothing */
        }
        else {
            CaretAssertMessage(0, ("Unrecognized file type "
                                   + DataFileTypeEnum::toName(mediaFile->getDataFileType())
                                   + " for media drawing."));
        }


        GraphicsPrimitiveV3fT2f* primitive(mediaFile->getGraphicsPrimitiveForPlaneXyzDrawing(drawingData.m_tabIndex,
                                                                                             drawingData.m_overlayIndex));
        if (primitive == NULL) {
            /*
             * If a CZI image is completely offscreen there may be no data to load for it
             */
            continue;
        }

        glPushMatrix();
        
        CaretAssert(primitive->isValid());

        glPushAttrib(GL_COLOR_BUFFER_BIT);
        if ( ! selectImageFlag) {
            /*
             * Allow blending.  Images may have a border color with alpha of zero
             * so that background shows through.
             */
            BrainOpenGLFixedPipeline::setupBlending(BrainOpenGLFixedPipeline::BlendDataType::FEATURE_IMAGE);
        }
        
        /*
         * Set texture filtering
         */
        primitive->setTextureMinificationFilter(s_textureMinificationFilter);
        primitive->setTextureMagnificationFilter(s_textureMagnificationFilter);
        
        GraphicsEngineDataOpenGL::draw(primitive);
        glPopAttrib();

        if (selectImageFlag) {
            processSelection(m_browserTabContent->getTabNumber(),
                             drawingData,
                             primitive);
        }
        
        glPopMatrix();
        
        if (underlayHistologySlicesFile == NULL) {
            underlayHistologySlicesFile  = drawingData.m_selectedFile;
            underlayHistologySlice       = drawingData.m_selectedFile->getHistologySliceByIndex(drawingData.m_selectedSliceIndex);
            underlayHistologySliceNumber = drawingData.m_selectedSliceNumber;
        }
    }
    
    glPopMatrix();
    
    /*
     * Height used for drawing ID symbols
     */
    float planeRangeY(1.0);
    if (underlayHistologySlicesFile != NULL) {
        const BoundingBox bb(underlayHistologySlicesFile->getPlaneXyzBoundingBox());
        planeRangeY = bb.getDifferenceY();
    }

    /*
     * Draw identification symbols
     */
    BrainOpenGLIdentificationDrawing idDrawing(m_fixedPipelineDrawing,
                                               m_fixedPipelineDrawing->m_brain,
                                               m_browserTabContent,
                                               m_fixedPipelineDrawing->mode);
    const float mediaThickness(2.0f);
    Plane plane;
    idDrawing.drawHistologyFilePlaneCoordinateIdentificationSymbols(underlayHistologySlicesFile,
                                                                    underlayHistologySliceNumber,
                                                                    plane,
                                                                    mediaThickness,
                                                                    m_browserTabContent->getScaling(),
                                                                    planeRangeY);


    /*
     * Draw the crosshairs
     */
    const BrowserTabContent* btc(viewportContent->getBrowserTabContent());
    CaretAssert(btc);
    drawCrosshairs(orthoLRBT,
                   btc->getHistologySelectedCoordinate(underlayHistologySlicesFile));
    
    /*
     * Draw annotation in histology space
     */
    if (underlayHistologySlice != NULL) {
        const float sliceSpacing(underlayHistologySlicesFile->getSliceSpacing());

        HistologySpaceKey histologySpaceKey(underlayHistologySlicesFile->getFileName(),
                                            underlayHistologySliceNumber);
        m_fixedPipelineDrawing->drawHistologySpaceAnnotations(viewportContent,
                                                              histologySpaceKey,
                                                              underlayHistologySlice,
                                                              sliceSpacing);
    }
    
//    /*
//     * Draw annotations in stereotaxic space
//     */
//    EventOpenGLObjectToWindowTransform xform(EventOpenGLObjectToWindowTransform::SpaceType::MODEL);
//    EventManager::get()->sendEvent(xform.getPointer());
//    if (xform.isValid()) {
//        std::array<int32_t, 4> viewport(xform.getViewport());
//
//    glPushMatrix();
//    glMatrixMode(GL_PROJECTION);
//    glPushMatrix();
//
//    glMatrixMode(GL_MODELVIEW);
//
//    glPopMatrix();
//
//    glMatrixMode(GL_PROJECTION);
//    glPopMatrix();
//    glMatrixMode(GL_MODELVIEW);
//
//    glLoadIdentity();
//    }
}

/**
 * Draw the selection box
 */
void
BrainOpenGLHistologySliceDrawing::drawSelectionBox()
{
    const GraphicsRegionSelectionBox* selectionBox = m_browserTabContent->getMediaRegionSelectionBox();

    switch (selectionBox->getStatus()) {
        case GraphicsRegionSelectionBox::Status::INVALID:
            break;
        case GraphicsRegionSelectionBox::Status::VALID:
        {
            float minX, maxX, minY, maxY;
            if (selectionBox->getBounds(minX, minY, maxX, maxY)) {
                std::unique_ptr<GraphicsPrimitiveV3f> primitive(GraphicsPrimitive::newPrimitiveV3f(GraphicsPrimitive::PrimitiveType::POLYGONAL_LINE_LOOP_BEVEL_JOIN,
                                                                                                   m_fixedPipelineDrawing->m_foregroundColorFloat));

                const float z(0.0f);
                primitive->addVertex(minX, minY, z);
                primitive->addVertex(maxX, minY, z);
                primitive->addVertex(maxX, maxY, z);
                primitive->addVertex(minX, maxY, z);
                
                const float lineWidthPercentage(0.5);
                primitive->setLineWidth(GraphicsPrimitive::LineWidthType::PERCENTAGE_VIEWPORT_HEIGHT,
                                        lineWidthPercentage);
                
                GraphicsEngineDataOpenGL::draw(primitive.get());
            }
        }
            break;
    }
}

/**
 * Process selection in amedia  file
 * @param tabIndex
 *   Index of the tab
 * @param drawingData
 *   Info about selections in the layer
 * @param primitive
 *    Primitive that draws image file
 */
void
BrainOpenGLHistologySliceDrawing::processSelection(const int32_t tabIndex,
                                                   const HistologyOverlay::DrawingData& drawingData,
                                                   GraphicsPrimitiveV3fT2f* primitive)
{
    SelectionItemHistologyCoordinate* idHistology = m_fixedPipelineDrawing->m_brain->getSelectionManager()->getHistologyPlaneCoordinateIdentification();
    EventOpenGLObjectToWindowTransform xform(EventOpenGLObjectToWindowTransform::SpaceType::MODEL);
    EventManager::get()->sendEvent(xform.getPointer());
    if (xform.isValid()) {
        BoundingBox bounds;
        primitive->getVertexBounds(bounds);
        
        const float drawnImageWidth(bounds.getDifferenceX());
        const float drawnImageHeight(bounds.getDifferenceY());
        if ((drawnImageWidth > 0.0)
            && (drawnImageHeight > 0.0)) {
            
            const float mouseX(this->m_fixedPipelineDrawing->mouseX);
            const float mouseY(this->m_fixedPipelineDrawing->mouseY);
            
            float windowXYZ[3] { mouseX, mouseY, 0.0 };
            Vector3D planeXYZ;
            xform.inverseTransformPoint(windowXYZ, planeXYZ);
            
            MediaFile* mediaFile(drawingData.m_mediaFile);
            CaretAssert(mediaFile);

            PixelLogicalIndex pixelLogicalIndex;
            if ( ! mediaFile->planeXyzToLogicalPixelIndex(planeXYZ,
                                                          pixelLogicalIndex)) {
                return;
            }
            
            /*
             * Frame indices to test
             */
            std::vector<int32_t> validFrameIndices;
            for (int32_t i = 0; i < mediaFile->getNumberOfFrames(); i++) {
                validFrameIndices.push_back(i);
            }

            /*
             * Ensure that the pixel is within a displayed frame (Scene
             * in CZI terminology).  Otherwise empty space pixels may
             * be identified.  Also, if RGBA is NOT valid, then
             * there is no CZI data for the pixel.
             */
            bool validPixelFlag(false);
            for (int32_t frameIndex : validFrameIndices) {
                if (mediaFile->isPixelIndexInFrameValid(frameIndex,
                                                        pixelLogicalIndex)) {
                    uint8_t rgba[4];
                    validPixelFlag = mediaFile->getPixelRGBA(tabIndex,
                                                             frameIndex,
                                                             pixelLogicalIndex,
                                                             rgba);
                    if (validPixelFlag) {
                        break;
                    }
                }
            }
            
            /*
             * Multiple layers/images may be drawn.
             * Always give highest priority to pixel that is within an image.
             */
            bool replaceFlag(false);
            if (validPixelFlag) {
                replaceFlag = true;
            }
            else if ( ! idHistology->isPixelRGBAValid()) {
                replaceFlag = true;
            }
            if (replaceFlag) {
                HistologyCoordinate hc(HistologyCoordinate::newInstancePlaneXYZIdentification(drawingData.m_selectedFile,
                                                                                      drawingData.m_mediaFile,
                                                                                      drawingData.m_selectedSliceIndex,
                                                                                      planeXYZ));
                idHistology->setHistologySlicesFile(drawingData.m_selectedFile);
                idHistology->setCoordinate(hc);
                idHistology->setModelXYZ(hc.getPlaneXYZ());
                idHistology->setTabIndex(tabIndex);
                idHistology->setOverlayIndex(drawingData.m_overlayIndex);
                uint8_t pixelByteRGBA[4] = { 0, 0, 0, 0 };
                idHistology->setScreenXYZ(windowXYZ);
                idHistology->setScreenDepth(0.0);
                if (validPixelFlag) {
                    if (mediaFile->getPixelRGBA(tabIndex,
                                                drawingData.m_overlayIndex,
                                                pixelLogicalIndex,
                                                pixelByteRGBA)) {
                        idHistology->setPixelRGBA(pixelByteRGBA);
                    }
                }
            }
        }
    }
}

/**
 * Draw the histology coordinate
 * @param orthoLRBT;
 *    Orthographic projection
 * @param histologyCoordinate
 *    The histology coordinate
 */
void
BrainOpenGLHistologySliceDrawing::drawCrosshairs(const std::array<float, 4>& orthoLRBT,
                                                 const HistologyCoordinate& histologyCoordinate)
{
    glPushAttrib(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    

    
    const float minX(orthoLRBT[0]);
    const float maxX(orthoLRBT[1]);
    const float maxY(orthoLRBT[2]);
    const float minY(orthoLRBT[3]);
    
    const float* red(CaretColorEnum::toRGBA(CaretColorEnum::RED));
    const float* green(CaretColorEnum::toRGBA(CaretColorEnum::GREEN));
    
    const Vector3D centerXYZ(histologyCoordinate.getPlaneXYZ());
    const float z(0.0);
    if (debugFlag) {
        std::cout << "Selected histology plane: " << AString::fromNumbers(histologyCoordinate.getPlaneXYZ()) << std::endl;
        std::cout << "             stereotaxic: " << AString::fromNumbers(histologyCoordinate.getStereotaxicXYZ()) << std::endl;
    }
    
    std::unique_ptr<GraphicsPrimitiveV3fC4f> primitive(GraphicsPrimitive::newPrimitiveV3fC4f(GraphicsPrimitive::PrimitiveType::POLYGONAL_LINES));
    primitive->addVertex(minX, centerXYZ[1], z, green);
    primitive->addVertex(maxX, centerXYZ[1], z, green);
    primitive->addVertex(centerXYZ[0], minY, z, red);
    primitive->addVertex(centerXYZ[0], maxY, z, red);
    
    const float lineWidthPercentage(0.5);
    primitive->setLineWidth(GraphicsPrimitive::LineWidthType::PERCENTAGE_VIEWPORT_HEIGHT,
                            lineWidthPercentage);
    
    GraphicsEngineDataOpenGL::draw(primitive.get());
    
    glPopAttrib();
}

/**
 * Get a description of this object's content.
 * @return String describing this object's content.
 */
AString 
BrainOpenGLHistologySliceDrawing::toString() const
{
    return "BrainOpenGLHistologySliceDrawing";
}

