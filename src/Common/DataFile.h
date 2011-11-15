#ifndef __DATAFILE_H__
#define __DATAFILE_H__

/*LICENSE_START*/ 
/* 
 *  Copyright 1995-2002 Washington University School of Medicine 
 * 
 *  http://brainmap.wustl.edu 
 * 
 *  This file is part of CARET. 
 * 
 *  CARET is free software; you can redistribute it and/or modify 
 *  it under the terms of the GNU General Public License as published by 
 *  the Free Software Foundation; either version 2 of the License, or 
 *  (at your option) any later version. 
 * 
 *  CARET is distributed in the hope that it will be useful, 
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 *  GNU General Public License for more details. 
 * 
 *  You should have received a copy of the GNU General Public License 
 *  along with CARET; if not, write to the Free Software 
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 */ 


#include <AString.h>

#include "CaretObject.h"
#include "DataFileInterface.h"

namespace caret {

    /**
     * Abstract Data File.
     */
    class DataFile : public CaretObject, public DataFileInterface {
        
    protected:
        DataFile();
        
        virtual ~DataFile();

        DataFile(const DataFile& s);

        DataFile& operator=(const DataFile&);
        
    public:
        virtual AString getFileName() const;
        
        virtual AString getFileNameNoPath() const;
        
        virtual void setFileName(const AString& filename);
        
        /**
         * Read the data file.
         *
         * @param filename
         *    Name of the data file.
         * @throws DataFileException
         *    If the file was not successfully read.
         */
        virtual void readFile(const AString& filename) throw (DataFileException) = 0;
        
        /**
         * Write the data file.
         *
         * @param filename
         *    Name of the data file.
         * @throws DataFileException
         *    If the file was not successfully written.
         */
        virtual void writeFile(const AString& filename) throw (DataFileException) = 0;
        
        virtual void clear();
        
    private:
        void copyHelperDataFile(const DataFile& df);
        
        void initializeMembersDataFile();
        
        /** name of data file */
        AString filename;
        
    };
    
} // namespace

#endif // __DATAFILE_H__
