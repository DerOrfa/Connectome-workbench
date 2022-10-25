
/*LICENSE_START*/
/*
 *  Copyright (C) 2022 Washington University School of Medicine
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

#define __HISTOLOGY_SLICE_DECLARE__
#include "HistologySlice.h"
#undef __HISTOLOGY_SLICE_DECLARE__

#include "CaretAssert.h"
#include "EventManager.h"
#include "HistologySliceImage.h"
#include "CziNonLinearTransform.h"
#include "DataFileContentInformation.h"
#include "MediaFile.h"
#include "Plane.h"
#include "SceneClass.h"
#include "SceneClassAssistant.h"

using namespace caret;


    
/**
 * \class caret::HistologySlice 
 * \brief Contains images for a slice
 * \ingroup Files
 */

/**
 * Constructor.
 * @param sliceIndex
 *    Index of the slice
 * @param sliceNumber
 *    Number of the slice
 * @param MRIToHistWarpFileName,
 *    Name of MRI to Hist File
 * @param histToMRIWarpFileName,
 *    Name of Hist to MRI File
 * @param planeToMillimetersMatrix,
 *    The plane to millimeters matrix
 * @param planeToMillimetersMatrixValidFlag
 *    Validity of plane to millimeters matrix
 */
HistologySlice::HistologySlice(const int32_t sliceIndex,
                               const int32_t sliceNumber,
                               const AString& MRIToHistWarpFileName,
                               const AString& histToMRIWarpFileName,
                               const Matrix4x4& planeToMillimetersMatrix,
                               const bool planeToMillimetersMatrixValidFlag)
: CaretObject(),
m_sliceIndex(sliceIndex),
m_sliceNumber(sliceNumber),
m_MRIToHistWarpFileName(MRIToHistWarpFileName),
m_histToMRIWarpFileName(histToMRIWarpFileName),
m_planeToMillimetersMatrix(planeToMillimetersMatrix),
m_planeToMillimetersMatrixValidFlag(planeToMillimetersMatrixValidFlag)
{
    
    m_sceneAssistant = std::unique_ptr<SceneClassAssistant>(new SceneClassAssistant());
    
    m_fromStereotaxicNonLinearTransform.reset(new CziNonLinearTransform(CziNonLinearTransform::Mode::FROM_MILLIMETERS,
                                                                        m_histToMRIWarpFileName));
    m_toStereotaxicNonLinearTransform.reset(new CziNonLinearTransform(CziNonLinearTransform::Mode::TO_MILLIMETERS,
                                                                      m_MRIToHistWarpFileName));
//    EventManager::get()->addEventListener(this, EventTypeEnum::);
}

/**
 * Destructor.
 */
HistologySlice::~HistologySlice()
{
    EventManager::get()->removeAllEventsFromListener(this);
}

/**
 * Copy constructor.
 * @param obj
 *    Object that is copied.
 */
HistologySlice::HistologySlice(const HistologySlice& obj)
: CaretObject(obj),
EventListenerInterface(),
SceneableInterface(obj)
{
    this->copyHelperHistologySlice(obj);
}

/**
 * Assignment operator.
 * @param obj
 *    Data copied from obj to this.
 * @return 
 *    Reference to this object.
 */
HistologySlice&
HistologySlice::operator=(const HistologySlice& obj)
{
    if (this != &obj) {
        CaretObject::operator=(obj);
        this->copyHelperHistologySlice(obj);
    }
    return *this;    
}

/**
 * Helps with copying an object of this type.
 * @param obj
 *    Object that is copied.
 */
void 
HistologySlice::copyHelperHistologySlice(const HistologySlice& /*obj*/)
{
    CaretAssertMessage(0, "Copying not supported");
    m_stereotaxicXyzBoundingBoxValidFlag = false;
    m_planeXyzBoundingBoxValidFlag       = false;
}

/**
 * @return Index of the slice
 */
