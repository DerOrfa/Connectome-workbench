#ifndef __CZI_IMAGE_FILE_H__
#define __CZI_IMAGE_FILE_H__

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


#include <array>
#include <memory>

#include <QRectF>

#include "BrainConstants.h"
#include "CziImage.h"
#include "CziImageResolutionChangeModeEnum.h"
#include "EventListenerInterface.h"
#include "SingleChannelPyramidLevelTileAccessor.h"
#include "MediaFile.h"
#include "PixelIndex.h"
#include "Plane.h"
#include "VolumeFile.h"

class QImage;

/*
 * Unable to use: "using namespace libCZI;"
 * Symbols in libCZI namespace clash with Windows symbols.
 * C:\Program Files (x86)\Windows Kits\8.1\include\\um\objidlbase.h(585): error C2872: 'IStream': ambiguous symbol
 * C:\Program Files (x86)\Windows Kits\8.1\include\\um\objidlbase.h(143): note: could be 'libCZI::IStream IStream'
 * C:\Program Files (x86)\Windows Kits\8.1\include\\um\objidlbase.h(143): note: or       'libCZI::IStream'
 */

namespace caret {

    class CziImage;
    class CziImageLoaderAllFrames;
    class GraphicsObjectToWindowTransform;
    class Matrix4x4;
    class RectangleTransform;
    class VolumeSpace;
    
    class CziImageFile : public MediaFile, public EventListenerInterface {
        
    public:        
        CziImageFile();
        
        virtual ~CziImageFile();
        
        CziImageFile(const CziImageFile&) = delete;

        CziImageFile& operator=(const CziImageFile&) = delete;
        
        virtual CziImageFile* castToCziImageFile() override;
        
        virtual const CziImageFile* castToCziImageFile() const override;

        bool isEmpty() const override;
        
        virtual void clearModified() override;
        
        virtual bool isModified() const override;
        
        GiftiMetaData* getFileMetaData() override;
        
        const GiftiMetaData* getFileMetaData() const override;
        
        virtual int32_t getWidth() const override;
        
        virtual int32_t getHeight() const override;

        virtual int32_t getNumberOfFrames() const override;
        
        virtual bool isPixelIndexValid(const int32_t tabIndex, 
                                       const int32_t overlayIndex,
                                       const PixelIndex& pixelIndexOriginAtTopLeft) const override;
        
        virtual bool isPixelIndexValid(const int32_t tabIndex,
                                       const int32_t overlayIndex,
                                       const PixelLogicalIndex& pixelLogicalIndex) const override;
        
        virtual bool isPixelIndexValid(const PixelLogicalIndex& pixelLogicalIndex) const;
        
        virtual void getPixelIdentificationText(const int32_t tabIndex,
                                                const int32_t overlayIndex,
                                                const PixelLogicalIndex& pixelLogicalIndex,
                                                std::vector<AString>& columnOneTextOut,
                                                std::vector<AString>& columnTwoTextOut,
                                                std::vector<AString>& toolTipTextOut) const override;
        
        virtual void addToDataFileContentInformation(DataFileContentInformation& dataFileInformation) override;
        
        virtual void readFile(const AString& filename) override;
        
        virtual void writeFile(const AString& filename) override;

        virtual bool supportsWriting() const override;
        
        int32_t getNumberOfScenes() const;
        
        const CziImage* getImageForDrawingInTab(const int32_t tabIndex,
                                                const int32_t overlayIndex,
                                                const int32_t frameIndex,
                                                const bool allFramesFlag,
                                                const CziImageResolutionChangeModeEnum::Enum resolutionChangeMode,
                                                const int32_t pyramidLayerIndex,
                                                const GraphicsObjectToWindowTransform* transform);
        
        virtual GraphicsPrimitiveV3fT2f* getGraphicsPrimitiveForMediaDrawing(const int32_t tabIndex,
                                                                             const int32_t overlayIndex) const override;

        QRectF getFullResolutionLogicalRect() const;
        
        virtual bool getPixelRGBA(const int32_t tabIndex,
                                  const int32_t overlayIndex,
                                  const PixelLogicalIndex& pixelLogicalIndex,
                                  uint8_t pixelRGBAOut[4]) const override;
        
        void getPyramidLayerRange(int32_t& lowestResolutionPyramidLayerIndexOut,
                                  int32_t& highestResolutionPyramidLayerIndexOut) const;
        
