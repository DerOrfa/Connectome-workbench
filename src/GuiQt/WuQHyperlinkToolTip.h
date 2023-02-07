#ifndef __WUQ_HYPERLINK_TOOL_TIP_H__
#define __WUQ_HYPERLINK_TOOL_TIP_H__

/*LICENSE_START*/
/*
 *  Copyright (C) 2023 Washington University School of Medicine
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

#include <QObject>

class QWidget;

namespace caret {

//    class WuQHyperlinkReceiver {
//    public:
//        WuQHyperlinkReceiver();
//
//        virtual ~WuQHyperlinkReceiver();
//
//        virtual void hyperlinkClicked(const QString& hyperlink);
//    };
    
    class WuQHyperlinkToolTip : public QObject {
        
        Q_OBJECT

    public:
        virtual ~WuQHyperlinkToolTip();
        
        WuQHyperlinkToolTip(const WuQHyperlinkToolTip&) = delete;

        WuQHyperlinkToolTip& operator=(const WuQHyperlinkToolTip&) = delete;
        
        static WuQHyperlinkToolTip* instance();
        
        static void add(QWidget* widget);
        
//        static void setHyperlinkReceiver(WuQHyperlinkReceiver* hyperlinkReciver);
        
        // ADD_NEW_METHODS_HERE

    signals:
        void hyperlinkClicked(const QString& hyperlink);
        
    protected:
        bool eventFilter(QObject *object, QEvent *event) override;

    private:
        WuQHyperlinkToolTip(QObject* parent);
        
        static WuQHyperlinkToolTip* s_instance;

//        static WuQHyperlinkReceiver* s_hyperlinkReceiver;
        
        // ADD_NEW_MEMBERS_HERE

    };
    
#ifdef __WUQ_HYPERLINK_TOOL_TIP_DECLARE__
    WuQHyperlinkToolTip* WuQHyperlinkToolTip::s_instance = NULL;
//    WuQHyperlinkReceiver* WuQHyperlinkToolTip::s_hyperlinkReceiver = NULL;
#endif // __WUQ_HYPERLINK_TOOL_TIP_DECLARE__

} // namespace
#endif  //__WUQ_HYPERLINK_TOOL_TIP_H__