int32_t
HistologySlice::getSliceIndex() const
{
    return m_sliceIndex;
}

/**
 * @return Number of the slice
 */
int32_t
HistologySlice::getSliceNumber() const
{
    return m_sliceNumber;
}

/**
 * Add an histology slice image
 * @param histologySliceImage
 *    Image to add.  This instance will take ownership of the image.
 */
void
HistologySlice::addHistologySliceImage(HistologySliceImage* histologySliceImage)
{
    CaretAssert(histologySliceImage);
    histologySliceImage->setPlaneToMillimetersMatrix(m_planeToMillimetersMatrix,
                                                     m_planeToMillimetersMatrixValidFlag,
                                                     m_toStereotaxicNonLinearTransform,
                                                     m_fromStereotaxicNonLinearTransform);
    std::unique_ptr<HistologySliceImage> ptr(histologySliceImage);
    m_histologySliceImages.push_back(std::move(ptr));
    
    m_stereotaxicXyzBoundingBoxValidFlag = false;
    m_planeXyzBoundingBoxValidFlag       = false;
}

/**
 * @return Number of histology slice images
 */
int32_t
HistologySlice::getNumberOfHistologySliceImages() const
{
    return m_histologySliceImages.size();
}

/**
 * @return Histology slice image at the given index
 * @param index
 *    Index of the image
 */
HistologySliceImage*
HistologySlice::getHistologySliceImage(const int32_t index)
{
    CaretAssertVectorIndex(m_histologySliceImages, index);
    return m_histologySliceImages[index].get();
}

/**
 * @return Histology slice image at the given index, const method
 * @param index
 *    Index of the image
 */
const HistologySliceImage*
HistologySlice::getHistologySliceImage(const int32_t index) const
{
    CaretAssertVectorIndex(m_histologySliceImages, index);
    return m_histologySliceImages[index].get();
}

/**
 * @return stereotaxic BoundingBox for the slice (bounding box of all images in slice)
 */
BoundingBox
HistologySlice::getStereotaxicXyzBoundingBox() const
{
    if ( ! m_stereotaxicXyzBoundingBoxValidFlag) {
        m_stereotaxicXyzBoundingBox.resetForUpdate();
        
        for (auto& slice : m_histologySliceImages) {
            const MediaFile* mf(slice->getMediaFile());
            if (mf != NULL) {
                BoundingBox bb(mf->getStereotaxicXyzBoundingBox());
                m_stereotaxicXyzBoundingBox.unionOperation(bb);
            }
        }
        m_stereotaxicXyzBoundingBoxValidFlag = true;
    }
    
    return m_stereotaxicXyzBoundingBox;
}

/**
 * @return Plane BoundingBox for the slice (bounding box of all images in slice)
 */
BoundingBox
HistologySlice::getPlaneXyzBoundingBox() const
{
    if ( ! m_planeXyzBoundingBoxValidFlag) {
        m_planeXyzBoundingBox.resetForUpdate();
        
        for (auto& slice : m_histologySliceImages) {
            const MediaFile* mf(slice->getMediaFile());
            if (mf != NULL) {
                BoundingBox bb(mf->getPlaneXyzBoundingBox());
                m_planeXyzBoundingBox.unionOperation(bb);
            }
        }
        m_planeXyzBoundingBoxValidFlag = true;
    }
    
    return m_planeXyzBoundingBox;
}

/**
 * Convert a plane XYZ to stereotaxic XYZ
 * @param planeXyz
 *     XYZ in plane
 * @param stereotaxicXyzOut
 *    Output with stereotaxic XYZ
 * @return True if successful, else false.
 */
bool
HistologySlice::planeXyzToStereotaxicXyz(const Vector3D& planeXyz,
                                         Vector3D& stereotaxicXyzOut) const
{
    Vector3D unusedXyz;
    return planeXyzToStereotaxicXyz(planeXyz,
                                    unusedXyz,
                                    stereotaxicXyzOut);
}

