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



#include <memory>

#include <QRectF>

#include "MediaFile.h"
#include "libCZI.h"

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
    class GraphicsPrimitiveV3fT3f;
    class RectangleTransform;
    class VolumeSpace;
    
    class CziImageFile : public MediaFile {
        
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
        
        virtual int32_t getNumberOfFrames() const override;
        
        virtual DefaultViewTransform getDefaultViewTransform(const int32_t tabIndex) const override;

        virtual void getPixelIdentificationText(const int32_t tabIndex,
                                                const PixelIndex& pixelIndex,
                                                std::vector<AString>& columnOneTextOut,
                                                std::vector<AString>& columnTwoTextOut) const;
        
        virtual void addToDataFileContentInformation(DataFileContentInformation& dataFileInformation) override;
        
        virtual void readFile(const AString& filename) override;
        
        virtual void writeFile(const AString& filename) override;

        virtual bool supportsWriting() const override;
        
        CziImage* getImageForTab(const int32_t tabIndex);
        
        const CziImage* getImageForTab(const int32_t tabIndex) const;

        bool getImagePixelRGBA(const int32_t tabIndex,
                               const IMAGE_DATA_ORIGIN_LOCATION imageOrigin,
                               const PixelIndex& pixelIndex,
                               uint8_t pixelRGBAOut[4]) const;
        
        PixelCoordinate getPixelSizeInMillimeters() const;
        
        // ADD_NEW_METHODS_HERE

          
          
          
          
          
    protected: 
        virtual const BoundingBox* getSpatialBoundingBox(const int32_t tabIndex) const override;
        
        virtual const VolumeSpace* getPixelToCoordinateTransform(const int32_t tabIndex) const override;
        
        virtual void saveSubClassDataToScene(const SceneAttributes* sceneAttributes,
                                             SceneClass* sceneClass);

        virtual void restoreSubClassDataFromScene(const SceneAttributes* sceneAttributes,
                                                  const SceneClass* sceneClass);

    private:
        enum class Status {
            CLOSED,
            OPEN,
            ERRORED  /* Note: "ERROR" fails to compile on Windows */
        };

        class PyramidLayer {
        public:
            PyramidLayer(const libCZI::ISingleChannelPyramidLayerTileAccessor::PyramidLayerInfo layerInfo,
                         const int64_t width,
                         const int64_t height)
            : m_layerInfo(layerInfo),
            m_width(width),
            m_height(height) { }
            
            const libCZI::ISingleChannelPyramidLayerTileAccessor::PyramidLayerInfo m_layerInfo;
            
            const int64_t m_width;
            
            const int64_t m_height;
        };
        CziImage* getDefaultImage();
        
        const CziImage* getDefaultImage() const;
        
        void closeFile();
        
        CziImage* readPyramidLevelFromCziImageFile(const int32_t pyramidLevel,
                                                      AString& errorMessageOut);
        
        CziImage* readFromCziImageFile(const QRectF& regionOfInterest,
                                          const int64_t outputImageWidthHeightMaximum,
                                          AString& errorMessageOut);
        
        QImage* createQImageFromBitmapData(libCZI::IBitmapData* bitmapData,
                                           AString& errorMessageOut);
        
        void addToMetadataIfNotEmpty(const AString& name,
                                     const AString& text);
        
        void readMetaData();
        
        void readPyramidInfo(const int64_t imageWidth,
                             const int64_t imageHeight);
        
        std::unique_ptr<SceneClassAssistant> m_sceneAssistant;

        Status m_status = Status::CLOSED;
        
        AString m_errorMessage;
        
        std::shared_ptr<libCZI::IStream> m_stream;
        
        std::shared_ptr<libCZI::ICZIReader> m_reader;

        std::shared_ptr<libCZI::ISingleChannelScalingTileAccessor> m_scalingTileAccessor;
        
        std::shared_ptr<libCZI::ISingleChannelPyramidLayerTileAccessor> m_pyramidLayerTileAccessor;
        
        int32_t m_numberOfPyramidLayers = 0;
        
        float m_pixelSizeMmX = 1.0f;
        
        float m_pixelSizeMmY = 1.0f;
        
        float m_pixelSizeMmZ = 1.0f;
        
        mutable DefaultViewTransform m_defaultViewTransform;
        
        mutable bool m_defaultViewTransformValidFlag = false;

        mutable std::unique_ptr<GiftiMetaData> m_fileMetaData;
        
        std::unique_ptr<CziImage> m_defaultImage;
        
        /*
         * Logical rectangle of full-resolution image
         */
        QRectF m_fullResolutionLogicalRect;
        
        std::vector<PyramidLayer> m_pyramidLayers;
        
        // ADD_NEW_MEMBERS_HERE

    };
    
#ifdef __CZI_IMAGE_FILE_DECLARE__
    // <PLACE DECLARATIONS OF STATIC MEMBERS HERE>
#endif // __CZI_IMAGE_FILE_DECLARE__

} // namespace
#endif  //__CZI_IMAGE_FILE_H__
