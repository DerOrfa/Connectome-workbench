
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

#define __MEDIA_FILE_DECLARE__
#include "MediaFile.h"
#undef __MEDIA_FILE_DECLARE__

#include "CaretAssert.h"
#include "CaretLogger.h"
#include "SceneClass.h"
#include "SceneClassAssistant.h"

using namespace caret;


    
/**
 * \class caret::MediaFile 
 * \brief Base class for media type files (image, movie)
 * \ingroup Files
 */

/**
 * Constructor.
 * @param dataFileType
 *    Type of data file
 */
MediaFile::MediaFile(const DataFileTypeEnum::Enum dataFileType)
: CaretDataFile(dataFileType)
{
    switch (dataFileType) {
        case DataFileTypeEnum::CZI_IMAGE_FILE:
        case DataFileTypeEnum::IMAGE:
            break;
        default:
        {
            const AString msg("Invalid data file type="
                               + DataFileTypeEnum::toName(dataFileType)
                               + ".  Has new file type been added?");
            CaretAssertMessage(0, msg);
            CaretLogSevere(msg);
        }
    }
    
    initializeMembersMediaFile();
    
}

/**
 * Copy constructor.
 * @param mediaFile
 *    Media file that is copied.
 */
MediaFile::MediaFile(const MediaFile& mediaFile)
: CaretDataFile(mediaFile)
{
    initializeMembersMediaFile();
}

/**
 * Destructor.
 */
MediaFile::~MediaFile()
{
}

/**
 * Initialize members of media file
 */
void
MediaFile::initializeMembersMediaFile()
{
    m_sceneAssistant = std::unique_ptr<SceneClassAssistant>(new SceneClassAssistant());
}


/**
 * @return Name of frame at given index.
 * @param frameIndex Index of the frame
 */
AString
MediaFile::getFrameName(const int32_t frameIndex) const
{
    CaretAssert((frameIndex >= 0) && (frameIndex < getNumberOfFrames()));
    const AString defaultFrameName(AString::number(frameIndex+1));
    return defaultFrameName;
}

/**
 * @return The units for the 'interval' between two consecutive frames.
 */
NiftiTimeUnitsEnum::Enum
MediaFile::getFrameIntervalUnits() const
{
    return NiftiTimeUnitsEnum::NIFTI_UNITS_UNKNOWN;
}

/**
 * Get the units value for the first frame and the
 * quantity of units between consecutive frames.  If the
 * units for the frame is unknown, value of one (1) are
 * returned for both output values.
 *
 * @param firstFrameUnitsValueOut
 *     Output containing units value for first frame.
 * @param frameIntervalStepValueOut
 *     Output containing number of units between consecutive frame.
 */
void
MediaFile::getFrameIntervalStartAndStep(float& firstFrameUnitsValueOut,
                                          float& frameIntervalStepValueOut) const
{
    firstFrameUnitsValueOut   = 1.0;
    frameIntervalStepValueOut = 1.0;
}

/**
 * @return The structure for this file.
 */
StructureEnum::Enum
MediaFile::getStructure() const
{
    return StructureEnum::INVALID;
}

/**
 * Set the structure for this file.
 * @param structure
 *   New structure for this file.
 */