/**
 * Convert a plane XYZ to stereotaxic XYZ
 * @param planeXyz
 *     XYZ in plane
 * @param stereotaxicNoNonLinearXyzOut
 *    Output with stereotaxic XYZ but non-linear offset
 * @param stereotaxicWithNonLinearXyzOut
 *    Output with stereotaxic XYZ but with non-linear offset
 * @return True if successful, else false.
 */
bool
HistologySlice::planeXyzToStereotaxicXyz(const Vector3D& planeXyz,
                                         Vector3D& stereotaxicNoNonLinearXyzOut,
                                         Vector3D& stereotaxicWithNonLinearXyzOut) const
{
    const HistologySliceImage* firstSliceImage(getHistologySliceImage(0));
    if (firstSliceImage != NULL) {
        const MediaFile* mediaFile(firstSliceImage->getMediaFile());
        if (mediaFile->planeXyzToStereotaxicXyz(planeXyz,
                                                stereotaxicNoNonLinearXyzOut,
                                                stereotaxicWithNonLinearXyzOut)) {
            return true;
        }
    }
    stereotaxicNoNonLinearXyzOut.fill(0);
    stereotaxicWithNonLinearXyzOut.fill(0);
    return false;
}

/**
 * Converrt a stereotaxic coordinate to a plane coordinate
 * @param stereotaxicXyz
 *    Input stereotaxic coordinate
 * @param planeXyzOut
 *    Output plane coordinate
 * @return True if successful, else false
 */
bool
HistologySlice::stereotaxicXyzToPlaneXyz(const Vector3D& stereotaxicXyz,
                                         Vector3D& planeXyzOut) const
{
    Vector3D unsusedXyz;
    return stereotaxicXyzToPlaneXyz(stereotaxicXyz,
                                    unsusedXyz,
                                    planeXyzOut);
}

/**
 * Converrt a stereotaxic coordinate to a plane coordinate
 * @param stereotaxicXyz
 *    Input stereotaxic coordinate
 * @param planeXyzOut
 *    Output plane coordinate
 * @return True if successful, else false
 */
bool
HistologySlice::stereotaxicXyzToPlaneXyz(const Vector3D& stereotaxicXyz,
                                         Vector3D& planeNoNonLinearXyzOut,
                                         Vector3D& planeWithNonLinearXyzOut) const
{
    /*
     * Stereotaxic <-> Plane is same for all images
     */
    const HistologySliceImage* firstSliceImage(getHistologySliceImage(0));
    if (firstSliceImage != NULL) {
        const MediaFile* mediaFile(firstSliceImage->getMediaFile());
        if (mediaFile->stereotaxicXyzToPlaneXyz(stereotaxicXyz,
                                                planeNoNonLinearXyzOut,
                                                planeWithNonLinearXyzOut)) {
            return true;
        }
    }
    planeNoNonLinearXyzOut.fill(0);
    planeWithNonLinearXyzOut.fill(0);
    return false;
}

/**
 * @return Plane for stereotaxic coordinates
 */
const Plane&
HistologySlice::getStereotaxicPlane() const
{
    if ( ! m_stereotaxicPlaneValidFlag) {
        if (getNumberOfHistologySliceImages() > 0) {
            /*
             * Use plane from first image as all images in slice should
             * be close to being in the same plane
             */
            const HistologySliceImage* histologyImage(getHistologySliceImage(0));
            CaretAssert(histologyImage);
            const MediaFile* mediaFile(histologyImage->getMediaFile());
            if (mediaFile != NULL) {
                const Plane* plane(mediaFile->getStereotaxicImagePlane());
                if (plane->isValidPlane()) {
                    m_stereotaxicPlane = *plane;
                    m_stereotaxicPlaneValidFlag = true;
                }
            }
        }
    }
    
    return m_stereotaxicPlane;
}

