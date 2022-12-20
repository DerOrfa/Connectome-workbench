#ifndef __VIEWING_TRANSFORMATION_TO_FIT_REGION_H__
#define __VIEWING_TRANSFORMATION_TO_FIT_REGION_H__

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



#include <memory>

#include "BoundingBox.h"
#include "CaretObject.h"
#include "Vector3D.h"
#include "GraphicsViewport.h"

namespace caret {

    class BrainOpenGLViewportContent;
    class BrowserTabContent;
    class GraphicsRegionSelectionBox;
    class MouseEvent;
    
    class ViewingTransformationToFitRegion : public CaretObject {
        
    public:
        ViewingTransformationToFitRegion(const MouseEvent* mouseEvent,
                                         const GraphicsRegionSelectionBox* selectedRegion,
                                         const BrowserTabContent* browserTabContent);
        
        virtual ~ViewingTransformationToFitRegion();
        
        ViewingTransformationToFitRegion(const ViewingTransformationToFitRegion&) = delete;

        ViewingTransformationToFitRegion& operator=(const ViewingTransformationToFitRegion&) = delete;

        bool applyToOrthogonalVolume(const Vector3D& translationIn,
                                     Vector3D& translationOut,
                                     float& zoomOut);

        // ADD_NEW_METHODS_HERE

        virtual AString toString() const;
        
    private:
        bool initializeData();
        
        bool generateZoom(float& zoomOut);
        
        const MouseEvent* m_mouseEvent = NULL;
        
        const BrainOpenGLViewportContent* m_viewportContent = NULL;
        
        const GraphicsRegionSelectionBox* m_selectedRegion = NULL;
        
        const BrowserTabContent* m_browserTabContent = NULL;
        
        BoundingBox m_selectionRegionBounds;
        
        GraphicsViewport m_viewport;
        
        float m_viewportAspectRatio = 0.0;
        
        GraphicsViewport m_selectionRegionViewport;
        
        float m_selectionRegionViewportAspectRatio = 0.0;
        
        // ADD_NEW_MEMBERS_HERE

    };
    
#ifdef __VIEWING_TRANSFORMATION_TO_FIT_REGION_DECLARE__
    // <PLACE DECLARATIONS OF STATIC MEMBERS HERE>
#endif // __VIEWING_TRANSFORMATION_TO_FIT_REGION_DECLARE__

} // namespace
#endif  //__VIEWING_TRANSFORMATION_TO_FIT_REGION_H__