        int32_t getPyramidLayerIndexForTabOverlay(const int32_t tabIndex,
                                                  const int32_t overlayIndex) const;
        
        void setPyramidLayerIndexForTab(const int32_t tabIndex,
                                   const int32_t pyramidLayerIndex);
        
        void reloadPyramidLayerInTab(const int32_t tabIndex);
        
        virtual void receiveEvent(Event* event) override;
        
        virtual bool pixelIndexToStereotaxicXYZ(const PixelLogicalIndex& pixelLogicalIndex,
                                                const bool includeNonlinearFlag,
                                                std::array<float, 3>& xyzOut) const override;
        
        virtual bool stereotaxicXyzToPixelIndex(const std::array<float, 3>& xyz,
                                                const bool includeNonlinearFlag,
                                                PixelLogicalIndex& pixelLogicalIndexOut) const override;
        
        virtual bool findPixelNearestStereotaxicXYZ(const std::array<float, 3>& xyz,
                                                    const bool includeNonLinearFlag,
                                                    float& signedDistanceToPixelMillimetersOut,
                                                    PixelLogicalIndex& pixelLogicalIndexOut) const override;
        
        void testPixelTransforms(const int32_t pixelIndexStep,
                                 const bool nonLinearFlag,
                                 const bool verboseFlag,
                                 AString& resultsMessageOut,
                                 QImage& imageOut) const;
        
        virtual PixelIndex pixelLogicalIndexToPixelIndex(const PixelLogicalIndex& pixelLogicalIndex) const override;
        
        virtual PixelLogicalIndex pixelIndexToPixelLogicalIndex(const PixelIndex& pixelIndex) const override;

        // ADD_NEW_METHODS_HERE

          
          
    protected: 
        virtual void saveFileDataToScene(const SceneAttributes* sceneAttributes,
                                             SceneClass* sceneClass);

        virtual void restoreFileDataFromScene(const SceneAttributes* sceneAttributes,
                                                  const SceneClass* sceneClass);

    private:
        enum class Status {
            CLOSED,
            OPEN,
            ERRORED  /* Note: "ERROR" fails to compile on Windows */
        };

        class PyramidLayer {
        public:
            PyramidLayer(const int32_t sceneIndex,
                         const libCZI::ISingleChannelPyramidLayerTileAccessor::PyramidLayerInfo layerInfo,
                         const int64_t width,
                         const int64_t height)
            : m_sceneIndex(sceneIndex),
            m_layerInfo(layerInfo),
            m_width(width),
            m_height(height) { }
            
            int32_t m_sceneIndex;
            
            libCZI::ISingleChannelPyramidLayerTileAccessor::PyramidLayerInfo m_layerInfo;
            
            int64_t m_width = 0;
            
            int64_t m_height = 0;
            
            float m_zoomLevelFromLowestResolutionImage = 1.0;
        };
        
        class CziSceneInfo {
        public:
            CziSceneInfo(const int32_t sceneIndex,
                         const QRectF& logicalRectange)
            : m_sceneIndex(sceneIndex),
            m_logicalRectangle(logicalRectange) {
                
            }
            
            void addPyramidLayer(const PyramidLayer& pyramidLayer) {
                m_pyramidLayers.push_back(pyramidLayer);
            }
            
            int32_t getNumberOfPyramidLayers() const {
                return m_pyramidLayers.size();
            }
            
            int32_t m_sceneIndex;
            
            QRectF m_logicalRectangle;
            
            std::vector<PyramidLayer> m_pyramidLayers;
        };
        
        class NiftiTransform {
        public:
            mutable std::unique_ptr<VolumeFile> m_niftiFile;
            
            mutable std::unique_ptr<Matrix4x4> m_sformMatrix;
            
            mutable bool m_triedToLoadFileFlag = false;
            
            /** Scales pixel index from full resolution to layer used by NIFTI transform */
            mutable float m_pixelScaleI = -1.0f;
            
            /** Scales pixel index from full resolution to layer used by NIFTI transform */
            mutable float m_pixelScaleJ = -1.0f;
            
            /** Volume is oriented left-to-right for first axis (same as CZI image) */
            bool m_xLeftToRightFlag = false;
            
            /** Volume is oriented top-to-bottom for second axis (same as CZI image) */
            bool m_yTopToBottomFlag = false;
        };
        