void
MediaFile::setStructure(const StructureEnum::Enum /*structure */)
{
    /* File does not support structures */
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
MediaFile::saveFileDataToScene(const SceneAttributes* sceneAttributes,
                                            SceneClass* sceneClass)
{
    CaretDataFile::saveFileDataToScene(sceneAttributes,
                                       sceneClass);
    m_sceneAssistant->saveMembers(sceneAttributes,
                                  sceneClass);
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
MediaFile::restoreFileDataFromScene(const SceneAttributes* sceneAttributes,
                                                 const SceneClass* sceneClass)
{
    CaretDataFile::restoreFileDataFromScene(sceneAttributes,
                                            sceneClass);
    m_sceneAssistant->restoreMembers(sceneAttributes,
                                     sceneClass);
}

/**
 * @return File casted to an media file (avoids use of dynamic_cast that can be slow)
 * Overidden in MediaFile
 */
MediaFile*
MediaFile::castToMediaFile()
{
    return this;
}

/**
 * @return File casted to an media file (avoids use of dynamic_cast that can be slow)
 * Overidden in ImageFile
 */
const MediaFile*
MediaFile::castToMediaFile() const
{
    return this;
}

/**
 * @return A pixel index converted from a pixel logical index.
 * @param pixelLogicalIndex
 *    The logical pixel index.
 */
PixelIndex
MediaFile::pixelLogicalIndexToPixelIndex(const PixelLogicalIndex& pixelLogicalIndex) const
{
    PixelIndex pixelIndex(pixelLogicalIndex.getI(),
                          pixelLogicalIndex.getJ(),
                          pixelLogicalIndex.getK());
    
    return pixelIndex;
}

/**
 * @return A pixel logical index converted from a pixel index.
 * @param pixelIndex
 *    The  pixel index.
 */
PixelLogicalIndex
MediaFile::pixelIndexToPixelLogicalIndex(const PixelIndex& pixelIndex) const
{
    PixelLogicalIndex pixelLogicalIndex(pixelIndex.getI(),
                                        pixelIndex.getJ(),
                                        pixelIndex.getK());
    
    return pixelLogicalIndex;
}

/**
 * Convert a pixel index to a plane XYZ.  If not supported, output is same as input.
 * @param pixelIndex
 *    Index of pixel
 * @param planeXyzOut
 *    Output with XYZ in plane
 * @return True if successful, else false.
 */
bool
MediaFile::pixelIndexToPlaneXYZ(const PixelIndex& pixelIndex,
                                  Vector3D& planeXyzOut) const
{
    planeXyzOut[0] = pixelIndex.getI();
    planeXyzOut[1] = pixelIndex.getJ();
    planeXyzOut[2] = pixelIndex.getK();
    return false;
}

/**
 * Convert a pixel index to a plane XYZ.  If not supported, output is same as input.
 * @param pixelIndex
 *    Index of pixel
 * @param planeXyzOut
 *    Output with XYZ in plane
 * @return True if successful, else false.
 */
bool
MediaFile::logicalPixelIndexToPlaneXYZ(const PixelLogicalIndex& pixelLogicalIndex,
                                         Vector3D& planeXyzOut) const
{
    planeXyzOut[0] = pixelLogicalIndex.getI();
    planeXyzOut[1] = pixelLogicalIndex.getJ();
    planeXyzOut[2] = pixelLogicalIndex.getK();
    return false;
}

/**
 * Convert a pixel XYZ to a pixel index.  If not supported, output is same as input.
 * @param planeXyz
 *     XYZ in plane
 * @param pixelIndexOut
 *    Index of pixel
 * @return True if successful, else false.
 */
bool
MediaFile::planeXyzToPixelIndex(const Vector3D& planeXyz,
                                  PixelIndex& pixelIndexOut) const
{
    pixelIndexOut.setIJK(planeXyz);
    return false;
}

/**
 * Convert a pixel XYZ to a logical pixel index.  If not supported, output is same as input.
 * @param planeXyz
 *     XYZ in plane
 * @param logicalPixelIndexOut
 *    Index of pixel
 * @return True if successful, else false.
 */
bool
MediaFile::planeXyzToLogicalPixelIndex(const Vector3D& planeXyz,
                                         PixelLogicalIndex& pixelLogicalIndexOut) const
{
    pixelLogicalIndexOut.setIJK(planeXyz);
    return false;
}


/**
 * @return Return a rectangle that defines the bounds of the media data
 */
QRectF
MediaFile::getLogicalBoundsRect() const
{
    QRectF rect(0, 0, getWidth(), getHeight());
    return rect;
}