/**
 * Project the givent stereotaxic coordinate onto the slice
 * @param stereotaxicXYZ
 *    Input stereotaxic coordinate
 * @param stereotaxicOnSliceXYZ
 *    Output stereotaxic coordinate projected to the slice
 * @return
 *    True if successful, else false
 */
bool
HistologySlice::projectStereotaxicXyzToSlice(const Vector3D stereotaxicXYZ,
                                            Vector3D& stereotaxicOnSliceXYZ) const
{
    const Plane plane(getStereotaxicPlane());
    if (plane.isValidPlane()) {
        plane.projectPointToPlane(stereotaxicXYZ,
                                  stereotaxicOnSliceXYZ);
        return true;
    }
    return false;
}

/**
 * Project the givent stereotaxic coordinate onto the slice
 * @param stereotaxicXYZ
 *    Input stereotaxic coordinate
 * @param stereotaxicOnSliceXYZ
 *    Output stereotaxic coordinate projected to the slice
 * @param stereotaxicDistanceToSliceOut
 *    Distance to slice
 * @param planeOnSliceXYZ
 *    Plane coordinate of point on slice plane
 * @return
 *    True if successful, else false
 */
bool
HistologySlice::projectStereotaxicXyzToSlice(const Vector3D& stereotaxicXYZ,
                                             Vector3D& stereotaxicOnSliceXYZ,
                                             float& stereotaxicDistanceToSliceOut,
                                             Vector3D& planeOnSliceXYZ) const
{
    if (projectStereotaxicXyzToSlice(stereotaxicXYZ,
                                     stereotaxicOnSliceXYZ)) {
        if (stereotaxicXyzToPlaneXyz(stereotaxicOnSliceXYZ,
                                     planeOnSliceXYZ)) {
            stereotaxicDistanceToSliceOut = (stereotaxicXYZ - stereotaxicOnSliceXYZ).length();
            return true;
        }
    }

    return false;
}

/**
 * @return Plane for plane  coordinates
 */
const Plane&
HistologySlice::getPlaneXyzPlane() const
{
    if ( ! m_planeXyzPlaneValidFlag) {
        if (getNumberOfHistologySliceImages() > 0) {
            /*
             * Use plane from first image as all images in slice should
             * be close to being in the same plane
             */
            const HistologySliceImage* histologyImage(getHistologySliceImage(0));
            CaretAssert(histologyImage);
            const MediaFile* mediaFile(histologyImage->getMediaFile());
            if (mediaFile != NULL) {
                const Plane* plane(mediaFile->getPlaneCoordinatesPlane());
                if (plane->isValidPlane()) {
                    m_planeXyzPlane = *plane;
                    m_planeXyzPlaneValidFlag = true;
                }
            }
        }
    }
    
    return m_planeXyzPlane;
}

/**
 * Find the media file in the histology slice images
 * @param mediaFileName
 *    Name of media file
 * @return
 *    Media file with the given name or NULL if not found
 */
MediaFile*
HistologySlice::findMediaFileWithName(const AString& mediaFileName) const
{
    for (auto& hsi : m_histologySliceImages) {
        MediaFile* mf(hsi->getMediaFile());
        if (mf != NULL) {
            if (mf->getFileName() == mediaFileName) {
                return mf;
            }
        }
    }
    
    return NULL;
}

/**
 * Get a description of this object's content.
 * @return String describing this object's content.
 */
AString 
HistologySlice::toString() const
{
    AString s("Histology slice "
              + AString::number(getSliceNumber()));
    s.appendWithNewLine("   MRI to Hist File: " + m_MRIToHistWarpFileName);
    s.appendWithNewLine("   Hist to MRI File: " + m_histToMRIWarpFileName);
    const int32_t numImages(getNumberOfHistologySliceImages());
    for (int32_t i = 0; i < numImages; i++) {
        s.appendWithNewLine(getHistologySliceImage(i)->toString());
    }
    return s;
}