        class TestTransformResult {
        public:
            TestTransformResult(const PixelLogicalIndex& pixel,
                                const PixelLogicalIndex& pixelTwo,
                                const std::array<float, 3>& xyz,
                                const int64_t dI,
                                const int64_t dJ,
                                const float dIJK)
            : m_pixel(pixel),
            m_pixelTwo(pixelTwo),
            m_xyz(xyz),
            m_dI(dI),
            m_dJ(dJ),
            m_dIJK(dIJK) { }

            const PixelLogicalIndex m_pixel;
            const PixelLogicalIndex m_pixelTwo;
            const std::array<float, 3> m_xyz;
            const int64_t m_dI;
            const int64_t m_dJ;
            const float m_dIJK;
        };

        class TabOverlayInfo {
        public:
            void resetContent() {
                m_cziImage.reset();
                m_pyramidLevelChangedFlag = false;
                m_pyramidLevel = -1;
                m_logicalRect.setRect(0, 0, 0, 0);
            }
            
            std::unique_ptr<CziImage> m_cziImage;
            
            bool m_pyramidLevelChangedFlag = false;
            
            int32_t m_pyramidLevel = -1;
            
            QRectF m_logicalRect;
        };
        
        bool pixelIndexToStereotaxicXYZ(const PixelLogicalIndex& pixelLogicalIndex,
                                        const bool includeNonlinearFlag,
                                        std::array<float, 3>& xyzOut,
                                        std::array<float, 3>& debugPixelIndexOut) const;

        bool stereotaxicXyzToPixelIndex(const std::array<float, 3>& xyz,
                                        const bool includeNonlinearFlag,
                                        PixelLogicalIndex& pixelLogicalIndex,
                                        const std::array<float, 3>& debugPixelIndex) const;
        
        PixelCoordinate getPixelSizeInMillimeters() const;
        
        CziImage* loadImageForPyrmaidLayer(const int32_t tabIndex,
                                           const int32_t overlayIndex,
                                           const GraphicsObjectToWindowTransform* transform,
                                           const int32_t pyramidLayerIndex);
        
        CziImage* getImageForTabOverlay(const int32_t tabIndex,
                                        const int32_t overlayIndex);
        
        const CziImage* getImageForTabOverlay(const int32_t tabIndex,
                                              const int32_t overlayIndex) const;
        
        CziImage* getDefaultAllFramesImage();
        
        const CziImage* getDefaultAllFramesImage() const;
        
        CziImage* getDefaultFrameImage(const int32_t frameIndex);
        
        const CziImage* getDefaultFrameImage(const int32_t frameIndex) const;
        
        void readDefaultImage();
        
        CziImage* readDefaultFrameImage(const int32_t frameIndex);
        
        void closeFile();
        
        bool isPixelIndexFullResolutionValid(const PixelIndex& pixelIndex) const;
        
        CziImage* readFramePyramidLayerFromCziImageFile(const int32_t frameIndex,
                                                        const int32_t pyramidLayer,
                                                        const QRectF& logicalRectangleRegionRect,
                                                        const QRectF& rectangleForReadingRect,
                                                        AString& errorMessageOut);

        CziImage* readPyramidLayerFromCziImageFile(const int32_t pyramidLayer,
                                                   const QRectF& logicalRectangleRegionRect,
                                                   const QRectF& rectangleForReadingRect,
                                                   AString& errorMessageOut);
        
        CziImage* readFromCziImageFile(const QRectF& regionOfInterest,
                                       const int64_t outputImageWidthHeightMaximum,
                                       const CziImageResolutionChangeModeEnum::Enum resolutionChangeMode,
                                       const int32_t resolutionChangeModeLevel,
                                       AString& errorMessageOut);
        
        QImage* createQImageFromBitmapData(libCZI::IBitmapData* bitmapData,
                                           AString& errorMessageOut);
        
        void addToMetadataIfNotEmpty(const AString& name,
                                     const AString& text);
        
        void readMetaData();
        
        void readPyramidInfo(const libCZI::SubBlockStatistics& subBlockStatistics);
        
        int32_t getPyramidLayerWithMaximumResolution(const int32_t resolution) const;
        
        void pixelSizeToLogicalSize(const int32_t pyramidLayerIndex,
                                    int32_t& widthInOut,
                                    int32_t& heightOut) const;
        
        static int CalcSizeOfPixelOnLayer0(const libCZI::ISingleChannelPyramidLayerTileAccessor::PyramidLayerInfo& pyramidInfo);
        