/**
 * Receive an event.
 *
 * @param event
 *    An event for which this instance is listening.
 */
void
HistologySlice::receiveEvent(Event* /*event*/)
{
//    if (event->getEventType() == EventTypeEnum::) {
//        <EVENT_CLASS_NAME*> eventName = dynamic_cast<EVENT_CLASS_NAME*>(event);
//        CaretAssert(eventName);
//
//        event->setEventProcessed();
//    }
}

/**
 * @return Names (absolute path) of all child data files of this file.
 * This includes the CZI Image Files, Distance File, and the Non-Linear
 * Transform Files.
 */
std::vector<AString>
HistologySlice::getChildDataFilePathNames() const
{
    std::vector<AString> childDataFilePathNames;
    childDataFilePathNames.push_back(m_MRIToHistWarpFileName);
    childDataFilePathNames.push_back(m_histToMRIWarpFileName);
    
    for (const auto& image : m_histologySliceImages) {
        std::vector<AString> names(image->getChildDataFilePathNames());
        childDataFilePathNames.insert(childDataFilePathNames.end(),
                                      names.begin(), names.end());
    }

    return childDataFilePathNames;
}

/**
 * Save information specific to this type of model to the scene.
 *
 * @param sceneAttributes
 *    Attributes for the scene.  Scenes may be of different types
 *    (full, generic, etc) and the attributes should be checked when
 *    saving the scene.
 *
 * @param instanceName
 *    Name of instance in the scene.
 */
SceneClass*
HistologySlice::saveToScene(const SceneAttributes* sceneAttributes,
                                 const AString& instanceName)
{
    SceneClass* sceneClass = new SceneClass(instanceName,
                                            "HistologySlice",
                                            1);
    m_sceneAssistant->saveMembers(sceneAttributes,
                                  sceneClass);
    
    // Uncomment if sub-classes must save to scene
    //saveSubClassDataToScene(sceneAttributes,
    //                        sceneClass);
    
    return sceneClass;
}

/**
 * Restore information specific to the type of model from the scene.
 *
 * @param sceneAttributes
 *    Attributes for the scene.  Scenes may be of different types
 *    (full, generic, etc) and the attributes should be checked when
 *    restoring the scene.
 *
 * @param sceneClass
 *     sceneClass from which model specific information is obtained.
 */
void
HistologySlice::restoreFromScene(const SceneAttributes* sceneAttributes,
                                      const SceneClass* sceneClass)
{
    if (sceneClass == NULL) {
        return;
    }
    
    m_sceneAssistant->restoreMembers(sceneAttributes,
                                     sceneClass);    
    
    //Uncomment if sub-classes must restore from scene
    //restoreSubClassDataFromScene(sceneAttributes,
    //                             sceneClass);
    
}

/**
 * Add information about the slice to the data file information.
 *
 * @param dataFileInformation
 *    Consolidates information about a data file.
 */
void
HistologySlice::addToDataFileContentInformation(DataFileContentInformation& dataFileInformation)
{
    dataFileInformation.addNameAndValue("Slice Index", getSliceIndex());
    dataFileInformation.addNameAndValue("Slice Number", getSliceNumber());
    
    const int32_t numImages(getNumberOfHistologySliceImages());
    for (int32_t jImage = 0; jImage < numImages; jImage++) {
        const HistologySliceImage* image(getHistologySliceImage(jImage));
        const MediaFile* mediaFile(image->getMediaFile());
        dataFileInformation.addNameAndValue("Image",
                                            (mediaFile->getFileNameNoPath()
                                             + " ("
                                             + mediaFile->getFilePath()
                                             + ")"));
        dataFileInformation.addNameAndValue("Stereotaxic Plane Equation",
                                            mediaFile->getStereotaxicImagePlane()->toString());
        const_cast<MediaFile*>(mediaFile)->addPlaneCoordsToDataFileContentInformation(dataFileInformation);
    }
}