        void autoTwoModePanZoomResolutionChange(const CziImage* cziImage,
                                                const int32_t tabIndex,
                                                const int32_t overlayIndex,
                                                const int32_t frameIndex,
                                                const bool allFramesFlag,
                                                const GraphicsObjectToWindowTransform* transform);
        
        int32_t autoModeZoomOnlyResolutionChange(const int32_t tabIndex,
                                                 const int32_t overlayIndex,
                                                 const GraphicsObjectToWindowTransform* transform);
        
        int32_t autoModePanZoomResolutionChange(const CziImage* cziImage,
                                                const int32_t tabIndex,
                                                const int32_t overlayIndex,
                                                const GraphicsObjectToWindowTransform* transform);

        QRectF moveAndClipRectangle(const QRectF& rectangleIn);
        
        void loadNiftiTransformFile(const AString& filename,
                                    NiftiTransform& transform) const;
        
        const Plane* getImagePlane() const;
        
        void resetPrivate();
        
        PixelLogicalIndex viewportXyToPixelLogicalIndex(const GraphicsObjectToWindowTransform* transform,
                                                        const float x,
                                                        const float y);

        float getIntersectedArea(const QRectF& rectOne,
                                 const QRectF& rectTwo) const;

        int32_t getPreferencesImageDimension() const;
        
        std::array<float, 3> getPreferencesImageBackgroundRGB() const;
        
        std::unique_ptr<SceneClassAssistant> m_sceneAssistant;

        Status m_status = Status::CLOSED;
        
        AString m_errorMessage;
        
        std::shared_ptr<libCZI::IStream> m_stream;
        
        std::shared_ptr<libCZI::ICZIReader> m_reader;

        std::shared_ptr<libCZI::ISingleChannelScalingTileAccessor> m_scalingTileAccessor;
        
        std::shared_ptr<libCZI::ISingleChannelPyramidLayerTileAccessor> m_pyramidLayerTileAccessor;
        
        std::vector<CziSceneInfo> m_cziSceneInfos;
        
        int32_t m_lowestResolutionPyramidLayerIndex = -1;
        
        int32_t m_highestResolutionPyramidLayerIndex = -1;
        
        int32_t m_numberOfPyramidLayers = 0;
        
        float m_pixelSizeMmX = 1.0f;
        
        float m_pixelSizeMmY = 1.0f;
        
        float m_pixelSizeMmZ = 1.0f;
        
        mutable std::unique_ptr<GiftiMetaData> m_fileMetaData;
        
        std::unique_ptr<CziImage> m_defaultAllFramesImage;
        
        std::vector<std::unique_ptr<CziImage>> m_defaultFrameImages;
        
        std::array<std::unique_ptr<CziImage>, BrainConstants::MAXIMUM_NUMBER_OF_BROWSER_TABS> m_tabCziImages;
        
        std::unique_ptr<TabOverlayInfo> m_tabOverlayInfo[BrainConstants::MAXIMUM_NUMBER_OF_BROWSER_TABS]
                                                        [BrainConstants::MAXIMUM_NUMBER_OF_OVERLAYS];
        
        /*
         * While we could reset the image to cause a change that may result in failure
         * to display an image for a frame, so use flag to load new resolution.
         */
        std::array<bool, BrainConstants::MAXIMUM_NUMBER_OF_BROWSER_TABS> m_tabCziImagePyramidLevelChanged;
        
        /*
         * Logical rectangle of full-resolution image
         */
        QRectF m_fullResolutionLogicalRect;
        
        std::vector<PyramidLayer> m_pyramidLayers;
        
        std::array<int32_t, BrainConstants::MAXIMUM_NUMBER_OF_BROWSER_TABS> m_pyramidLayerIndexInTabs;
             
        std::unique_ptr<CziImageLoaderAllFrames> m_allFramesCziImageLoader;
        
        mutable NiftiTransform m_pixelToStereotaxicTransform;
        
        mutable NiftiTransform m_stereotaxicToPixelTransform;
        
        mutable std::unique_ptr<Plane> m_imagePlane;
        
        mutable bool m_imagePlaneInvalid = false;
        
        // ADD_NEW_MEMBERS_HERE

        friend class CziImage;
        friend class CziImageLoaderAllFrames;
        
    };
    
#ifdef __CZI_IMAGE_FILE_DECLARE__
    // <PLACE DECLARATIONS OF STATIC MEMBERS HERE>
#endif // __CZI_IMAGE_FILE_DECLARE__

} // namespace
#endif  //__CZI_IMAGE_FILE_H__
